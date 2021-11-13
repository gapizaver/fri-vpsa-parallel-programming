/*H********************************************************************************
* Ime datoteke: serverLinux.cpp
*
* Opis:
*		Enostaven stre�nik, ki zmore sprejeti le enega klienta naenkrat.
*		Stre�nik sprejme klientove podatke in jih v nespremenjeni obliki po�lje
*		nazaj klientu - odmev.
*
*H*/

//Vklju�imo ustrezna zaglavja
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<pthread.h>
#include<stdlib.h>
/*
Definiramo vrata (port) na katerem bo stre�nik poslu�al
in velikost medponilnika za sprejemanje in po�iljanje podatkov
*/
#define PORT 1053
#define BUFFER_SIZE 256
#define MAX_POVEZAV 3

void* threadWork(void* cs);
// števec povezav
pthread_mutex_t mut;
int counter = 0;

int main(int argc, char **argv){

	//Spremenjlivka za preverjane izhodnega statusa funkcij
	int iResult;
	pthread_t thid;
                                                                                
	/*
	Ustvarimo nov vti�, ki bo poslu�al
	in sprejemal nove kliente preko TCP/IP protokola
	*/
	int listener=socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Error creating socket\n");
		return 1;
	}

	//Nastavimo vrata in mre�ni naslov vti�a
	sockaddr_in  listenerConf;
	listenerConf.sin_port=htons(PORT);
	listenerConf.sin_family=AF_INET;
	listenerConf.sin_addr.s_addr=INADDR_ANY;

	//Vti� pove�emo z ustreznimi vrati
	iResult = bind( listener, (sockaddr *)&listenerConf, sizeof(listenerConf));
	if (iResult == -1) {
		printf("Bind failed\n");
		close(listener);
		return 1;
    }

	//Za�nemo poslu�ati
	if ( listen( listener, 5 ) == -1 ) {
		printf( "Listen failed\n");
		close(listener);
		return 1;
	}

	//Definiramo nov vti� in medpomnilik
	int clientSock;
	
	/*
	V zanki sprejemamo nove povezave
	in jih stre�emo (najve� eno naenkrat)
	*/
	while (1)
	{
		//Sprejmi povezavo in ustvari nov vti�
		clientSock = accept(listener,NULL,NULL);
		if (clientSock == -1) {
			printf("Accept failed\n");
			close(listener);
			return 1;
		}



		//! ustvari novo nit in ji kot argument predaj clientSock
		if (pthread_create(&thid, NULL, threadWork, (void*) &clientSock) != 0) {
			perror("Napaka pri kreiranju niti:");
			exit(1);
		}

		pthread_detach(thid);
	}

	//Po�istimo vse vti�e
	close(listener);

	return 0;
}

void* threadWork(void* cs) {
	int clientSock = *((int *) cs);
	
	pthread_mutex_lock(&mut);
	if (counter < MAX_POVEZAV) {
		counter++;
		pthread_mutex_unlock(&mut);
	} else {
		pthread_mutex_unlock(&mut);
		close(clientSock);
		return NULL;
	}

	//Postrezi povezanemu klientu
	int iResult;
	char buff[BUFFER_SIZE];
	// preveri če max povezav
	

	do {
		//Sprejmi podatke
		iResult = recv(clientSock, buff, BUFFER_SIZE, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			//Vrni prejete podatke po�iljatelju
			iResult = send(clientSock, buff, iResult, 0 );
			if (iResult == -1) {
				printf("send failed!\n");
				close(clientSock);
				break;
			}
			printf("Bytes sent: %d\n", iResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else{
			printf("recv failed!\n");
			close(clientSock);
			break;
		}

	} while (iResult > 0);

	close(clientSock);

	pthread_mutex_lock(&mut);
	counter--;
	pthread_mutex_unlock(&mut);
	return NULL;
}

