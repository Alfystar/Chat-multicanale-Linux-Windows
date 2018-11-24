#define _GNU_SOURCE             /* See feature_test_macros(7) */


#include <stdio.h>

/** Librerie per creare la Pipe**/


#include "helpFunx.h"

/** LIBRERIE Ad Hoc **/
#include "include/fileSystemUtylity.h"
#include "include/terminalShell.h"  /** screen Shell lib **/


/** LIBRERIE Connessione **/

//#include "include/socketConnect.h"


/** STRUTTURE & Typedef DEL MAIN **/

/** PROTOTIPI DEL MAIN **/




/** TIDs Of acceptTh **/
pthread_t *acceptArray;

void helpProject (void){
	printf ("I parametri inseribili sono:\n");
	printf ("\tServer command:\n");
	printf ("\t[storage] [coda] [port]\nCreo il server nella cartella, porta, e per {coda} persone specificate\n");

}

void pipeInit (){
	int errorRet;

	/** Creo le pipe per scrivere instdout,stderr e debug sul monitori**/
	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2 (FdStdOutPipe, O_DIRECT);
	if (errorRet != 0){
		printErrno ("La creazione della pipe per lo stdout ha dato l'errore", errorRet);
		exit (-1);
	}
	fdOut = FdStdOutPipe[1];

	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2 (FdStdErrPipe, O_DIRECT);
	if (errorRet != 0){
		printErrno ("La creazione della pipe per lo stderr ha dato l'errore", errorRet);
		exit (-1);
	}

	//crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	errorRet = pipe2 (FdDebugPipe, O_DIRECT);
	if (errorRet != 0){
		printErrno ("La creazione della pipe per lo stderr ha dato l'errore", errorRet);
		exit (-1);
	}
	fdDebug = FdDebugPipe[1];
}

void print_trace (int nSig){
	sleep (1);
	endwin ();

	printf ("\n\nprint_trace: got signal %d\nCurrent Thread in Scheduling are:\n", nSig);

	void *array[32];    /* Array to store backtrace symbols */
	size_t size;     /* To store the exact no of values stored */
	char **strings;    /* To store functions from the backtrace list in ARRAY */
	size_t nCnt;

	size = backtrace (array, 32);

	strings = backtrace_symbols (array, size);

	/* prints each string of function names of trace*/
	for (nCnt = 0; nCnt < size; nCnt++)
		printf ("%s\n", strings[nCnt]);


	exit (-1);
}

int main (int argc, char *argv[]){
	if (argc != 4){
		helpProject ();
		exit (EXIT_FAILURE);
	}

	signal (SIGSEGV, print_trace);   //catturo sigSegv per potermi printare info varie

	pipeInit ();

	/** fase di avvio STORAGE del server **/
	if (StartServerStorage (argv[1]) == -1){
		perror ("\n!! Il server non è stato in grado di essere inizializato\nEXIT_FAILURE :");
		exit (EXIT_FAILURE);
	}
	storagePathServer = argv[1];
	printf ("\t#### SERVER STORAGE AVVIATO ####\n\n");

	/** INIT GLOBAL AVL TREE**/
	printf ("[3]---> Fase 3 Inizializzare alberi & SEMAFORI AVL avl dove verranno posizionate le pipe dei thread Room e User\n");

	usAvlTree_Pipe = init_avl_S ();
	rmAvlTree_Pipe = init_avl_S ();
	if (!usAvlTree_Pipe.avlRoot || !rmAvlTree_Pipe.avlRoot){
		printf ("[3]---> Fase 3: FALLITA\n");
		exit (-1);
	}
	printf ("[3]---> success\n\n");

	/** Spawn dei thread ROOM **/
	// Necessario prima delle connessioni perche' le room sono intrinseche al server
	// I th-User sono invece instanziati ad-hoc
	printf ("[4]---> Fase 4 Spawning Th-room salvati nello storage\n");
	nameList *chats = chatRoomExist ();
	printf ("Current chatRoom created:\n");
	nameListPrint (chats, STDOUT_FILENO);
	char roomDir[128];
	infoChat *info;
	for (int i = 0; i < chats->nMemb; i++){
		sprintf (roomDir, "./%s/%s", chatDirName, chats->names[i]);
		info = openRoom (roomDir);
		strncpy (info->myPath, roomDir, 128);
		//idKey è la prima parte del nome, ovvero IDKEY:XXXXX
		makeThRoom (atoi (chats->names[i]), roomDir, info);
	}
	nameListFree (chats);
	printf ("[4]---> Fase 4 Spawning ROOM-th (GENERAL) success\n\n");

	/** fase di avvio CONNESSIONE del server **/
	printf ("[5]---> Fase 5 Inizializzazione connessione e creazione Thread-accept\n");
	portProces = atoi (argv[3]);   //save global port data
	connection *serverCon = initSocket ((u_int16_t)portProces, "INADDR_ANY");
	if (serverCon == NULL){
		exit (-1);
	}

	printf ("\t1)Socket inizializzato\n");
	if (initServer (serverCon, (int)strtol (argv[2], NULL, 10)) == -1){
		exit (-1);
	}
	printf ("\t2)Server CONNECTION ONLINE\n");
	printf ("\t3)Spawning n°=%d accept-th\n", nAcceptTh);
	/** Spawn dei thread accetta user **/
	int errorRet;
	acceptArray = malloc (nAcceptTh * sizeof (pthread_t));
	thAcceptArg *acceptArg;
	for (int i = 0; i < nAcceptTh; i++){
		acceptArg = malloc (sizeof (thAcceptArg));
		acceptArg->id = i;
		acceptArg->conInfo.con = *serverCon;
		acceptArg->conInfo.arg = NULL;

		errorRet = pthread_create (&acceptArray[i], NULL, acceptTh, acceptArg);
		if (errorRet != 0){
			printErrno ("La creazione del Thread Accept ha dato il seguente errore", errorRet);
			exit (-1);
		}
	}
	printf ("[5]---> Fase 5 Success\n");
	printf ("\t#### SERVER CONNECTION ALL ONLINE ####\n\n");

	/** Il Main Thread  diventa il terminale con cui interagire da qui in poi è il terminale **/
	printf ("\n\n__________________________________________________________________\n[x][x][x][x][x][x]\tAvvio Del terminale\t[x][x][x][x][x][x]\n");

	/** Ridirezione dello stdErr, così chè gli errori vengano tutti scritti in rosso nel riquadro corrispondente **/
	dup2 (FdStdErrPipe[1], STDERR_FILENO);
	close (FdStdErrPipe[1]);
	usleep (500000);

	terminalShell ();
	printf ("\n\nServer Terminato da shell, senza presenza di errori\n\n");
	return 0;
}