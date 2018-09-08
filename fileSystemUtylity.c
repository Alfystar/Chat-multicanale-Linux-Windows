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


int StartServerStorage(char *storage)  //apre o crea un nuovo storage per il database
{
	/* modifica il path reference dell'env per "spostare" il programma nella nuova locazione
	 * la variabile PWD contiene il path assoluto, della working directory, ed è aggiornata da una sheel
	 * MA I PROGRAMMI SCRITTI IN C usano un altra variabile per dire il proprio percorso di esecuzione.
	 * Di conseguenza bisogna prima modificare il path del processo e sucessivamente aggiornare l'env     *
	 */

	printf("[1]---> Fase 1, aprire lo storage\n");

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
					return -1;
				} else {
					printf("New directory create\n");
					errorRet = chdir(storage);
					if (errorRet == -1) {
						printErrno("nonostante la creazione chdir()", errno);
						return -1;
					}
				}
				break;
			default:
				printErrno("chdir", errno);
				return -1;
		}
	}
	char curDirPath[100];
	errorRet = setenv("PWD", getcwd(curDirPath, 100), true);    //aggiorno l'env per il nuovo pwd
	if (errorRet != 0) printErrno("setEnv('PWD')", errorRet);
	printf("Current Directory set:\n-->\tgetcwd()=%s\n-->\tPWD=%s\n\n", curDirPath, getenv("PWD"));
	printf("[1]---> success\n\n");


	/**** Si verifica che la cartella a cui si è arrivata sia già un server, e se non allora si inizializza ****/

	printf("[2]---> Fase 2 Stabilire se la cartella è Valida/Validabile/INVALIDA\n\n");

	printf("## ");

	int confId = open("serverStat.conf", O_RDWR, 0666);
	if (confId == -1)  //sono presenti errori
	{
		printErrno("errore in open('serverStat.conf')", errno);
		if (errno == 2)//file non presente
		{
			char **dirElement = freeDir();

			if (dirElement[0] ==
			    NULL) //non sono presenti cartelle di alcun tipo,la directory è quindi valida, creo il file config
			{
				printf("La cartella non è valida, non sono presenti file o cartelle estrane\nProcedo alla creazione di serverStat.conf\n");
				confId = creatServerStatConf();
				mkdir(userDirName, 0777);

			} else {    //è presente altro e la cartella non è valida per inizializzare il server
				printf("La cartella non è valida e neanche validabile.\nNon si può procedere all'avvio del server\n");
				return -1;
			}
			freeDublePointerArr(dirElement, sizeof(dirElement));
		}
	} else {

		printf("La cartella era già uno storage per il server\n");
	}

	char bufread[4096];
	lseek(confId, 0, SEEK_SET);
	int br = read(confId, bufread, 4096);

	printf("\n######### Contenuto di serverStat.conf: #########\n%s#################################################\n",
	       bufread);
	printf("Current setting:\n");
	printf("-->\tFirmware Version: %s\n", firmwareVersion);
	close(confId);
	printf("[2]---> success\n\n");

	return 0;   //avvio con successo
}

/******************* Funzioni di per operare sulle chat *******************/

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

/******************* Funzioni di supporto al file conf *******************/
int creatServerStatConf() {
	int validServerId = open("serverStat.conf", O_CREAT | O_RDWR | O_TRUNC, 0666);

	/*** Procedura per aggiungere ora di creazione del server ***/
	char testo[4096] = "Server creato in data: ";

	time_t current_time;
	char *c_time_string;

	/* Obtain current time. */
	current_time = time(NULL);

	if (current_time == ((time_t) -1)) {
		fprintf(stderr, "Failure to obtain the current time.\n");
		exit(EXIT_FAILURE);
	}

	/* Convert to local time format. */
	c_time_string = ctime(&current_time);

	if (c_time_string == NULL) {
		fprintf(stderr, "Failure to convert the current time.\n");
		exit(EXIT_FAILURE);
	}

	/* Print to stdout. ctime() has already added a terminating newline character. */
	//printf("Current time is %s", c_time_string);
	strcat(testo, c_time_string);
	strcat(testo, "Firmware Version: ");
	strcat(testo, firmwareVersion);
	strcat(testo, "\n");

	size_t lenWrite = strlen(testo);
	size_t byteWrite = 0;
	do {
		byteWrite += write(validServerId, testo + byteWrite, lenWrite - byteWrite);
	} while (byteWrite != lenWrite);

	return validServerId;
}


/******************* Funzioni di scan della directory *******************/

/* return **array end with NULL
 * The returned list Must be free in all entry
 */
char **chatRoomExist() {
	char **chatRoom;
	struct dirent **namelist;

	int n;
	n = scandir(".", &namelist, filterDirChat, alphasort);
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

char **UserDefine() {
	char **chatRoom;
	struct dirent **namelist;

	int n;
	char home[512] = "./";
	n = scandir(strcat(home, userDirName), &namelist, filterDir, alphasort);
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

char **freeDir() {
	char **dirs;
	struct dirent **namelist;

	int n;
	n = scandir(".", &namelist, filterDirAndFile, alphasort);
	if (n == -1) {
		perror("scandir");
		exit(EXIT_FAILURE);
	}
	dirs = malloc(sizeof(char *) * (n + 1));
	for (int i = 0; i < n; i++) {
		dirs[i] = malloc(strlen(namelist[i]->d_name) + 1);
		strcpy(dirs[i], namelist[i]->d_name);
		free(namelist[i]);
	}
	dirs[n] = NULL;
	free(namelist);
	return dirs;
}


/******************* Funzioni per filtrare gli elementi *******************/
int filterDirChat(const struct dirent *entry) {
	/** Visualizza qualsiasi directory escludendo la user**/
	if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
	                                  strcmp(entry->d_name, userDirName) != 0)) {
		return 1;
	}
	return 0;
}

int filterDir(const struct dirent *entry) {
	/** Visualizza qualsiasi directory**/
	if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)) {
		return 1;
	}
	return 0;
}

int filterDirAndFile(const struct dirent *entry) {
	/** visualizza se è una directory o file, ignorando . e ..**/
	if (((entry->d_type == DT_DIR) || (entry->d_type == DT_REG)) &&
	    (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)) {
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
