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
bool executeNoHold = true;
int semid;
int logs;
int semid;
int des;
void zwolnijZasoby(){
	semctl(semid, 0, IPC_RMID, ctl);
	semctl(semid, 1, IPC_RMID, ctl);
	semctl(semid, 2, IPC_RMID, ctl);
	semctl(semid, 3, IPC_RMID, ctl);
	semctl(semid, 4, IPC_RMID, ctl);
	semctl(semid, 5, IPC_RMID, ctl);
	semctl(semid, 6, IPC_RMID, ctl);
	semctl(semid, 7, IPC_RMID, ctl);
	remove("/tmp/plikfifo");
	kill(rodzic, 9);
}

int semLock(int semid, int semIndex){
	struct sembuf opr;
	opr.sem_num = semIndex;
	opr.sem_op = -1;
	opr.sem_flg = 0;

	if(semop(semid, &opr, 1)==1){
		warn("Blad blokowania semafora!\n");
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
		warn("Blad odblokowania semafora!\n");
		return 0;	
	}
	return 1;
}


void * kontrolaZatrzymaniaD3(void *arg){//oczekiwanie w wątku w D3 na otwarcie semafora zamknięcia z D2
	semLock(semid, 2);
	printf("Zatrzymuje d3\n");
	zwolnijZasoby();
	exit(0);
}

void * kontrolaWstrzymaniaD3(void *arg){//oczekiwanie w wątku w D3 na otwarcie semafora wstrzymania z D2
	while(1){
		semLock(semid, 4);
		printf("Wstrzymuje d3\n");
		execute = false;
	}
}

void * kontrolaKontynuowaniaD3(void *arg){//oczekiwanie w wątku w D3 na otwarcie semafora wstrzymania z D2
	while(1){
		semLock(semid, 6);
		printf("Uruchamiam d3\n");
		execute = true;
	}
}

void * kontrolaZatrzymaniaD2(void *arg){// oczekiwanie w D2 na otwarcie zemafora zakmnięcia z D3
	semLock(semid, 3);
	kill(d2, 2);
}

void * kontrolaWstrzymaniaD2(void *arg){// oczekiwanie w D2 na otwarcie zemafora wstrzymania z D3
	while(1){
		semLock(semid, 5);
		kill(d2, 15);
	}	
}

void * kontrolaKontynuowaniaD2(void *arg){// oczekiwanie w D2 na otwarcie zemafora wstrzymania z D3
	while(1){
		semLock(semid, 7);
		kill(d2, 20);
	}	
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
	//printf("(6)");
	fflush(stdout);
	executeNoHold = true;
	//printf("(7 - %i)", execute);
	fflush(stdout);
}



void dziecko1(){
	//if(execute==true)printf("(exe = %i)", execute);
	if(executeNoHold==true && execute){
		executeNoHold=false;
		char znak;
	//	printf("(0)");
		fflush(stdout);
		int rd;
		if(read(0, &znak, 1) == 1){
			//printf("(1 :pobralem)");
			fflush(stdout);
			write(des, &znak, 1);
			//close(des);

		}
		else 
		{

			kill(rodzic, 2);
			//while(1);
		}
	}
	//else write(logs, "Dziecko 1 wstrzymane\n", 21);
}

void dziecko2(){
	if(execute){
		char znak;
		read(des, &znak, 1);
		//printf("(2)");
		fflush(stdout);
		close(deskryptory[0]);
		write(deskryptory[1], &znak,1);
		semUnlock(semid, 0);
		semLock(semid, 1);
		//printf("(5 - d1: %d)", d1);
		fflush(stdout);
		kill(d1, 12);// jak się trzecie wykona, to uruchamiamy pierwsze
	}
	else write(logs, "Dziecko 2 wstrzymane\n", 21);
}

void dziecko3(){
	if(execute){
		semLock(semid, 0);
		close(deskryptory[1]);
		unsigned char znak;
		read(deskryptory[0], &znak, 1);				
		//printf("(3)");
		printf("0x%02X ", znak);
		fflush(stdout);
		//sleep(1);
		//printf("(4)");
		semUnlock(semid, 1);
		fflush(stdout);
	}
}

