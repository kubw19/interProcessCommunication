#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

pid_t d1, d2, d3, rodzic;
int deskryptory[2];
bool execute = true;
int semid;
int logs;

void posrednik1(int signal){
	kill(d2, 10);
	return;
}

void posrednik2(int signal){
	kill(d1, 10);
	return;
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

int checkInterrupt(){
	if(getpid()==d3 && semctl(semid, 1, GETVAL)==0){
		return 1;		
	}
	else if(getpid()==d3 && semctl(semid, 1, GETVAL)==1){
		return 0;
	}
	if(execute==false)return 1;
	return 0;
}

void wstrzymajWszystkie(int signal){

}

void uruchomWszystkie(int signal){

}

void dziecko1(){
	if(checkInterrupt()==0){
		printf("Dziecko1\n");
		printf("In:  ");
		fflush(stdout);

		int i = 0;	
		char *znaki = malloc(sizeof(char));
		read(0, &znaki[i], 1);
		while(znaki[i]!='\n'){
			i++;
			znaki = realloc(znaki, sizeof(char)*(i+1));
			read(0, &znaki[i], 1);
		}
		znaki[i]='\0';
		int des = open( "/tmp/plikfifo", O_WRONLY);
		write(des, &i, sizeof(int));
	    write(des, znaki, i*sizeof(char));			
		close(des);
		zatrzymaj(0);
	}
	else write(logs, "Dziecko 1 wstrzymane\n", 21);
}

void dziecko2(){
	if(checkInterrupt()==0){
		int des = open( "/tmp/plikfifo", O_RDONLY);
		printf("Dziecko2\n");
		int size;
		read(des, &size, sizeof(int));
		char *znaki = malloc(sizeof(char)*size);
		read(des, znaki, size);
		close(des);
		znaki[size]='\0';
		close(deskryptory[0]);
		write(deskryptory[1], &size,sizeof(int));
		write(deskryptory[1], znaki,size*sizeof(char));
		while(semctl(semid, 0, GETVAL)==1){ // czekanie na trzecie dziecko
			sleep(1);
		}
		kill(d1, 8);// jak się trzecie wykona, to uruchamiamy pierwsze
		semctl(semid, 0, SETVAL, 1);
	}
	else write(logs, "Dziecko 2 wstrzymane\n", 21);
}

void dziecko3(){
	if(checkInterrupt()==0){
		close(deskryptory[1]);
		int size;
		read(deskryptory[0], &size, sizeof(int));
		printf("Dziecko3\n");

		char *znaki = malloc(sizeof(char)*size);
		read(deskryptory[0], znaki,size);
		znaki[size]='\0';

		printf("OUT: %s\n\n", znaki);
		semctl(semid, 0, SETVAL, 0);
	}
	else write(logs, "Dziecko 3 wstrzymane\n", 21);
}


void sendKillAllRequest(int signal){
	kill(rodzic, 15);
}

void killAll(int signal){
	kill(d1, 9);
	kill(d2, 9);
	kill(d3, 9);
	kill(rodzic, 9);
}
int main(){
	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	logs = open("log.txt",  O_RDWR | O_CREAT | O_TRUNC);
	pipe(deskryptory);
	rodzic = getpid();
	semid = semget(45282, 2, IPC_CREAT|0600);
	semctl(semid, 0, SETVAL, 1);
	semctl(semid, 1, SETVAL, 1);
	
	if((d1=fork())){
		if((d2=fork())){
			if((d3=fork())){
				//rodzic
				signal(15, killAll);
				while(1);
			}
			else{
				//d3
				signal(10, wstrzymajWszystkie);
				signal(12, uruchomWszystkie);
				signal(15, sendKillAllRequest);
				while(1){
					dziecko3();
				}
			}
		}
		else{	
				//2 dziecko
				signal(10, wstrzymajWszystkie);
				signal(12, uruchomWszystkie);
				signal(15, sendKillAllRequest);
				while(1){
					dziecko2();
				};
		}
	}
	else{
		//1dziecko
			signal(8, uruchom);
			signal(10, wstrzymajWszystkie);
			signal(12, uruchomWszystkie);
			signal(15, sendKillAllRequest);
			printf("1 dziecko: %d\n", getpid());
			while(1){
				dziecko1();
				usleep(200000);
			}
			
	}
}
