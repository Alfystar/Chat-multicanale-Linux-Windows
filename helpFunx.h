//
// Created by alfylinux on 06/07/18.
// Funzioni ricorrenti utili in tutto il programma
//

#ifndef CHAT_MULTILEVEL_HELPFUNX_H
#define CHAT_MULTILEVEL_HELPFUNX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

void printErrno(char *, int);

void printAllEnv();

void printDublePointerList(char **);

void freeDublePointerArr(void **, int);


#endif //CHAT_MULTILEVEL_HELPFUNX_H
