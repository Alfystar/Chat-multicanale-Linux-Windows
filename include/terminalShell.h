//
// Created by alfylinux on 09/09/18.
//

#ifndef CHAT_MULTILEVEL_TERMINALSHELL_H
#define CHAT_MULTILEVEL_TERMINALSHELL_H

#define _GNU_SOURCE             /* See feature_test_macros(7) */

/** screen Shell lib **/

#include <ncurses.h>    //non è rientrante, solo un thread alla volta può scrivere

/** Thread lib **/
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#include "../helpFunx.h"
#include "fileSystemUtylity.h"
#include "thFunx.h"

/** Strutture utili alla shell **/
typedef struct thShellArg_ {
	int fd_Read;
} thShellArg;


/** Global Variable **/
WINDOW *mainWindows;
WINDOW *titleW;
WINDOW *cmdW;
WINDOW *showPannel;
WINDOW *monitor; //schermo autonomo che si occupa di visualizzare i printf generati dai thread per notifica

sem_t screewWrite;

extern int fdOutP;  //pipe di uscita per lo stdOut


/** Funzioni generali e mainTh **/
void terminalShell(int [], int []);

void driverCmd(int argc, char *argv[], int *exit);

void menuHelpw(WINDOW *, int, int);


///Funzioni specifiche di comando
void windowSetUp();

void titlePrintW(WINDOW *, int, int);

void chatShowW(WINDOW *, int, int);

void userShowW(WINDOW *, int, int);

///Comandi di controllo sulle chat


/** shell th funx**/
void shellThStdout(thShellArg *);

void shellThStdErr(thShellArg *);





#endif //CHAT_MULTILEVEL_TERMINALSHELL_H
