//
// Created by alfylinux on 09/09/18.
//

#include "../include/terminalShell.h"


void terminalShell(int fdStdOutPipe[], int fdStdErrPipe[]) {
	int error;
	windowSetUp();
	sem_init(&screewWrite, 0, 1); //il semaforo può essere usato per scrivere

	/** creazione dei 2 thread che si occupano di leggere le pipe **/
	pthread_t ThSdtOut_var;
	thShellArg *arg = malloc(sizeof(thShellArg));
	arg->fd_Read = fdStdOutPipe[0];
	error = pthread_create(&ThSdtOut_var, NULL, shellThStdout, arg);
	if (error != 0) {
		printErrno("La creazione del Thread shellThStdout ha dato il seguente errore", error);
		exit(-1);
	}
	pthread_t ThSdtErr_var;
	arg = malloc(sizeof(thShellArg));
	arg->fd_Read = fdStdErrPipe[0];
	error = pthread_create(&ThSdtErr_var, NULL, shellThStdErr, arg);
	if (error != 0) {
		printErrno("La creazione del Thread shellThSdtErr ha dato il seguente errore", error);
		exit(-1);
	}

	/** Cmd terminal **/
	char *cmdBuf[1024];
	char *sArgv[64];  //consento lo storage fino a 64 comandi
	int sArgc = 0;
	char *savePoint;
	char exit = 1;
	while (exit) {
		sem_wait(&screewWrite);
		mvwprintw(cmdW, 1, 0, ">>  ");
		wclrtoeol(cmdW); //cancella tutto quello che si trova a destra sulla linea
		sem_post(&screewWrite);
		wscanw(cmdW, "%[^\n]", cmdBuf);

		/** Tokenizzazione di cmdW per creare un effetto argv[] **/
		sArgc = 0;
		sArgv[sArgc] = strtok_r(cmdBuf, " ", &savePoint);
		while (sArgv[sArgc] != NULL) {
			sArgc++;
			sArgv[sArgc] = strtok_r(NULL, " ", &savePoint);
		}


		for (int i = 0; i < sArgc; i++) {
			dprintf(fdOutP, "sArgv[%d]=%s\n", i, sArgv[i]);
		}

		/// interpretazione sArgv[] ed esecuzione comandi

		driverCmd(sArgc, sArgv, &exit);
		sem_wait(&screewWrite);
		wrefresh(cmdW);
		sem_post(&screewWrite);

		memset(cmdBuf, 0, 1024);
		usleep(5000);

	}
	endwin();
}

void driverCmd(int argc, char *argv[], int *exit) {
	if (argc >= 1) {
		if (strcmp(argv[0], "q") == 0) {
			*exit = 0;
			return;
		}
		if (strcmp(argv[0], "chat") == 0) {
			sem_wait(&screewWrite);
			chatShowW(showPannel, 1, 0);
			sem_post(&screewWrite);
			return;
		}
		if (strcmp(argv[0], "user") == 0) {
			sem_wait(&screewWrite);
			userShowW(showPannel, 1, 0);
			sem_post(&screewWrite);
			return;
		}
		if (strcmp(argv[0], "svst") == 0) {
			printServStat(fdOutP);
			return;
		}

	}

	if (argc >= 2) //seleziona comando
	{
		if (strcmp(argv[0], "mkRm") == 0) {
			//todo make roomchat
			infoChat *info = newRoom(argv[1], 0);
			if (info == 0) {
				dprintf(STDERR_FILENO, "creazione della chat impossibile");
				return;
			}

			return;
		}
		if (strcmp(argv[0], "mkUs") == 0) {
			//todo make new user
			return;
		}
	}

	menuHelpw(cmdW, 2, 0);
}

void menuHelpw(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, y_start, x_start, "elenco comandi disponibili\n");
	wprintw(w, "## <command> [argv] ...\n");
	wprintw(w, "\t(0)arg\n");
	wprintw(w, "->h\t-> Visualizza questa lista\n");
	wprintw(w, "->q\t-> termina server\n");
	wprintw(w, "->chat\t-> Chat Archiviate\n");
	wprintw(w, "->user\t-> Utenti Registrati\n");
	wprintw(w, "->svst\t-> Mostra servStat sul monitor\n");
	wprintw(w, "\t(1)arg\n");
	wprintw(w, "->mkRm [name]\t-> Crea una nuova stanza chiamata [name]\n");
	wprintw(w, "->mkUs [name]\t-> Crea un nuovo user chiamato [name]\n");
}


