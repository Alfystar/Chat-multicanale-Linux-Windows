#define _GNU_SOURCE             /* See feature_test_macros(7) */


#include <stdio.h>
#include <stdlib.h>

/** Thread lib **/
#include <unistd.h>
#include <pthread.h>

/** screen Shell lib **/
#include "terminalShell.h"


/** Librerie per creare la Pipe**/
#include <fcntl.h>
#include <limits.h>


#include "helpFunx.h"

#include "fileSystemUtylity.h"

#include "defineSets.h"


/** STRUTTURE & Typedef DEL MAIN **/
typedef struct thAcceptlArg_ {
	int id;
	int fdStdout;
	int fdStderr;
} thAcceptlArg;

typedef struct thUserlArg_ {
	int id;
} thUserlArg;


/** PROTOTIPI DEL MAIN **/
void acceptTh(thAcceptlArg *);

void userTh(thUserlArg *);

void titlePrintW(WINDOW *, int, int);


int fdStdoutPipe[2];  // dal manuale: fdStdoutPipe[0] refers to the read end of the pipe. fdStdoutPipe[1] refers to the write end of the pipe.
int fdStdErrPipe[2];  // dal manuale: fdStdoutPipe[0] refers to the read end of the pipe. fdStdoutPipe[1] refers to the write end of the pipe.

int errorRet;

pthread_t *acceptArray;

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("!#!#!#\tPrego inserire i dati mancanti:\n!#!#!#\t<Path-Server-Data>\n");
		exit(EXIT_FAILURE);
	}

	/** Creo la pipe che avrà funzione di stdout e stderr al th**/
	errorRet = pipe2(fdStdoutPipe, 0); //crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
	if (errorRet != 0) {
		printErrno("La creazione della pipe per lo stdout ha dato l'errore", errorRet);
		exit(-1);
	}
	errorRet = pipe2(fdStdErrPipe, 0); //crea questa pipe e comunica per pacchetti, ogni READ leggerà un solo pacchetto
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

	/** Spawn dei thredh accetta user **/
	acceptArray = malloc(nAcceptTh * sizeof(pthread_t));
	for (int i = 0; i < nAcceptTh; i++) {
		thAcceptlArg *arg = malloc(sizeof(thAcceptlArg));
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
	char **chats = chatRoomExist();
	printf("Current chatRoom created:\n");
	printDublePointeChar(chats);
	freeDublePointerArr(chats, sizeof(chats));

	/** Il Main Thread  diventa il terminale con cui interagire da qui in poi è il terminale **/
	printf("\n\n__________________________________________________________________\n[x][x][x][x][x][x]\tAvvio Del terminale\t[x][x][x][x][x][x]\n");
	usleep(500000);


	terminalShell(fdStdoutPipe, fdStdErrPipe);

	return 0;
}


void menuHelp() {
	printf("-h\thelp\t-> elenco comandi disponibili\n");
	printf("-q\tquit\t-> termina server\n");
}

void acceptTh(thAcceptlArg *info) {
	while (1) {

		dprintf(info->fdStdout, " ac n°%d", info->id);
		sleep(1);
		dprintf(info->fdStderr, " acer n°%d", info->id);
		sleep(1);

	}
}

void userTh(thUserlArg *info) {

}


//TODO:login funx

//todo: insert in file
/*
 * Funzione trovata online che inserisce nella posizione attuale di fseek la stringa contenuta in *buffer
 int finsert (FILE* file, const char *buffer) {

    long int insert_pos = ftell(file);
    if (insert_pos < 0) return insert_pos;

    // Grow from the bottom
    int seek_ret = fseek(file, 0, SEEK_END);
    if (seek_ret) return seek_ret;
    long int total_left_to_move = ftell(file);
    if (total_left_to_move < 0) return total_left_to_move;

    char move_buffer[4096];
    long int ammount_to_grow = strlen(buffer);
    if (ammount_to_grow >= sizeof(move_buffer)) return -1;

    total_left_to_move -= insert_pos;

    for(;;) {
        u16 ammount_to_move = sizeof(move_buffer);  //prende la dimensione del buffer
        if (total_left_to_move < ammount_to_move) ammount_to_move = total_left_to_move;

        long int read_pos = insert_pos + total_left_to_move - ammount_to_move;

        seek_ret = fseek(file, read_pos, SEEK_SET);
        if (seek_ret) return seek_ret;
        fread(move_buffer, ammount_to_move, 1, file);
        if (ferror(file)) return ferror(file);

        seek_ret = fseek(file, read_pos + ammount_to_grow, SEEK_SET);
        if (seek_ret) return seek_ret;
        fwrite(move_buffer, ammount_to_move, 1, file);
        if (ferror(file)) return ferror(file);

        total_left_to_move -= ammount_to_move;

        if (!total_left_to_move) break;

    }

    seek_ret = fseek(file, insert_pos, SEEK_SET);
    if (seek_ret) return seek_ret;
    fwrite(buffer, ammount_to_grow, 1, file);
    if (ferror(file)) return ferror(file);

    return 0;
}


 **** esempio di uso
	FILE * file= fopen("test.data", "r+");
	ASSERT(file);

	const char *to_insert = "INSERT";

	fseek(file, 3, SEEK_SET);
	finsert(file, to_insert);

	ASSERT(ferror(file) == 0);
	fclose(file);

 *
 */
//todo: delete and compress data in file


//TODO: Id-name
/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere Id utenti messaggio*/

/*
 * Il file dovra contenere gli id degli utenti e contenere
 * anche il loro nome equivalente in codice ascii
 *
 *
 * (Opzionale)
 * Tenere traccia anche di una password per poter accedere.
 *
 *
 * /---|----|---|-----------|---|-----------|----|---\
 * | 1 | Id | 2 |   NAME    |':'| password  |'\0'| 3 |
 * \---|----|---|-----------|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: -2: in ascii indica "End of Trasmission"
 */

//TODO: Messaggi
/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere pacchetti messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * (i numeri sono i valori decimali dell'equivalente ascii)
 *
 * /---|----|-----|---|-----------|----|---\
 * | 1 | Id | Ora | 2 |   TEXT    |'\0'| 3 |
 * \---|----|-----|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: in ascii indica "End of Trasmission"
 */


/*TODO: creazione connessione tramite socket
 * 
 */

/*TODO: creazione thread utente e sue funzioni di appoggio
 *
 */
