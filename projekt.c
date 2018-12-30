#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>

int deskryptory[2];

int des, des2; 
pid_t d1, d2, d3, rodzic;
bool pobierzCiag, wczytajPlik;
int deskryptory[2];


void posrednik1(int signal){
	//printf("posrednik\n");	
	kill(d2, 10);
	return;
}

void posrednik2(int signal){
	//printf("posrednik2\n");	
	kill(d1, 10);
	return;
}


void dziecko2(){
	int des;
	//des = open( "/tmp/plik.txt", O_CREAT | O_RDWR| O_APPEND,  0666 );
	//des2 = open( "/tmp/plik", O_WRONLY);		
	//sleep(4);
	des = open( "/tmp/plikfifo", O_RDONLY);
	int size;
	read(des, &size, 1);
	//size++;
	char *znak = malloc(size*sizeof(char));
	read(des, znak,size);
	//printf("z dziecka2: %s\n",znak);
	close(deskryptory[0]);
	write(deskryptory[1], &size,1);
	printf("%s\n", znak);
	write(deskryptory[1], znak,size);
	//close(deskryptory[1]);
	//write(des2,&znak, 1);
	//close(des2);
	close(des);
}

void dziecko3(){
	//int des;
	//des = open( deskryptory[0], O_RDONLY);
	close(deskryptory[1]);
	int size;
	read(deskryptory[0], &size, 1);
	//size++;
	char *znak = malloc(size*sizeof(char));
	read(deskryptory[0], znak,size);
	//close(deskryptory[1]);
	//printf("%s\n", znak);
	//write(des2,&znak, 1);
	//close(des2);


}


int main(){
	mknod("/tmp/plikfifo", S_IFIFO|0666, 0);
	pipe(deskryptory);
	pobierzCiag = true;	
	rodzic = getpid();
	if((d1=fork())){
		if((d2=fork())){
			if((d3=fork())){
				//rodzic
//
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
				//signal(10, saveSignal);
					//printf("wczytuje\n");
				//kill(rodzic, 12);
				while(1){
					dziecko2();
				};
				
				
				

		}
	}
	else{
		//1dziecko
			//signal(10, readSignal);
			while(1){
				//if(pobierzCiag==1){
					des = open( "/tmp/plikfifo", O_WRONLY);
					int i = 0;	
					char *znaki = malloc(sizeof(char));
					read(0, &znaki[i], 1);
					while(znaki[i]!='\n'){
						i++;
						znaki = realloc(znaki, sizeof(char)*(i+1));
						read(0, &znaki[i], 1);
					}
					int val = i+1;
					write(des, &val,1);
					write(des, znaki,i+1);
					//kill(rodzic,10);						
					close(des);
					//pobierzCiag=false;
				//}	
			}

		while(1);
	}
}
