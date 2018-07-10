#include <stdio.h>
#include <stdlib.h>

#include "helpFunx.h"

#include "fileSystemUtylity.h"



int errorRet;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("!#!#!#\tPrego inserire i dati mancanti:\n!#!#!#\t<Path-Server-Data>\n");
	    exit(EXIT_FAILURE);
    }
    StartServerStorage(argv[1]);

	char **chats = chatRoomExist();
	printDublePointeChar(chats);
	freeDublePointerArr(chats, sizeof(char **));

	return 0;
}




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
 * /---|----|---|-----------|----|-----------|----|---\
 * | 1 | Id | 2 |   NAME    |'\0'| password  |'\0'| 3 |
 * \---|----|---|-----------|----|-----------|----|---/
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
