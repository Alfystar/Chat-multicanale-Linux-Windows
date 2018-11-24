//
// Created by alfylinux on 06/07/18.
// Funzioni ricorrenti utili in tutto il programma
//

#ifndef CHAT_MULTILEVEL_HELPFUNX_H
#define CHAT_MULTILEVEL_HELPFUNX_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fts.h>
#include <errno.h>
#include "globalSet.h"


extern char **environ;

void printErrno (char *, int);

void printAllEnv ( );

void printDublePointeChar (char **);

void freeDublePointerArr (void **, int);

int recursive_delete (const char *dir);


#endif //CHAT_MULTILEVEL_HELPFUNX_H
