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

#include "../helpFunx.h"
#include "fileSystemUtylity.h"
#include <unistd.h>
#include <fcntl.h>

/** Strutture utili alla shell **/
typedef struct thShellArg_ {
	int fd_Read;
} thShellArg;


/** Funzioni generali e mainTh **/
void terminalShell(int [], int []);

void windowSetUp();

void titlePrintW(WINDOW *, int, int);

void menuHelpw(WINDOW *, int, int);

void chatShowW(WINDOW *, int, int);

void userShowW(WINDOW *, int, int);

/** shell th funx**/
void shellThStdout(thShellArg *);

void shellThStdErr(thShellArg *);


/** Global Variable **/
WINDOW *mainWindows;
WINDOW *titleW;
WINDOW *cmdW;
WINDOW *showPannel;
WINDOW *monitor; //schermo autonomo che si occupa di visualizzare i printf generati dai thread per notifica

sem_t screewWrite;


#endif //CHAT_MULTILEVEL_TERMINALSHELL_H
