//
// Created by alfylinux on 06/07/18.
//

#ifndef CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H
#define CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

#include "../helpFunx.h"
#include "../defineSets.h"
#include "tableFile.h"
#include "mexData.h"


typedef struct nameList {
	int nMemb;
	char **names;
} nameList;

typedef struct serverStatFile_ {
	long idKeyUser;
	long idKeyChat;
	char serverTimeCreate[64];
	char firmware_V[64];
} serverStatFile;

typedef struct serverStat_ {
	serverStatFile statFile;
	int fd;             //quando chiuso a -2, -1 è per gli errori
	sem_t lock;

} serverStat;

typedef struct infoChat_ {
	table *tab;
	conversation *conv;
	char myName[128];
	int fdTemp;
} infoChat;

typedef struct infoUser_ {
	table *tab;
	char myName[128];
} infoUser;

///GLOBAL VARIABLE
extern char **environ;
extern int fdOutP;  //pipe di uscita per lo stdOut

serverStat serStat;

/** PROTOTIPI   **/
///Funzioni di avvio e terminazione Server
//all'avvio del server esso inizializza il sistema, le directory ecc..
int StartServerStorage(char *storage_name);

///Funzioni di per operare sulle chat
infoChat *newRoom(char *name, int adminId);

infoChat *openRoom(char *pathDir);

infoUser *newUser(char *name);

infoUser *openUser(char *pathDir);

int lockDirFile(char *pathDir);


///Funzioni di supporto al file conf
int creatServerStatConf();

int overrideServerStatConf();

void printFcntlFile(int fd);

void printServStat(int FdOut);

int serStat_addUs_lock();

long readSerStat_idKeyUser_lock();

int serStat_addchat_lock();

long readSerStat_idKeyChat_lock();


///Funzioni di scan della directory
/*Metodi per operare sul database lato fileSystem*/
nameList *chatRoomExist();

nameList *userExist();
nameList *allDir();
void nameListFree(nameList *nl);

///Funzioni per filtrare gli elementi
/*scandir permette di filtrare i file, mettendo nella lista solo quelli che ritornano !=0
 * Di seguito tutte le funzioni create per i vari filtri
 */
int filterDirChat(const struct dirent *entry);

int filterDir(const struct dirent *entry);

int filterDirAndFile(const struct dirent *entry);

char *fileType(unsigned char d_type, char *externalBuf, int bufLen);

///Funzioni per visualizzare gli elementi
void nameListPrint(nameList *nameList);

void infoChatPrint(infoChat *info);


#endif //CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H


/*Funzioni per creare la struttura delle room nel file-sistem  */

/*
 * Dentro il server le cartelle contengono le nameList => una dir è una nameList
 *
 * Dir-Server/
 *  |---Dir-ROOM
 *  |   |---Dir-chat-Name 1
 *  |   |   |--- ...
 *  |   |
 *  |   |---Dir-chat-Name 2
 *  |   |   |--- ...
 *  |   |
 *  |   |---Dir-chat-Name 3
 *  |   |   |--- ...
 *  |
 *  |---Dir-USER
 *  |   |--- Table-User-Login-data 1
 *  |   |--- Table-User-Login-data 2
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
