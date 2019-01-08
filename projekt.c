#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <err.h>
#include <pthread.h>

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */

#else
/* according to X/OPEN we have to define it ourselves */
union semun {
      int val;                  /* value for SETVAL */
      struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
      unsigned short *array;    /* array for GETALL, SETALL */
                                /* Linux specific part: */
      struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif

union semun ctl;
pid_t d1, d2, d3, rodzic;
int deskryptory[2];
bool execute = true;
int semid;
int logs;
int semid;

void zwolnijZasoby(){
	semctl(semid, 0, IPC_RMID, ctl);
	semctl(semid, 1, IPC_RMID, ctl);
	semctl(semid, 2, IPC_RMID, ctl);
	semctl(semid, 3, IPC_RMID, ctl);
	kill(rodzic, 9);
}

int semLock(int semid, int semIndex){
	struct sembuf opr;
	opr.sem_num = semIndex;
	opr.sem_op = -1;
	opr.sem_flg = 0;

	if(semop(semid, &opr, 1)==1){
		warn("Blad blokowania semafora!");
		return 0;
	}
	return 1;
}

int semUnlock(int semid, int semIndex){
	struct sembuf opr;
	opr.sem_num = semIndex;
	opr.sem_op = 1;
	opr.sem_flg = 0;

	if(semop(semid, &opr, 1)==1){
		warn("Blad odblokowania semafora!");
		return 0;	
	}
	return 1;
}


void * kontrolaZatrzymaniaD3(void *arg){
	semLock(semid, 2);
	zwolnijZasoby();
	printf("rodzic: %d...", rodzic);
	exit(0);
}

void * kontrolaZatrzymaniaD2(void *arg){
	semLock(semid, 3);
	kill(d2, 2);
}


void uruchom(int signal){
	if(getpid()==d3){
		semctl(semid, 1, SETVAL, 0);
		return;
	}
	execute=true;
}


void zatrzymaj(int signal){
	if(getpid()==d3){
		semctl(semid, 0, SETVAL, 0);
		return;
	}
	execute=false;
}

void powrotDoD1(int signal){
	execute = true;
}

int checkInterrupt(){
	if(execute==false)return 1;
	return 0;
}




void dziecko1(){
	if(checkInterrupt()==0){
		char znak;
		if(read(0, &znak, 1)==1){
			int des = open( "/tmp/plikfifo", O_WRONLY);
			write(des, &znak, 1);
			close(des);
			execute=false;
		}
		else kill(rodzic, 2);
	}
	else write(logs, "Dziecko 1 wstrzymane\n", 21);
}

void dziecko2(){
	if(checkInterrupt()==0){

		int des = open( "/tmp/plikfifo", O_RDONLY);
		char znak;
		read(des, &znak, 1);
		close(des);
		close(deskryptory[0]);
		write(deskryptory[1], &znak,1);
		semUnlock(semid, 0);
		semLock(semid, 1);
		kill(d1, 12);// jak się trzecie wykona, to uruchamiamy pierwsze


	}
	else write(logs, "Dziecko 2 wstrzymane\n", 21);
}

void dziecko3(){
	semLock(semid, 0);
	close(deskryptory[1]);
	char znak;
	read(deskryptory[0], &znak, 1);				
	printf("%#X ", (int)znak);
	fflush(stdout);
	sleep(1);
	semUnlock(semid, 1);
}

void zamknijWszystkie(int signal){
	if(getpid()==d1){
		printf("Zatrzymuje d1\n");
		kill(rodzic, 10);
		kill(getpid(), 9);
	}
	else if(getpid()==d2){
		fflush(stdout);
		printf("Zatrzymuje d2\n");
		kill(d1, 2);
		semUnlock(semid, 2);
		kill(getpid(), 9);
	}

	else if(getpid()==d3){
		semUnlock(semid, 3);
	}

	else if(getpid() == rodzic){
		kill(d1, 2);
	}
}

//SYGNAŁY ODBIERANE PRZEZ DZIECKO1



//koniec

//SYGNAŁY ODBIERANE PRZEZ DZIECKO2

void posrednikD1D2(int signal){
	if(signal==10){
		kill(d2, 2);
	}
}




int main(){
	key_t semKey = ftok(".", 'A');
	semid = semget(semKey, 4, IPC_CREAT | 0600);
	ctl.val = 1 ; //zezwolenie na zapis
	semctl(semid, 0, SETVAL, ctl);

	ctl.val = 0 ; //zezwolenie na odczyt
	semctl(semid, 1, SETVAL, ctl);

	ctl.val = 0 ; //semafor zatrzymania procesu D3
	semctl(semid, 2, SETVAL, ctl);	

	ctl.val = 0 ; //semafor wysłania zatrzymania z D3 do D2
	semctl(semid, 3, SETVAL, ctl);	

	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	logs = open("log.txt",  O_RDWR | O_CREAT | O_TRUNC);
	pipe(deskryptory);
	rodzic = getpid();
	
	if((d1=fork())){
		if((d2=fork())){
			if((d3=fork())){
				//rodzic
				signal(2, zamknijWszystkie);
				signal(10, posrednikD1D2);
				while(1);
			}
			else{
				//d3
				d3 = getpid();
				signal(2, zamknijWszystkie);
				pthread_t p_thread;
				pthread_create(&p_thread, NULL, kontrolaZatrzymaniaD3, NULL);
				while(1){
					dziecko3();
				}
				pthread_join(p_thread, NULL);

			}
		}
		else{	
				//2 dziecko
				d2 = getpid();
				pthread_t p_thread;
				pthread_create(&p_thread, NULL, kontrolaZatrzymaniaD2, NULL);
				signal(2, zamknijWszystkie);// Dziecko 2 otrzymuje sygnał zamknięcia i zamyka dziecko1 i dziecko 3

				while(1){
					dziecko2();
				};
				pthread_join(p_thread, NULL);
		}
	}
	else{
		//1dziecko
			d1 = getpid();
			signal(12, powrotDoD1);
			signal(2, zamknijWszystkie);

			while(1){
				dziecko1();
			}
			
	}
}
