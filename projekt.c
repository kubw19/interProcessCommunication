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
bool execDziecko1 = true;
int semid;

void posrednik1(int signal){
	kill(d2, 10);
	return;
}

void posrednik2(int signal){
	kill(d1, 10);
	return;
}

void dziecko1(){
	printf("Dziecko1: ");
	for(int i =3;i>0;i--){
		printf("%d ",i);
		fflush(stdout);
		sleep(1);
	}
	printf("\nIn:  ");
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
	execDziecko1=false;
}

void dziecko2(){
	int des = open( "/tmp/plikfifo", O_RDONLY);
	printf("Dziecko2: ");
	for(int i =3;i>0;i--){
		printf("%d ",i);
		fflush(stdout);
		sleep(1);
	}
	printf("\n");
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
	kill(d1, 10);// jak się trzecie wykona, to uruchamiamy pierwsze
	semctl(semid, 0, SETVAL, 1);
}

void dziecko3(){
	close(deskryptory[1]);
	int size;
	read(deskryptory[0], &size, sizeof(int));
	printf("Dziecko3: ");
	for(int i =3;i>0;i--){
		printf("%d ",i);
		fflush(stdout);
		sleep(1);
	}
	printf("\n");
	char *znaki = malloc(sizeof(char)*size);
	read(deskryptory[0], znaki,size);
	znaki[size]='\0';

	printf("OUT: %s\n\n", znaki);
	semctl(semid, 0, SETVAL, 0);
}


void uruchomD1(int signal){
	execDziecko1=true;
}

int main(){
	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	pipe(deskryptory);
	rodzic = getpid();
	semid = semget(45281, 1, IPC_CREAT|0600);
	semctl(semid, 0, SETVAL, 1);
	
	if((d1=fork())){
		if((d2=fork())){
			if((d3=fork())){
				//rodzic
				while(1);
			}
			else{
				//d3
				while(1){
					dziecko3();
				}
			}
		}
		else{	
				//2 dziecko
				while(1){
					dziecko2();
				};
		}
	}
	else{
		//1dziecko
			signal(10, uruchomD1);
			while(1){
			if(execDziecko1){
				dziecko1();
			}
			}
			
	}
}