void zamknijWszystkie(int signal){//wykonanie gdy dany proces odbierze sygnał zamknięcia
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

void wstrzymajWszystkie(int signal){//wykonanie gdy dany proces odbierze sygnał zamknięcia
	if(getpid()==d1){
		if(execute==true){
		printf("Wstrzymuje d1\n");
		kill(rodzic, 15);
		execute = false;
		}
		else {
		}
		}
	else if(getpid()==d2){
		printf("Wstrzymuje d2\n");
		kill(d1, 15);
		semUnlock(semid, 4);//wstrzymanie procesu 3
		execute = false;
	}

	else if(getpid()==d3){
		semUnlock(semid, 5);
		printf("Wstrzymuje d3\n");
		execute=false;
	}

	else if(getpid() == rodzic){
		kill(d1, 2);
	}
}

void kontynuujWszystkie(int signal){//wykonanie gdy dany proces odbierze sygnał zamknięcia
	if(getpid()==d1){
		if(execute==false){
		printf("Uruchamiam d1\n");
		kill(rodzic, 20);
		execute = true;
		}
		else {
		}
		}
	else if(getpid()==d2){
		printf("Uruchamiam d2\n");
		kill(d1, 20);
		semUnlock(semid, 6);//uruchomienie procesu 3
		execute = true;
	}

	else if(getpid()==d3){
		semUnlock(semid, 7);
		printf("Uruchamiam d3\n");
		execute=true;
	}
}

void posrednikD1D2(int signal){
	if(signal==10){
		kill(d2, 2);
	}
	else if(signal==15){
		kill(d2, 15);
	}
	else if(signal==20){
		kill(d2, 20);
	}
}




int main(){
	key_t semKey = ftok(".", 'A');
	semid = semget(semKey, 8, IPC_CREAT | 0600);
	ctl.val = 1 ; //zezwolenie na zapis
	semctl(semid, 0, SETVAL, ctl);

	ctl.val = 0 ; //zezwolenie na odczyt
	semctl(semid, 1, SETVAL, ctl);

	ctl.val = 0 ; //semafor zatrzymania procesu D3
	semctl(semid, 2, SETVAL, ctl);	

	ctl.val = 0 ; //semafor wysłania zatrzymania z D3 do D2
	semctl(semid, 3, SETVAL, ctl);	

	ctl.val = 0 ; //semafor wstrzymania z D2 do D3
	semctl(semid, 4, SETVAL, ctl);	

	ctl.val = 0 ; //semafor wstrzymania z D3 do D2
	semctl(semid, 5, SETVAL, ctl);	

	ctl.val = 0 ; 
	semctl(semid, 6, SETVAL, ctl);	

	ctl.val = 0 ; 
	semctl(semid, 7, SETVAL, ctl);	

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
				signal(15, posrednikD1D2);
				signal(20, posrednikD1D2);
				while(1);
			}
			else{
				//d3
				d3 = getpid();
				signal(2, zamknijWszystkie);
				signal(15, wstrzymajWszystkie);
				signal(20, kontynuujWszystkie);
				pthread_t p_thread[3];
				pthread_create(&p_thread[0], NULL, kontrolaZatrzymaniaD3, NULL);
				pthread_create(&p_thread[1], NULL, kontrolaWstrzymaniaD3, NULL);
				pthread_create(&p_thread[2], NULL, kontrolaKontynuowaniaD3, NULL);
				while(1){
					dziecko3();
				}

			}
		}
		else{	
				//2 dziecko
				des = open( "/tmp/plikfifo", O_RDONLY);
				d2 = getpid();
				pthread_t p_thread[3];
				pthread_create(&p_thread[0], NULL, kontrolaZatrzymaniaD2, NULL);
				pthread_create(&p_thread[1], NULL, kontrolaWstrzymaniaD2, NULL);
				pthread_create(&p_thread[2], NULL, kontrolaKontynuowaniaD2, NULL);
				signal(2, zamknijWszystkie);// Dziecko 2 otrzymuje sygnał zamknięcia i zamyka dziecko1 i dziecko 3
				signal(15, wstrzymajWszystkie);
				signal(20, kontynuujWszystkie);

				while(1){
					dziecko2();
				};
		}
	}
	else{
		//1dziecko
			d1 = getpid();
			des = open( "/tmp/plikfifo", O_WRONLY);
			signal(12, powrotDoD1);
			signal(2, zamknijWszystkie);
			signal(15, wstrzymajWszystkie);
			signal(20, kontynuujWszystkie);

			while(1){
				dziecko1();
			}
			
	}
}
