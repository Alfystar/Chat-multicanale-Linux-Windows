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

#include "../helpFunx.h"
#include "../defineSets.h"

extern char **environ;

typedef struct nameList {
	int nMemb;
	char **names;
} nameList;

/** PROTOTIPI   **/


//all'avvio del server esso inizializza il sistema, le directory ecc..
int StartServerStorage(char *storage_name);

///Funzioni di per operare sulle chat
int newRoom(char *name, int adminId);

///Funzioni di supporto al file conf
int creatServerStatConf();

///Funzioni di scan della directory
/*Metodi per operare sul database lato fileSystem*/
nameList *chatRoomExist();

nameList *UserExist();

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


#endif //CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H
