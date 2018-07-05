#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}

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
 * /---|----|---|-----------|----|---\
 * | 1 | Id | 2 |   NAME    |'\0'| 3 |
 * \---|----|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: -2: in ascii indica "End of Trasmission"
 */


/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere pacchetti messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * i numeri sono i valori decimali dell'equivalente ascii
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
 * -4: -2: in ascii indica "End of Trasmission"
 */


/***************************************************************/
/*Funzioni per creare la struttura delle room nel file-sistem  */

/*
 * Dir-Server/
 *  |---Dir-chat-Name 1
 *  |   |--- ...
 *  |
 *  |---Dir-chat-Name 2
 *  |   |--- ...
 *  |
 *  |---Dir-chat-Name 3
 *  |   |--- ...
 *  |
 *  |--- File-User-Login-data
 *
 *
 * Name-File = Name-chat
 * Storico-chat : Name-chat-firstMessageData
 *
 * /"NoDir-chat-Name"\
 *  |--- File: attuale chat
 *  |---Dir-history/
 *  |   |--- File-hystory xxxx-mm-gg
 *  |   |--- File-hystory xxxx-mm-gg
 *  |   ...
 *
 *  I file in hystory sono read only,
 *  Il file attuale ha una dimensone limite di 1 MB
 *  superata tale soglia deve essere copiato e spostato nello storico
 *  e iniziato un nuovo file, ciò a impedire una dimensione eccessiva di
 *  dati da caricare in ram
 *
 */
