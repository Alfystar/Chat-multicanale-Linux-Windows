//
// Created by alfylinux on 06/07/18.
//

#include "../include/fileSystemUtylity.h"


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

	int confId = open(serverConfFile, O_RDWR, 0666);
	if (confId == -1)  //sono presenti errori
	{
		printErrno("errore in open('serverStat.conf')", errno);
		if (errno == 2)//file non presente
		{
			nameList *dir = allDir();
			nameListPrint(dir);

			if (dir->nMemb == 0)
			{
				//non sono presenti cartelle di alcun tipo,la directory è quindi valida, creo il file config
				printf("La cartella non è valida, non sono presenti file o cartelle estrane\nProcedo alla creazione di serverStat.conf\n");
				creatServerStatConf(); //la funzione inizializza in ram e su hard-disc il file serverConfFile
				mkdir(userDirName, 0777);
				mkdir(chatDirName, 0777);

			} else {    //è presente altro e la cartella non è valida per inizializzare il server
				printf("La cartella non è valida e neanche validabile.\nNon si può procedere all'avvio del server\n");
				return -1;
			}
			nameListFree(dir);
		}
	} else {

		printf("La cartella era già uno storage per il server\n");
		//procedo a caricare i dati del serverConfFile
		lseek(confId, 0, SEEK_SET);
		read(confId, &serStat.statFile, sizeof(serStat.statFile));
		printf("caricamento dalla memoria eseguito\n");
		serStat.fd = confId;
		if (sem_init(&serStat.lock, 0, 1)) {
			perror("serStat semaphore init take error: ");
			return -1;
		}

	}
	printServStat(STDOUT_FILENO);

	//close(confId);    meglio lasciarlo aperto per permettere l'override del file durante la creazione di nuove chat e user per aggiornare l'id
	printf("[2]---> success\n\n");

	return 0;   //avvio con successo
}

///Funzioni di per operare sulle chat

infoChat *newRoom(char *name, int adminUs) {
	infoChat *info = malloc(sizeof(infoChat));
	if (info == NULL) {
		perror("infoChat malloc() take error: ");
		return info;
	}

	serStat_addchat_lock();
	long newId = readSerStat_idKeyChat_lock();

	///Creo la directory Chat
	char nameChat[128];
	sprintf(nameChat, "%ld:%s", newId, name);
	char chatPath[128];
	sprintf(chatPath, "./%s/%s", chatDirName, nameChat);
	if (mkdir(chatPath, 0777)) {
		switch (errno) {
			case EEXIST:
				fprintf(stderr, "chatDir already exist");
				return NULL;
			default:
				perror("makeDir chat take error :");
				return NULL;
				break;
		}
	}
	///se arrivo qui sicuramente la cartella non esisteva e posso procedere tranquillamente

	strncpy(info->myName, nameChat, 128);

	char tempFile[128];
	sprintf(tempFile, "%s%s", chatPath, "/temp");
	info->fdTemp = lockDirFile(tempFile);
	if (info->fdTemp == -2) {
		//se ritorna null la creazione del thread è da annullare!!
		return NULL;
	}
	///Creo tabella e conversazione
	char tabNamePath[128];
	sprintf(tabNamePath, "%s/%s", chatPath, chatTable);
	info->tab = init_Tab(tabNamePath, nameChat);
	char convNamePath[128];
	sprintf(convNamePath, "%s/%s", chatPath, chatConv);
	info->conv = initConv(convNamePath, adminUs);

	return info;
}

infoChat *openRoom(char *pathDir) {
	//pathDir = ./NAME_DIR_CHAT/CHAT_NAME
	infoChat *info = malloc(sizeof(infoChat));
	if (info == NULL) {
		perror("infoChat malloc() take error: ");
		return info;
	}
	//todo verificare creazione del file temporaneo
	char tempFile[128];
	sprintf(tempFile, "%s%s", pathDir, "/temp");
	info->fdTemp = lockDirFile(tempFile);
	if (info->fdTemp == -1) {
		return -1;
	}


	///Creo tabella e conversazione
	char tabNamePath[128];
	sprintf(tabNamePath, "%s/%s", pathDir, chatTable);
	info->tab = open_Tab(tabNamePath);
	char convNamePath[128];
	sprintf(convNamePath, "%s/%s", pathDir, chatConv);
	info->conv = openConf(convNamePath);

	return info;
}

//todo da testare
infoUser *newUser(char *name) {
	infoUser *info = malloc(sizeof(infoUser));
	if (info == NULL) {
		perror("infoUser malloc() take error: ");
		return info;
	}

	serStat_addUs_lock();
	long newId = readSerStat_idKeyUser_lock();

	///Creo la directory User
	char nameUser[128];
	sprintf(nameUser, "%ld:%s", newId, name);
	char userPath[128];
	sprintf(userPath, "./%s/%s", userDirName, nameUser);
	if (mkdir(userPath, 0777)) {
		switch (errno) {
			case EEXIST:
				fprintf(stderr, "userPath already exist");
				return NULL;
			default:
				perror("makeDir chat take error :");
				return NULL;
				break;
		}
	}
	///se arrivo qui sicuramente la cartella non esisteva e posso procedere tranquillamente

	strncpy(info->myName, nameUser, 128);

	///Creo tabella
	char tabNamePath[128];
	sprintf(tabNamePath, "%s/%s", userPath, userTable);
	info->tab = init_Tab(tabNamePath, nameUser);


	return info;
}