void windowSetUp() {
	mainWindows = initscr();    //è lo sfondo, scrivere su di lui i commenti perpetui
	titleW = newwin(10, 80, 1, 1);
	cmdW = newwin(20, 80, 12, 1);
	showPannel = newwin(46, 60, 1, 83);
	monitor = newwin(13, 80, 34, 1);
	curs_set(0); //disattivo il cursore così il movimento causato dai thread non si nota
	refresh();

	start_color();
	init_pair(Titoli, COLOR_BLUE, COLOR_GREEN);
	init_pair(Comandi, COLOR_BLACK, COLOR_WHITE);
	init_pair(ViewPan, COLOR_RED, COLOR_WHITE);
	init_pair(StdoutPrint, COLOR_GREEN, COLOR_BLUE);
	init_pair(ErrorPrint, COLOR_RED, COLOR_BLACK);


	/** Main windows print **/
	mvprintw(LINES - 1, 1, "Versione del server: %s", firmwareVersion);
	refresh();

	/** Finestra Titolo setUp **/
	wbkgd(titleW, COLOR_PAIR(Titoli));
	box(titleW, 0, 0);    //sovrascrive il testo
	titlePrintW(titleW, 1, 13);
	wrefresh(titleW);

	/** Finestra dei comandi setUp **/
	wbkgd(cmdW, COLOR_PAIR(Comandi));
	//box(cmdW, ' ', '#');    //sovrascrive il testo
	mvprintw(cmdW->_begy - 1, cmdW->_begx, "---------------------Finestra di comando--------------------");

	wrefresh(cmdW);

	/** Finestra di Visualizzazione setUp **/
	wbkgd(showPannel, COLOR_PAIR(ViewPan));
	wrefresh(showPannel);

	/** STDOUT and STDERR windows setUp**/
	wbkgd(monitor, COLOR_PAIR(StdoutPrint));
	//box(monitor, ' ', 0);
	attron(COLOR_PAIR(StdoutPrint));
	mvprintw(monitor->_begy - 1, monitor->_begx,
	         "------------------------------StdOut & StdErr Print-----------------------------");
	mvwprintw(monitor, 1, 0, "");
	scrollok(monitor, TRUE);
	wrefresh(monitor);

	refresh();
}

void titlePrintW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, y_start, x_start, " _   _                       ___  ___                 ");
	mvwprintw(w, y_start + 1, x_start, "| | | |                      |  \\/  |                 ");
	mvwprintw(w, y_start + 2, x_start, "| |_| | ___  _ __ ___   ___  | .  . | ___ _ __  _   _ ");
	mvwprintw(w, y_start + 3, x_start, "|  _  |/ _ \\| '_ ` _ \\ / _ \\ | |\\/| |/ _ \\ '_ \\| | | |");
	mvwprintw(w, y_start + 4, x_start, "| | | | (_) | | | | | |  __/ | |  | |  __/ | | | |_| |");
	mvwprintw(w, y_start + 5, x_start, "\\_| |_/\\___/|_| |_| |_|\\___| \\_|  |_/\\___|_| |_|\\__,_|");
}



void chatShowW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, 1, 0, "Sul Server sono attualmente presenti i seguenti gruppi:\n");
	nameList *chats = chatRoomExist();

	for (int i = 0; i < chats->nMemb; i++) {
		wprintw(w, "[%d]\t|--%s\n", i, chats->names[i]);
	}
	nameListFree(chats);
	wclrtobot(w);
	wrefresh(w);
}

//todo verificare la funzione dopo aver creato user con il metodo corretto
void userShowW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, 1, 0, "Sul Server sono attualmente Iscritti:");
	nameList *user = UserExist();

	for (int i = 0; i < user->nMemb; i++) {
		mvwprintw(w, y_start + i, x_start, "[%d]\t|--%s\n", i + 1, user->names);
	}
	nameListFree(user);
	wclrtobot(w);
	wrefresh(w);
}


///Comandi di controllo sulle chat

//todo new room
void newRoomCmd(char *name) {

}
//todo print tab e conv rom

//todo new user
//todo print tab


/** shell th funx**/

void shellThStdout(thShellArg *info) {
	sleep(1);       //piccolo ritardo per permettere la sincronizzazione della main windows e dei thread
	sem_wait(&screewWrite);
	wattron(monitor, COLOR_PAIR(StdoutPrint));
	wprintw(monitor, "Monitor attivo\n");
	wrefresh(monitor);
	sem_post(&screewWrite);
	char fdBuf[PIPE_BUF];
	memset(fdBuf, 0, PIPE_BUF);
	int byteRead;
	while (1) {
		byteRead = read(info->fd_Read, fdBuf, PIPE_BUF);
		sem_wait(&screewWrite);
		if (byteRead == -1) {
			wattron(monitor, COLOR_PAIR(ErrorPrint));
			mvwprintw(monitor, 2, 1, "byteRead error %s\n", strerror(byteRead));
		} else {
			fdBuf[byteRead] = '\0';
			wattron(monitor, COLOR_PAIR(StdoutPrint));
			wprintw(monitor, "%s", fdBuf);
		}
		wclrtobot(monitor);
		wrefresh(monitor);
		sem_post(&screewWrite);
	}
}

void shellThStdErr(thShellArg *info) {
	sleep(1);       //piccolo ritardo per permettere la sincronizzazione della main windows e dei thread
	sem_wait(&screewWrite);
	wattron(monitor, COLOR_PAIR(ErrorPrint));
	wprintw(monitor, "Monitor d'errore attivo\n");
	wrefresh(monitor);
	sem_post(&screewWrite);
	char fdBuf[PIPE_BUF];
	memset(fdBuf, 0, PIPE_BUF);
	int byteRead;
	while (1) {
		byteRead = read(info->fd_Read, fdBuf, PIPE_BUF);
		sem_wait(&screewWrite);
		if (byteRead == -1) {
			wattron(monitor, COLOR_PAIR(ErrorPrint));
			mvwprintw(monitor, 2, 1, "byteRead error %s\n", strerror(byteRead));
		} else {
			fdBuf[byteRead] = '\0';
			wattron(monitor, COLOR_PAIR(ErrorPrint));
			wprintw(monitor, "%s", fdBuf);
		}
		wclrtobot(monitor);
		wrefresh(monitor);
		sem_post(&screewWrite);
	}
}
