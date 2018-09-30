#define _GNU_SOURCE             /* See feature_test_macros(7) */


#include <stdio.h>
#include <stdlib.h>

/** Thread lib **/
#include <unistd.h>
#include <pthread.h>

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



/** STRUTTURE & Typedef DEL MAIN **/
typedef struct thAcceptArg_ {
	int id;
	int fdStdout;
	int fdStderr;
} thAcceptArg;

typedef struct thUserArg_ {
	int id;
} thUserArg;

typedef struct thRoomArg_ {
	int id;
	char name[28];
	infoChat *info;
} thRoomArg;


/** PROTOTIPI DEL MAIN **/
void *acceptTh(thAcceptArg *);

void *userTh(thUserArg *);

void *roomTh(thRoomArg *);

void titlePrintW(WINDOW *, int, int);

void makeThRoom(int keyChat, char *name, infoChat *info);


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

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("!#!#!#\tPrego inserire i dati mancanti:\n!#!#!#\t<Path-Server-Data>\n");
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

	/** fase di avvio del server **/
	if (StartServerStorage(argv[1]) == -1) //errore di qualche tipo nell'avvio del server
	{
		printf("\n!! Il server non è stato in grado di essere inizializato\nEXIT_FAILURE\n");
		exit(EXIT_FAILURE);
	}
	printf("SERVER AVVIATO\n");

	/** Spawn dei thredh ROOM **/
	nameList *chats = chatRoomExist();
	char roomDir[128];
	infoChat *info;
	for (int i = 0; i < chats->nMemb; i++) {
		sprintf(roomDir, "./%s/%s", chatDirName, chats->names[i]);
		info = openRoom(roomDir);
		makeThRoom(i, roomDir, info);
	}
	printf("ROOM th creati\n");



	/** Spawn dei thredh accetta user **/
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


void menuHelp() {
	printf("-h\thelp\t-> elenco comandi disponibili\n");
	printf("-q\tquit\t-> termina server\n");
}

void *acceptTh(thAcceptArg *info) {
	while (1) {
		dprintf(fdStdoutPipe[1], "ciao sono un th Accept\n");
		pause();
	}
	free(info);
	return NULL;
}

void *userTh(thUserArg *info) {
	pause();
	free(info);
	return NULL;
}

void *roomTh(thRoomArg *info) {
	dprintf(fdStdoutPipe[1], "Ciao sono Un TH-ROOM\n\tsono la %d\n\tmi chiamo %s\n", info->id, info->name);
	pause();
	free(info);
	return NULL;

}

void makeThRoom(int keyChat, char *roomPath, infoChat *info) {
	pthread_t roomtid;
	thRoomArg *arg = malloc(sizeof(thRoomArg));
	arg->id = keyChat;
	strncpy(arg->name, roomPath, 28);
	arg->info = openRoom(roomPath);
	if (arg->info == NULL) {
		//todo: se nullo capire se è perchè già presente o non esistente.
	}
	errorRet = pthread_create(&roomtid, NULL, roomTh, arg);
	if (errorRet != 0) {
		printErrno("La creazione del Thread ROOM ha dato il seguente errore", errorRet);
		exit(-1);
	}
}