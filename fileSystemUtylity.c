//
// Created by alfylinux on 06/07/18.
//

#include "fileSystemUtylity.h"

//TODO: File Sistem tree
/***************************************************************/
/*Funzioni per creare la struttura delle room nel file-sistem  */

/*
 * Dentro il server le cartelle contengono le chatRoom => una dir è una chatRoom
 *
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
 *  |
 *  |---Dir-USER
 *  |   |--- File-User-Login-data 1
 *  |   |--- File-User-Login-data 2
 *  |   |--- ...
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
 * /"File-User-Login-data"\
 *  Il nome utente è nel nome del file.
 *  il file inizia con la pw
 *  c'è un elenco delle white list, ovvero tutte le chat a cui sono collegato
 *  es:
 *  chat1:chat2:pippo:baudo:ecc:....
 *  con strtok possiamo trovare le singole chat
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
					exit(EXIT_FAILURE);
				} else {
					printf("New directory create\n");
					errorRet = chdir(storage);
					if (errorRet == -1) {
						printErrno("nonostante la creazione chdir()", errno);
						exit(EXIT_FAILURE);
					}
				}
				break;
			default:
				printErrno("chdir", errno);
				exit(EXIT_FAILURE);
		}
	}

	//todo: deve verificare che la cartella è una cartella valida se già esiste (magari con un file) o creare il file di validità se nuovo

	char curDir[100];
	errorRet = setenv("PWD", getcwd(curDir, 100), true);    //aggiorno l'env per il nuovo pwd
	if (errorRet != 0) printErrno("setEnv('PWD')", errorRet);
	printf("Current Directory set:\n-->\tgetcwd()=%s\n-->\tPWD=%s\n\n", curDir, getenv("PWD"));
}

/* return **array end with NULL
 * The returned list Must be free in all entry
 */
char **chatRoomExist() {
	char **chatRoom;
	struct dirent **namelist;
	struct dirent a;
	a.
	int n;
	n = scandir(".", &namelist, filterDir, alphasort);
	if (n == -1) {
		perror("scandir");
		exit(EXIT_FAILURE);
	}
	chatRoom = malloc(sizeof(char *) * (n + 1));
	for (int i = 0; i < n; i++) {
		chatRoom[i] = malloc(strlen(namelist[i]->d_name) + 1);
		strcpy(chatRoom[i], namelist[i]->d_name);
		free(namelist[i]);
	}
	chatRoom[n] = NULL;
	free(namelist);
	return chatRoom;
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

int filterDir(const struct dirent *entry) {
	if ((entry->d_type == DT_DIR) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
		return 1;
	}
	return 0;
}

char *fileType(unsigned char d_type, char *buf, int bufLen) {
	switch (d_type) {
		case DT_REG:
			strncpy(buf, "Regular File", bufLen);
			break;
		case DT_DIR:
			strncpy(buf, "Directory", bufLen);
			break;

		case DT_FIFO:
			strncpy(buf, "PIPE fifo", bufLen);
			break;

		case DT_SOCK:
			strncpy(buf, "Local Domain Soket", bufLen);
			break;

		case DT_CHR:
			strncpy(buf, "Character device", bufLen);
			break;

		case DT_BLK:
			strncpy(buf, "Block Device", bufLen);
			break;

		case DT_LNK:
			strncpy(buf, "Symbolic link", bufLen);
			break;

		default:
		case DT_UNKNOWN:
			strncpy(buf, "Unknown", bufLen);
			break;
	}
	return buf;
}

char **findChat(char *path) {


}
