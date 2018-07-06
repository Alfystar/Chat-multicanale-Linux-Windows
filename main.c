#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

extern char **environ;



void printErrno(char *, int);

void printAllEnv();

//File Sistem tree
void StartServerStorage(char *);
int newChat(char *);

int errorRet;


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("!#!#!#\tPrego inserire i dati mancanti:\n!#!#!#\t<Path-Server-Data>\n");
        return -1;
    }
    StartServerStorage(argv[1]);

    newChat("test1");
    return 0;
}

/******************************************************************/
/*                 Funzioni di appoggio e debug                   */
void printErrno(char *note, int error) {
    printf("%s\terr:%d (%s)\n", note, error, strerror(error));
}

void printAllEnv() {
    printf("\n######################################\n");
    for (char **envp = environ; *envp != 0; envp++) {
        char *thisEnv = *envp;
        printf("%s\n", thisEnv);
    }
    printf("######################################\n\n");
}


//TODO: File Sistem tree
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
 * "attuale chat" = Name-chat
 * Storico-chat : Name-chat-firstMessageData
 *
 * /"Dir-chat-Name"\
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


void StartServerStorage(char *storage)  //apre o crea un nuovo storage per il database
{
	/* modifica il path reference dell'env per "spostare" il programma nella nuova locazione
	 * la variabile PWD contiene il path assoluto, della working directory, ed è aggiornata da una sheel
	 * MA I PROGRAMMI SCRITTI IN C usano un altra variabile per dire il proprio percorso di esecuzione.
	 * Di conseguenza bisogna prima modificare il path del processo e sucessivamente aggiornare l'env     *
	 */
	int errorRet;
	errorRet = chdir(storage);                        //modifico l'attuale directory di lavoro del processo
	if (errorRet != 0)    //un qualche errore nel ragiungimento della cartella
	{
		switch (errno) {
			case 2: //No such file or directory
				printf("directory non esistente, procedo alla creazione\n");
				errorRet = mkdir(storage, 0777);
				if (errorRet == -1) {
					printErrno("mkdir fails", errno);
					exit(-1);
				} else {
					printf("New directory create\n");
					errorRet = chdir(storage);
					if (errorRet == -1) {
						printErrno("nonostante la creazione chdir()", errno);
						exit(-1);
					}
				}
				break;
			default:
				printErrno("chdir", errno);
				exit(-1);
		}
	}

	char curDir[100];
	errorRet = setenv("PWD", getcwd(curDir, 100), true);    //aggiorno l'env per il nuovo pwd
	if (errorRet != 0) printErrno("setEnv('PWD')", errorRet);
	printf("Current Directory set:\n\tgetcwd()=%s\n\tPWD=%s\n", curDir, getenv("PWD"));
}

int newChat(char *name) {
	/*
	int testId=open("testo",O_CREAT|O_RDWR|O_TRUNC,0666);
	if (testId==-1)
	{
		printErrno("errore in open('test')",errno);
		return -1;
	}
	char* testo="test\n";
	int bw=write(testId,testo,strlen(testo));
	printf("bw=%d\n",bw);
	char bufread[500];
	lseek(testId,0,SEEK_SET);
	int br=read(testId,bufread,500);
	printf("br=%d\n",br);
	printf("buf=%s\n",bufread);
	 */

	return 0;
}

char **findChat(char *path) {


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

/*TODO: creazione thread utente e funzioni di appoggio
 *
 */
