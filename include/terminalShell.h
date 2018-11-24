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
#include "../globalSet.h"

/** Strutture utili alla shell **/
typedef struct thShellArg_{
	int fd_Read;
} thShellArg;
/** Global Variable **/
WINDOW *mainWindows;
WINDOW *titleW;
WINDOW *cmdW;
WINDOW *showPannel;
WINDOW *monitor; //schermo autonomo che si occupa di visualizzare i printf generati dai thread per notifica

sem_t screewWrite;
bool debugView; //start true in thPrintDebug

/** Extern Global **/
//extern int fdOut;  //pipe di uscita per lo stdOut


/** Funzioni generali e mainTh **/
void terminalShell ();
void driverCmd (int argc, char *argv[], int *exit);
void menuHelpw (WINDOW *, int, int, int argc, char *argv[]);
///Funzioni specifiche di comando
void windowSetUp ();
void titlePrintW (WINDOW *window, int y_start, int x_start);
void chatShowW (WINDOW *window, int y_start, int x_start);
void userShowW (WINDOW *window, int y_start, int x_start);
///Comandi di visualizzazione chat e user
void wtabPrint (WINDOW *window, table *t, int y_start);
void wfirstPrint (WINDOW *window, firstFree *f);
void wentryPrint (WINDOW *window, entry *e);
void wprintConv (WINDOW *window, conversation *c, int y_start);
void wprintMex (WINDOW *window, mex *m);
void wprintConvInfo (WINDOW *window, convInfo *cI);
/** shell th funx**/
void shellThStdout (thShellArg *);
void shellThStdErr (thShellArg *);
void shellThDebug (thShellArg *);


#endif //CHAT_MULTILEVEL_TERMINALSHELL_H
