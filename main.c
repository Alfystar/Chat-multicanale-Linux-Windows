#define _GNU_SOURCE             /* See feature_test_macros(7) */


#include <stdio.h>
#include <stdlib.h>



/** Librerie per creare la Pipe**/
#include <fcntl.h>
#include <limits.h>


#include "helpFunx.h"
#include "defineSets.h"

/** LIBRERIE Ad Hoc **/
#include "include/fileSystemUtylity.h"
#include "include/mexData.h"
#include "include/tableFile.h"
#include "include/terminalShell.h"  /** screen Shell lib **/
#include "thFunx.h"

/** LIBRERIE Connessione **/

#include "include/socketConnect.h"


/** STRUTTURE & Typedef DEL MAIN **/

typedef struct thUserServ_ {
	int id;
	char name[128];
} thUserServ;

/** PROTOTIPI DEL MAIN **/



/*
 * LaS libreria ncurse usa lo stdout per printare gli schermi, di conseguenza redirizzando il flusso si perde
 * la possibilità di visualizzare a schermo le finestre.
 * Per lo stdErr non è così, di conseguenza vengono create 2 pipe, in cui quella dello stdErr viene redirezionata
 * a un thread per farla visualizzare quando serve, mentre se si vuole printare a schermo delle informazioni normali
 * si deve usare la pipe Stdout la quale ha dietro un thread che si occupa di visualizzare la cosa
 */

int fdStdoutPipe[2];  // dal manuale: fdStdoutPipe[0] refers to the read end of the pipe. fdStdoutPipe[1] refers to the write end of the pipe.
int fdStdErrPipe[2];  // dal manuale: fdStdoutPipe[0] refers to the read end of the pipe. fdStdoutPipe[1] refers to the write end of the pipe.

int fdOutP;

int errorRet;

pthread_t *acceptArray;

void helpProject(void) {
    printf("I parametri inseribili sono:\n");
    printf("\tServer command:\n");
    printf("\t[storage] [port] [coda]\t\tcreo il server nella cartella, porta, e per {coda} persone specificate\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        helpProject();
		exit(EXIT_FAILURE);
	}

	/** Creo la pipe che avrà funzione di stdout e stderr al th**/
	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2(fdStdoutPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stdout ha dato l'errore", errorRet);
		exit(-1);
	}
	fdOutP = fdStdoutPipe[1];

	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2(fdStdErrPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stderr ha dato l'errore", errorRet);
		exit(-1);
	}

    /** fase di avvio STORAGE del server **/
    if (StartServerStorage(argv[1]) == -1)
	{
		perror("\n!! Il server non è stato in grado di essere inizializato\nEXIT_FAILURE :");
		exit(EXIT_FAILURE);
	}
    printf("SERVER STORAGE AVVIATO\n");

    /** Spawn dei thread ROOM **/
	// Necessario prima delle connessioni perche' le room sono intrinseche al server
	// I thread user sono invece instanziati ad-hoc

	nameList *chats = chatRoomExist();
	char roomDir[128];
	infoChat *info;
	for (int i = 0; i < chats->nMemb; i++) {
		sprintf(roomDir, "./%s/%s", chatDirName, chats->names[i]);
		info = openRoom(roomDir);
		makeThRoom(i, roomDir, info);
	}
	printf("ROOM-th start-Up creati\n");


	/** fase di avvio CONNESSIONE del server **/

	connection *con = initSocket((u_int16_t) strtol(argv[2], NULL, 10), "INADDR_ANY");
	if (con == NULL) {
		exit(-1);
	}

	printf("Socket inizializzato\n");
	if (initServer(con, (int) strtol(argv[3], NULL, 10)) == -1) {
		exit(-1);
	}
	printf("Server CONNESSIONE avviato\n");

	/** Spawn dei thread accetta user **/
	thUserServ *arg;
	int i = 0;

	while (1) {
		arg = malloc(sizeof(thUserServ));
		printf("arg = %p creato.\n", arg);
		arg->id = i;
		if (acceptCreate(con, thUserServer, arg) == -1) {
			exit(-1);
		}
		i++;
	}


	// precedente implementazione, todo: farle convergere
	acceptArray = malloc(nAcceptTh * sizeof(pthread_t));
	for (int i = 0; i < nAcceptTh; i++) {
		thAcceptArg *arg = malloc(sizeof(thAcceptArg));
		arg->id = i;
		arg->fdStdout = fdStdoutPipe[1];
		arg->fdStderr = fdStdErrPipe[1];
		errorRet = pthread_create(&acceptArray[i], NULL, acceptTh, arg);
		if (errorRet != 0) {
			printErrno("La creazione del Thread Accept ha dato il seguente errore", errorRet);
			exit(-1);
		}
	}

	/** Visualizzazione chat già presenti all'avvio**/

	printf("Current nameList created:\n");
	nameListPrint(chats);
	nameListFree(chats);

	/** Il Main Thread  diventa il terminale con cui interagire da qui in poi è il terminale **/
	printf("\n\n__________________________________________________________________\n[x][x][x][x][x][x]\tAvvio Del terminale\t[x][x][x][x][x][x]\n");
	/** Ridirezione dello stdErr, così chè gli errori vengano tutti scritti in rosso nel riquadro corrispondente **/
	dup2(fdStdErrPipe[1], STDERR_FILENO);
	close(fdStdErrPipe[1]);
	usleep(500000);

	terminalShell(fdStdoutPipe, fdStdErrPipe);

	return 0;
}