//todo da ridefinire
infoUser *openUser(char *pathDir) {
	//pathDir = ./NAME_DIR_USER/USER_DIR
	infoUser *info = malloc(sizeof(infoUser));
	if (info == NULL) {
		perror("infoUser malloc() take error: ");
		return info;
	}

	///Creo tabella
	char tabNamePath[128];
	sprintf(tabNamePath, "%s/%s", pathDir, chatTable);
	info->tab = open_Tab(tabNamePath);

	return info;
}

int lockDirFile(char *pathDir) {
	///creo un file temporaneo nella cartella, se presente un thread chat è già in funzione O_TMPFILE
	int lkFd = creat(pathDir, O_EXCL | __O_TMPFILE);
	if (lkFd == -1) {
		switch (errno) {
			case EEXIST:
				fprintf(stderr, "Thread chat just online\n");
				return -1;
				break;
			default:
				fprintf(stderr, "pathDir=%s\n", pathDir);
				perror("errore in creazione del file lock temporaneo :");
				return -1;
		}

	}
	return lkFd;
}


///Funzioni di supporto al file conf
int creatServerStatConf() {
	/**
	 * LA funzione si occupa di aprire e inizializzare il serverStatConf
	 * **/
	int validServerId = open("serverStat.conf", O_CREAT | O_RDWR | O_TRUNC, 0666);

	/*** Procedura per aggiungere ora di creazione del server ***/
	char testo[4096];

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
	serStat.statFile.idKeyChat = 0;
	serStat.statFile.idKeyUser = 0;
	strncpy(serStat.statFile.serverTimeCreate, c_time_string, 64);
	strncpy(serStat.statFile.firmware_V, firmwareVersion, 64);
	serStat.fd = validServerId;
	if (sem_init(&serStat.lock, 0, 1)) {
		perror("serStat semaphore init take error: ");
		return -1;
	}
	printServStat(STDOUT_FILENO);
	overrideServerStatConf();



	return validServerId;
}

int overrideServerStatConf() {
	//sovrascrive il file con l'attuale contenuto nella variabile
	if (serStat.fd == -2 || serStat.fd == -1) {
		fprintf(stderr, "fd not valid\n");
		return -1;
	}
	struct flock lockStatConf;

	lockStatConf.l_type = F_WRLCK;
	lockStatConf.l_whence = SEEK_SET;
	lockStatConf.l_start = 0;
	lockStatConf.l_len = 0;
	lockStatConf.l_pid = getpid();

	//acquisisco il lock in scrittura sul file, se è occupato allora attende
	if (fcntl(serStat.fd, F_SETLK, &lockStatConf))
	{
		perror("waiting lock on serverStat.conf take error:");
		return -1;
	}
	sem_wait(&serStat.lock);
	lseek(serStat.fd, 0, SEEK_SET);

	size_t byteWrite = 0;
	do {
		byteWrite += write(serStat.fd, &serStat.statFile + byteWrite, sizeof(serStat.statFile) - byteWrite);
	} while (byteWrite != sizeof(serStat.statFile));
	sem_post(&serStat.lock);
	lockStatConf.l_type = F_UNLCK;
	if (fcntl(serStat.fd, F_SETLK, &lockStatConf)) {
		//acquisisco il lock in scrittura sul file, se è occupato allora attende
		perror("waiting unlock on serverStat.conf take error:");
		return -1;
	}
	return 0;
}

void printFcntlFile(int fd) {
	///Funzione per testare il lock kernel di un certo file
	struct flock lockStatConf;
	lockStatConf.l_type = F_WRLCK;
	lockStatConf.l_whence = SEEK_SET;
	lockStatConf.l_start = 0;
	lockStatConf.l_len = 0;
	lockStatConf.l_pid = 0;
	if (fcntl(fd, F_GETLK, &lockStatConf))  //acquisisco i dati di lock del file
	{
		perror("printFcntlFile take error:");
		return;
	}
	if (lockStatConf.l_type != F_UNLCK) {
		printf("Too bad... it's already locked... by pid=%d\n", lockStatConf.l_pid);
		return;
	} else {
		printf("fd test is free from any lock\n");

	}
}

