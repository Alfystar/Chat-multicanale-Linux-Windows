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

#include "helpFunx.h"
#include "defineSets.h"

extern char **environ;

//all'avvio del server esso inizializza il sistema, le directory ecc..
int StartServerStorage(char *);

/******************* Funzioni di per operare sulle chat *******************/
int newChat(char *);

/******************* Funzioni di supporto al file conf *******************/
int creatServerStatConf();

/******************* Funzioni di scan della directory *******************/
/*Metodi per operare sul database lato fileSystem*/
char **chatRoomExist();

char **UserDefine();

char **freeDir();

/******************* Funzioni per filtrare gli elementi *******************/
/*scandir permette di filtrare i file, mettendo nella lista solo quelli che ritornano !=0
 * Di seguito tutte le funzioni create per i vari filtri
 */
int filterDirChat(const struct dirent *);

int filterDir(const struct dirent *);

int filterDirAndFile(const struct dirent *);

char *fileType(unsigned char, char *, int);

#endif //CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H
