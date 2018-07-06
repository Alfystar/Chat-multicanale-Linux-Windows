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

#include "helpFunx.h"

extern char **environ;

//all'avvio del server esso inizializza il sistema, le directory ecc..
void StartServerStorage(char *);

/*Metodi per operare sul database lato fileSystem*/
char **chatRoomExist();

int newChat(char *);


char *fileType(unsigned char, char *, int);

/*scandir permette di filtrare i file, mettendo nella lista solo quelli che ritornano !=0
 * Di seguito tutte le funzioni create per i vari filtri
 */
int filterDir(const struct dirent *);

#endif //CHAT_MULTILEVEL_FILESYSTEMUTYLITY_H