void printServStat(int fdOut) {
	dprintf(fdOut, "######### Contenuto di serverStat.conf: #########\n");
	dprintf(fdOut, "idKeyUser univoche number:\t--> %ld\n", serStat.statFile.idKeyUser);
	dprintf(fdOut, "idKeyChat univoche number:\t--> %ld\n", serStat.statFile.idKeyChat);
	dprintf(fdOut, "Time server Create: \t\t--> %s\n", serStat.statFile.serverTimeCreate);
	dprintf(fdOut, "Server Firmware version:\t--> %s\n", serStat.statFile.firmware_V);
	dprintf(fdOut, "file Descriptor position\t--> %d\n", serStat.fd);
	int semVal;
	sem_getvalue(&serStat.lock, &semVal);
	dprintf(fdOut, "lock state\t\t\t--> %d\n", semVal);

}

int serStat_addUs_lock() {
	if (sem_wait(&serStat.lock)) {
		perror("serStat sem_wait take error: ");
		return -1;
	}
	serStat.statFile.idKeyUser++;
	if (sem_post(&serStat.lock)) {
		perror("serStat sem_post take error: ");
		return -1;
	}
	overrideServerStatConf();
	return 0;
}

long readSerStat_idKeyUser_lock() {
	long read;
	if (sem_wait(&serStat.lock)) {
		perror("serStat sem_wait take error: ");
		return -1;
	}
	read = serStat.statFile.idKeyUser;
	if (sem_post(&serStat.lock)) {
		perror("serStat sem_post take error: ");
		return -1;
	}
	return read;
}

int serStat_addchat_lock() {
	if (sem_wait(&serStat.lock)) {
		perror("serStat sem_wait take error: ");
		return -1;
	}
	serStat.statFile.idKeyChat++;
	if (sem_post(&serStat.lock)) {
		perror("serStat sem_post take error: ");
		return -1;
	}
	printServStat(fdOutP);
	overrideServerStatConf();
	return 0;
}

long readSerStat_idKeyChat_lock() {
	long read;
	if (sem_wait(&serStat.lock)) {
		perror("serStat sem_wait take error: ");
		return -1;
	}
	read = serStat.statFile.idKeyChat;
	if (sem_post(&serStat.lock)) {
		perror("serStat sem_post take error: ");
		return -1;
	}
	return read;
}


///Funzioni di scan della directory

/* return **array end with NULL
 * The returned list Must be free in all entry
 */
nameList *chatRoomExist() {
	nameList *chats = malloc(sizeof(nameList));
	struct dirent **namelist;

	char home[128] = "./";

	chats->nMemb = scandir(strncat(home, chatDirName, 128), &namelist, filterDirChat, alphasort);
	if (chats->nMemb == -1) {
		perror("scan dir");
		exit(EXIT_FAILURE);
	}
	chats->names = malloc(sizeof(char *) * (chats->nMemb));
	for (int i = 0; i < chats->nMemb; i++) {
		chats->names[i] = malloc(strlen(namelist[i]->d_name));
		strcpy(chats->names[i], namelist[i]->d_name);
		free(namelist[i]);
	}

	free(namelist);
	return chats;
}

nameList *UserExist() {
	nameList *users = malloc(sizeof(nameList));
	struct dirent **namelist;

	char home[128] = "./";
	users->nMemb = scandir(strncat(home, userDirName, 128), &namelist, filterDir, alphasort);
	if (users->nMemb == -1) {
		perror("scan dir");
		exit(EXIT_FAILURE);
	}
	users->names = malloc(sizeof(char *) * (users->nMemb));
	for (int i = 0; i < users->nMemb; i++) {
		users->names[i] = malloc(strlen(namelist[i]->d_name));
		strcpy(users->names[i], namelist[i]->d_name);
		free(namelist[i]);
	}

	free(namelist);
	return users;
}

nameList *allDir() {
	nameList *dir = malloc(sizeof(nameList));
	struct dirent **namelist;

	dir->nMemb = scandir(".", &namelist, filterDirAndFile, alphasort);
	if (dir->nMemb == -1) {
		perror("scan dir");
		exit(EXIT_FAILURE);
	}
	dir->names = malloc(sizeof(char *) * (dir->nMemb));
	for (int i = 0; i < dir->nMemb; i++) {
		dir->names[i] = malloc(strlen(namelist[i]->d_name));
		strcpy(dir->names[i], namelist[i]->d_name);
		free(namelist[i]);
	}

	free(namelist);
	return dir;
}

void nameListFree(nameList *nl) {
	for (int i = 0; i < nl->nMemb; i++) {
		free(nl->names[i]);
	}
	free(nl->names);
	free(nl);
}

///Funzioni per filtrare gli elementi

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

///show funcion
void nameListPrint(nameList *nl) {
	printf("N° elem: %d\n", nl->nMemb);
	for (int i = 0; i < nl->nMemb; i++) {
		printf("%s\n", nl->names[i]);
	}
	printf("\t# End list #\n");
}

void infoChatPrint(infoChat *info) {

	/*
	 info->tab
	 info->conv
	 info->myName
	 */
	printf("########[[]][[]] infoChat contenent [[]][[]]########\n");
	tabPrint(info->tab);
	printConv(info->conv);
	printf("info->myName= %s\n", info->myName);
	printf("----------------------------------------------------\n");

}