#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>

pid_t d1, d2, d3, rodzic;
int deskryptory[2];


void posrednik1(int signal){
	kill(d2, 10);
	return;
}

void posrednik2(int signal){
	kill(d1, 10);
	return;
}

void dziecko1(){
	int i = 0;	
	char *znaki = malloc(sizeof(char));
	read(0, &znaki[i], 1);
	while(znaki[i]!='\n'){
		i++;
		znaki = realloc(znaki, sizeof(char)*(i+1));
		read(0, &znaki[i], 1);
	}
	znaki[i]='\0';
	des = open( "/tmp/plikfifo", O_WRONLY);
	write(des, &i, sizeof(int));
    write(des, znaki, i*sizeof(char));			
	close(des);
}

void dziecko2(){
	int des = open( "/tmp/plikfifo", O_RDONLY);
	int size;
	read(des, &size, sizeof(int));
	char *znaki = malloc(sizeof(char)*size);
	read(des, znaki, size);
	znaki[size]='\0';
	close(deskryptory[0]);
	write(deskryptory[1], &size,sizeof(int));
	write(deskryptory[1], znaki,size*sizeof(char));
}

void dziecko3(){
	close(deskryptory[1]);
	int size;
	read(deskryptory[0], &size, sizeof(int));
	char *znaki = malloc(sizeof(char)*size);
	read(deskryptory[0], znaki,size);
	znaki[size]='\0';

	printf("dziecko3: %s\n", znaki);
}


int main(){
	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	pipe(deskryptory);
	rodzic = getpid();
	
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
			while(1){
				dziecko1();
			}
	}
}
