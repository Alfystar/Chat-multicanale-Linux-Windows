#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>

/** Librerie per creare la Pipe**/
#include <fcntl.h>
#include <limits.h>


#include "helpFunx.h"
#include "globalSet.h"

/** LIBRERIE Ad Hoc **/
#include "include/fileSystemUtylity.h"
#include "include/mexData.h"
#include "include/tableFile.h"
#include "include/terminalShell.h"  /** screen Shell lib **/
#include "include/thFunx.h"
#include "treeFunx/include/avl.h"


/** LIBRERIE Connessione **/

//#include "include/socketConnect.h"


/** STRUTTURE & Typedef DEL MAIN **/

/** PROTOTIPI DEL MAIN **/




/** TIDs Of acceptTh **/
pthread_t *acceptArray;

void helpProject(void) {
    printf("I parametri inseribili sono:\n");
    printf("\tServer command:\n");
    printf("\t[storage] [port] [coda]\nCreo il server nella cartella, porta, e per {coda} persone specificate\n");
}

void pipeInit() {
	int errorRet;

	/** Creo le pipe per scrivere instdout,stderr e debug sul monitori**/
	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2(FdStdOutPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stdout ha dato l'errore", errorRet);
		exit(-1);
	}
	fdOut = FdStdOutPipe[1];

	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2(FdStdErrPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stderr ha dato l'errore", errorRet);
		exit(-1);
	}

	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2(FdDebugPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stderr ha dato l'errore", errorRet);
		exit(-1);
	}
	fdDebug = FdDebugPipe[1];
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        helpProject();
		exit(EXIT_FAILURE);
	}
	pipeInit();

    /** fase di avvio STORAGE del server **/
    if (StartServerStorage(argv[1]) == -1)
	{
		perror("\n!! Il server non è stato in grado di essere inizializato\nEXIT_FAILURE :");
		exit(EXIT_FAILURE);
	}
    printf("SERVER STORAGE AVVIATO\n");

	usAvlPipe = init_avl_S();
	rmAvlPipe = init_avl_S();


	/** Spawn dei thread ROOM **/
	// Necessario prima delle connessioni perche' le room sono intrinseche al server
	// I thread user sono invece instanziati ad-hoc

	nameList *chats = chatRoomExist();
	char roomDir[128];
	infoChat *info;
	for (int i = 0; i < chats->nMemb; i++) {
		sprintf(roomDir, "./%s/%s", chatDirName, chats->names[i]);
		info = openRoom(roomDir);
		strncpy(info->myName, roomDir, 128);
		//idKey è la prima parte del nome, ovvero IDKEY:XXXXX
		makeThRoom(atoi(chats->names[i]), roomDir, info);
	}
	printf("ROOM-th start-Up creati\n");


	/** fase di avvio CONNESSIONE del server **/

	connection *serverCon = initSocket((u_int16_t) strtol(argv[2], NULL, 10), "INADDR_ANY");
	if (serverCon == NULL) {
		exit(-1);
	}

	printf("Socket inizializzato\n");
	if (initServer(serverCon, (int) strtol(argv[3], NULL, 10)) == -1) {
		exit(-1);
	}
	printf("Server CONNESSIONE avviato\n");

	/** Spawn dei thread accetta user **/
	int errorRet;
	// precedente implementazione, todo: farle convergere
	acceptArray = malloc(nAcceptTh * sizeof(pthread_t));
	thAcceptArg *acceptArg;
	for (int i = 0; i < nAcceptTh; i++) {
		acceptArg = malloc(sizeof(thAcceptArg));
		acceptArg->id = i;
		acceptArg->conInfo.con = *serverCon;
		acceptArg->conInfo.arg = NULL;

		errorRet = pthread_create(&acceptArray[i], NULL, acceptTh, acceptArg);
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
	dup2(FdStdErrPipe[1], STDERR_FILENO);
	close(FdStdErrPipe[1]);
	usleep(500000);

	terminalShell();

	return 0;
}