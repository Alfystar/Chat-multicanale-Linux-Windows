//
// Created by alfylinux on 09/09/18.
//

#include "terminalShell.h"


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
	char *buf[4096];
	char exit = 1;
	while (exit) {
		sem_wait(&screewWrite);
		mvwprintw(cmdW, 1, 0, ">>  ");
		wclrtoeol(cmdW); //cancella tutto quello che si trova a destra sulla linea
		sem_post(&screewWrite);
		wscanw(cmdW, "%[^\n]", buf);

		sem_wait(&screewWrite);

		if (strcmp(buf, "help") == 0 || strcmp(buf, "-h") == 0) menuHelpw(cmdW, 2, 0);
		else if ((strcmp(buf, "quit") == 0 || strcmp(buf, "-q") == 0)) exit = 0;
		else if ((strcmp(buf, "chatRoom") == 0 || strcmp(buf, "-cr") == 0)) chatShowW(showPannel, 1, 0);
		else if ((strcmp(buf, "UserRegister") == 0 || strcmp(buf, "-ur") == 0)) userShowW(showPannel, 1, 0);
		else if ((strcmp(buf, "testPipe1") == 0 || strcmp(buf, "tp1") == 0))
			write(fdStdOutPipe[1], "Test della pipe\n", 128);
		else if ((strcmp(buf, "testPipe2") == 0 || strcmp(buf, "tp2") == 0))
			write(fdStdOutPipe[1], "Test della pipe numero 2\n", 128);
		else {
			mvwprintw(cmdW, 2, 0, buf);
			wclrtobot(cmdW);
			dprintf(fdStdOutPipe[1], "%s\n", buf);
		}
		wrefresh(cmdW);
		sem_post(&screewWrite);
		buf[0] = 0;
		usleep(5000);

	}
	endwin();
}

void shellThStdout(thShellArg *info) {
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


void windowSetUp() {
	mainWindows = initscr();    //è lo sfondo, scrivere su di lui i commenti perpetui
	titleW = newwin(10, 60, 1, 1);
	cmdW = newwin(20, 60, 12, 1);
	showPannel = newwin(43, 60, 1, 63);
	monitor = newwin(10, 60, 34, 1);
	curs_set(0); //disattivo il cursore così il movimento causato dai thread non si nota
	refresh();

	start_color();
	init_pair(Titoli, COLOR_BLUE, COLOR_GREEN);
	init_pair(Comandi, COLOR_BLACK, COLOR_WHITE);
	init_pair(ViewPan, COLOR_RED, COLOR_WHITE);
	init_pair(StdoutPrint, COLOR_GREEN, COLOR_BLUE);
	init_pair(ErrorPrint, COLOR_RED, COLOR_BLACK);


	/** Main windows print **/
	mvprintw(LINES - 3, 1, "Versione del server: %s", firmwareVersion);
	refresh();

	/** Finestra Titolo setUp **/
	wbkgd(titleW, COLOR_PAIR(Titoli));
	box(titleW, 0, 0);    //sovrascrive il testo
	titlePrintW(titleW, 1, 3);
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
	mvprintw(monitor->_begy - 1, monitor->_begx, "--------------------StdOut & StdErr Print-------------------");
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

void menuHelpw(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, y_start, x_start, "-h\thelp\t-> elenco comandi disponibili");
	mvwprintw(w, y_start + 1, x_start, "-q\tquit\t-> termina server");
	mvwprintw(w, y_start + 2, x_start, "-cr\tchatRoom\t-> Chat Archiviate");
	mvwprintw(w, y_start + 3, x_start, "-ur\tUserRegister\t-> Utenti Registrati");

}

void chatShowW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, 1, 0, "Sul Server sono attualmente presenti i seguenti gruppi:");
	char **chats = chatRoomExist();
	char **chatStartPoint = chats;
	int i = 1;
	for (; *chats != NULL; chats++) {
		mvwprintw(w, y_start + i, x_start, "[%d]\t|--%s\n", i, *chats);
		i++;
	}
	freeDublePointerArr(chatStartPoint, sizeof(chatStartPoint));
	wclrtobot(w);
	wrefresh(w);
}

void userShowW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, 1, 0, "Sul Server sono attualmente Iscritti:");
	char **user = UserDefine();
	char **userStartPoint = user;
	int i = 1;
	for (; *user != NULL; user++) {
		mvwprintw(w, y_start + i, x_start, "[%d]\t|--%s\n", i, *user);
		i++;
	}
	freeDublePointerArr(userStartPoint, sizeof(userStartPoint));
	wclrtobot(w);
	wrefresh(w);
}