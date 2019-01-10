//najnowsza wersja
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
bool odblokowal;
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
	printf("Zamykam D3\n");
	zwolnijZasoby();
	exit(0);
}

void * kontrolaWstrzymaniaD3(void *arg){//oczekiwanie w wątku w D3 na otwarcie semafora wstrzymania z D2
	while(1){
		semLock(semid, 4);
		printf("Wstrzymuje D3\n");
		execute = false;
	}
}

void * kontrolaKontynuowaniaD3(void *arg){//oczekiwanie w wątku w D3 na otwarcie semafora wstrzymania z D2
	while(1){
		semLock(semid, 6);
		printf("Kontynuuje D3\n");
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
		kill(d1, 15);
	}	
}

void * kontrolaKontynuowaniaD2(void *arg){// oczekiwanie w D2 na otwarcie zemafora wstrzymania z D3
	while(1){
		semLock(semid, 7);
		kill(d1, 20);
	}	
}


void dziecko1(){
	if(executeNoHold==true && execute){
		executeNoHold=false;
		char znak;
		fflush(stdout);
		int rd;
		if(read(0, &znak, 1) == 1){
			fflush(stdout);
			write(des, &znak, 1);
		}
		else 
		{
			kill(rodzic, 2);
		}
	}
}

void dziecko2(){
	if(execute){
		char znak;
		read(des, &znak, 1);
		fflush(stdout);
		close(deskryptory[0]);
		write(deskryptory[1], &znak,1);
		semUnlock(semid, 0);
		semLock(semid, 1);
		fflush(stdout);
		kill(d1, 21);// jak się trzecie wykona, to uruchamiamy pierwsze
	}
}

void dziecko3(){
	if(execute){
		semLock(semid, 0);
		close(deskryptory[1]);
		unsigned char znak;
		read(deskryptory[0], &znak, 1);				
		printf("0x%02X ", znak);
		fflush(stdout);
		semUnlock(semid, 1);
	}
}

void zamknijWszystkie(int signal){//wykonanie gdy dany proces odbierze sygnał zamknięcia
	if(getpid()==d1){
		printf("Zamykam D1\n");
		kill(rodzic, 10);
		exit(0);
	}
	else if(getpid()==d2 && signal == 2){
		kill(d1, 2);
	}
	else if(getpid()==d2 && signal == 10){
		printf("Zamykam D2\n");
		semUnlock(semid, 2);//przekazanie rozkazu zamknięcia do d3
		exit(0);
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
		execute = false;
		printf("Wstrzymuje D1\n");
		kill(rodzic, 12);
		}
		else {
		}
		}
	else if(getpid()==d2 && signal == 15){//przekazanie sygnalu wstrzymania do d1
		kill(d1, 15);
	}
	else if(getpid()==d2 && signal == 12){//wstrzymanie d2
		printf("Wstrzymuje D2\n");
		execute = false;
		semUnlock(semid, 4);//wstrzymanie procesu 3
	}

	else if(getpid()==d3){
		semUnlock(semid, 5);//wstrzymanie procesu 3
	}

}

void kontynuujWszystkie(int signal){//wykonanie gdy dany proces odbierze sygnał zamknięcia
	if(getpid()==d1){
		if(execute==false){
		kill(rodzic, 17);
		execute = true;
		printf("Kontynuuje D1\n");
		}
		else {
		}
		}
	else if(getpid()==d2 && signal == 20){//przekazanie sygnalu kontynuowania
		kill(d1, 20);
	}

	else if(getpid()==d2 && signal == 17){
		execute = true;
		printf("Kontynuuje D2\n");
		semUnlock(semid, 6);//uruchomienie procesu 3
	}

	else if(getpid()==d3){
		semUnlock(semid, 7);//uruchomienie procesu 2
	}
}

void posrednikD1D2(int signal){
	if(signal==10){
		kill(d2, 10);
	}
	else if(signal==12){
		kill(d2, 12);
	}
	else if(signal==17){
		kill(d2, 17);
	}
}

void powrotDoD1(int signal){
	executeNoHold = true;
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
	semctl(semid, 6, SETVAL, ctl);	//semafor uruchomienia z D2 do D3

	ctl.val = 0 ; 
	semctl(semid, 7, SETVAL, ctl);	//semafor uruchomienia z D3 do D2

	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	pipe(deskryptory);

	rodzic = getpid();
	
	if((d1=fork())){
		if((d2=fork())){
			if((d3=fork())){
				//rodzic
				signal(2, zamknijWszystkie);
				signal(10, posrednikD1D2);
				signal(12, posrednikD1D2);
				signal(17, posrednikD1D2);
				while(1);
			}
			else{
				//d3
				odblokowal = false;
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
				signal(10, zamknijWszystkie);
				signal(12, wstrzymajWszystkie);
				signal(17, kontynuujWszystkie);

				while(1){
					dziecko2();
				};
		}
	}
	else{
		//1dziecko
			d1 = getpid();
			des = open( "/tmp/plikfifo", O_WRONLY);
			signal(21, powrotDoD1);
			signal(2, zamknijWszystkie);
			signal(15, wstrzymajWszystkie);
			signal(20, kontynuujWszystkie);

			while(1){
				dziecko1();
			}
			
	}
}
