//
// Created by alfylinux on 09/09/18.
//

#include "../include/terminalShell.h"


void terminalShell (){
	int error;
	windowSetUp ();
	sem_init (&screewWrite, 0, 1); //il semaforo può essere usato per scrivere

	/** creazione dei 3 thread che si occupano di leggere le pipe **/
	pthread_t ThSdtOut_var;
	thShellArg *arg = malloc (sizeof (thShellArg));
	arg->fd_Read = FdStdOutPipe[0];
	error = pthread_create (&ThSdtOut_var, NULL, shellThStdout, arg);
	if (error != 0){
		printErrno ("La creazione del Thread shellThStdout ha dato il seguente errore", error);
		exit (-1);
	}

	pthread_t ThSdtErr_var;
	arg = malloc (sizeof (thShellArg));
	arg->fd_Read = FdStdErrPipe[0];
	error = pthread_create (&ThSdtErr_var, NULL, shellThStdErr, arg);
	if (error != 0){
		printErrno ("La creazione del Thread shellThSdtErr ha dato il seguente errore", error);
		exit (-1);
	}

	pthread_t ThDebug_var;
	arg = malloc (sizeof (thShellArg));
	arg->fd_Read = FdDebugPipe[0];
	error = pthread_create (&ThDebug_var, NULL, shellThDebug, arg);
	if (error != 0){
		printErrno ("La creazione del Thread shellThDebug ha dato il seguente errore", error);
		exit (-1);
	}

	/** Cmd terminal **/
	char *cmdBuf[1024];
	char *sArgv[64];  //consento lo storage fino a 64 comandi
	int sArgc = 0;
	char *savePoint;
	char exit = 1;
	while (exit){
		sem_wait (&screewWrite);
		mvwprintw (cmdW, 1, 0, ">>  ");
		wclrtoeol (cmdW); //cancella tutto quello che si trova a destra sulla linea
		sem_post (&screewWrite);
		wscanw (cmdW, "%[^\n]", cmdBuf);

		/** Tokenizzazione di cmdW per creare un effetto argv[] **/
		sArgc = 0;
		sArgv[sArgc] = strtok_r (cmdBuf, " ", &savePoint);
		while (sArgv[sArgc] != NULL){
			sArgc++;
			sArgv[sArgc] = strtok_r (NULL, " ", &savePoint);
		}


		for (int i = 0; i < sArgc; i++){
			dprintf (fdDebug, "sArgv[%d]=%s\n", i, sArgv[i]);
		}

		/// interpretazione sArgv[] ed esecuzione comandi
		//il lock screen è all'interno
		driverCmd (sArgc, sArgv, &exit);

		sem_wait (&screewWrite);
		wrefresh (cmdW);
		sem_post (&screewWrite);

		memset (cmdBuf, 0, 1024);
		usleep (5000);

	}
	endwin ();
}

void driverCmd (int argc, char *argv[], int *exit){
	//la funzione deve avere il controllo esclusivo dello schermo

	if (argc >= 1){
		if (strcmp (argv[0], "q") == 0){
			*exit = 0;
			return;
		}
		if (strcmp (argv[0], "d") == 0){
			debugView = true;
			return;
		}
		if (strcmp (argv[0], "!d") == 0){
			debugView = false;
			return;
		}
		if (strcmp (argv[0], "room") == 0){
			sem_wait (&screewWrite);
			chatShowW (showPannel, 1, 0);
			sem_post (&screewWrite);
			return;
		}
		if (strcmp (argv[0], "p-avlR") == 0){
			dprintf (fdOut, "Legend id:pipe\n");
			print_avl_S (rmAvlTree_Pipe);
			//print_avl(*rmAvlTree_Pipe.avlRoot, *rmAvlTree_Pipe.avlRoot);
			return;
		}
		if (strcmp (argv[0], "user") == 0){
			sem_wait (&screewWrite);
			userShowW (showPannel, 1, 0);
			sem_post (&screewWrite);
			return;
		}
		if (strcmp (argv[0], "p-avlU") == 0){
			print_avl_S (usAvlTree_Pipe);
			return;
		}
		if (strcmp (argv[0], "sstat") == 0){
			printServStat (fdOut);
			return;
		}
		if (strcmp (argv[0], "reset") == 0){
			sem_wait (&screewWrite);
			windowSetUp ();
			sem_post (&screewWrite);
			return;
		}
	}

	if (argc >= 2) //seleziona comando
	{
		if (strcmp (argv[0], "usTab") == 0){
			nameList *user = userExist ();
			int want = idSearch (user, atoi (argv[1]));
			if (want == -1){
				dprintf (STDERR_FILENO, "ID RICHIESTO INESISTENTE\n");
				sem_wait (&screewWrite);
				mvwprintw (showPannel, 0, 0, "");
				wclrtobot (showPannel);
				mvwprintw (showPannel, 1, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				mvwprintw (showPannel, 2, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				mvwprintw (showPannel, 3, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				wrefresh (showPannel);
				sem_post (&screewWrite);
				return;
			}

			char userDir[128];
			sprintf (userDir, "./%s/%s", userDirName, user->names[want]);
			nameListFree (user);
			//dprintf(fdDebug,"sto per ottenere la openUser con userdir=%s\n",userDir);
			infoUser *info = openUser (userDir);
			if (info == 0){
				dprintf (STDERR_FILENO, "Visualizzazione tabella impossibile\n");

				return;
			}
			sem_wait (&screewWrite);
			wtabPrint (showPannel, info->tab, 0);
			sem_post (&screewWrite);
			return;
		}
		if (strcmp (argv[0], "roomTab") == 0){
			nameList *chat = chatRoomExist ();
			int want = idSearch (chat, atoi (argv[1]));
			if (want == -1){
				dprintf (STDERR_FILENO, "ID RICHIESTO INESISTENTE\n");
				sem_wait (&screewWrite);
				mvwprintw (showPannel, 0, 0, "");
				wclrtobot (showPannel);
				mvwprintw (showPannel, 1, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				mvwprintw (showPannel, 2, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				mvwprintw (showPannel, 3, 10, "[][][] ID RICHIESTO INESISTENTE [][][]");
				wrefresh (showPannel);
				sem_post (&screewWrite);
				return;
			}
			char chatDir[128];
			sprintf (chatDir, "./%s/%s", chatDirName, chat->names[want]);
			nameListFree (chat);
			infoChat *info = openRoom (chatDir);
			if (info == 0){
				dprintf (STDERR_FILENO, "Visualizzazione tabella impossibile\n");
				return;
			}
			dprintf (fdOut, "path=%s\n", chatDir);
			sem_wait (&screewWrite);
			wtabPrint (showPannel, info->tab, 0);
			sem_post (&screewWrite);
			return;
		}
		if (strcmp (argv[0], "roomConv") == 0){
			nameList *chat = chatRoomExist ();
			int want = idSearch (chat, atoi (argv[1]));
			if (want == -1){
				dprintf (STDERR_FILENO, "Id richiesto inesistente\n");
				return;
			}
			char chatDir[128];
			sprintf (chatDir, "./%s/%s", chatDirName, chat->names[want]);
			nameListFree (chat);
			infoChat *info = openRoom (chatDir);
			if (info == 0){
				dprintf (STDERR_FILENO, "Visualizzazione tabella impossibile\n");
				return;
			}
			dprintf (fdOut, "path=%s\n", chatDir);
			sem_wait (&screewWrite);
			wprintConv (showPannel, info->conv, 0);
			sem_post (&screewWrite);

			return;
		}
	}
	dprintf (STDERR_FILENO, "Coomand not Reconize\n");
	sem_wait (&screewWrite);
	menuHelpw (cmdW, 2, 0, argc, argv);
	sem_post (&screewWrite);
}

void menuHelpw (WINDOW *w, int y_start, int x_start, int argc, char *argv[]){
	mvwprintw (w, y_start, x_start, "elenco comandi disponibili\n");
	wclrtobot (w);
	wprintw (w, "## <command> [argv] ...\n");
	if (argc <= 1){
		wprintw (w, "->h <sys>\t-> Visualizza comandi di sistema\n");
		wprintw (w, "->h <user>\t-> Visualizza comandi per gli utenti\n");
		wprintw (w, "->h <room>\t-> Visualizza comandi per le room\n");
		return;
	}
	if (argc >= 2){
		if (strcmp (argv[1], "sys") == 0){
			wprintw (w, "\t(0)arg\n");
			wprintw (w, "->h\t-> Visualizza lista classi di help\n");
			wprintw (w, "->q\t-> termina server\n");
			wprintw (w, "->d\t-> Attiva visualizzazione DebugMonitor\n");
			wprintw (w, "->!d\t-> Disattiva visualizzazione DebugMonitor\n");
			wprintw (w, "->sstat\t-> Mostra servStat sul monitor\n");
			wprintw (w, "->reset\t-> restart Interface\n");
			//wprintw(w, "\t(1)arg\n");
			//wprintw(w, "\t(2)arg\n");
			return;
		}
		if (strcmp (argv[1], "user") == 0){
			wprintw (w, "\t(0)arg\n");
			wprintw (w, "->user\t-> Utenti Registrati\n");
			wprintw (w, "->p-avlU\t-> Printa l'avl degli User\n");

			wprintw (w, "\t(1)arg\n");
			wprintw (w, "->usTab [Us id]\t\t-> Visualizza tabella user\n");

			//wprintw(w, "\t(2)arg\n");
			return;
		}
		if (strcmp (argv[1], "room") == 0){
			wprintw (w, "\t(0)arg\n");
			wprintw (w, "->room\t-> Chat Archiviate\n");
			wprintw (w, "->p-avlR\t-> Printa l'avl delle chat\n");

			wprintw (w, "\t(1)arg\n");
			wprintw (w, "->roomTab [Rm id]\t-> Visualizza tabella Room\n");
			wprintw (w, "->roomConv [Rm id]\t-> Visualizza conversazione Room\n");
			//wprintw(w, "\t(2)arg\n");
			return;
		}
	}
}

void windowSetUp (){
	mainWindows = initscr ();    //è lo sfondo, scrivere su di lui i commenti perpetui
	//titleW = newwin (10, 80, 1, 1);
	cmdW = newwin (15, 80, 2, 1);
	showPannel = newwin (46, 60, 1, 83);
	monitor = newwin (28, 80, 19, 1);
	curs_set (0); //disattivo il cursore così il movimento causato dai thread non si nota
	refresh();

	start_color ();
	init_pair (Titoli, COLOR_BLUE, COLOR_GREEN);
	init_pair (Comandi, COLOR_BLACK, COLOR_WHITE);
	init_pair (ViewPan, COLOR_RED, COLOR_WHITE);
	init_pair (StdoutPrint, COLOR_GREEN, COLOR_BLUE);
	init_pair (ErrorPrint, COLOR_RED, COLOR_BLACK);
	init_pair (DebugPrint, COLOR_MAGENTA, COLOR_BLUE);

	/** Main windows print **/
	mvprintw (LINES - 1, 1, "Versione del server: %s\tPort-Proces: %d\tServerStore :%s", firmwareVersion, portProces,
	          storagePathServer);
	refresh();

	/** Finestra Titolo setUp **/
	wbkgd (titleW, COLOR_PAIR(Titoli));
	box(titleW, 0, 0);    //sovrascrive il testo
	titlePrintW (titleW, 1, 13);
	wrefresh (titleW);

	/** Finestra dei comandi setUp **/
	wbkgd (cmdW, COLOR_PAIR(Comandi));
	attron(COLOR_PAIR (Comandi));

	mvprintw (cmdW->_begy - 1, cmdW->_begx,
	          "-------------------------------Finestra di comando------------------------------");
	wrefresh (cmdW);

	/** Finestra di Visualizzazione setUp **/
	wbkgd (showPannel, COLOR_PAIR(ViewPan));
	wrefresh (showPannel);

	/** STDOUT, STDERR and Debug windows setUp**/
	wbkgd (monitor, COLOR_PAIR(StdoutPrint));
	//box(monitor, ' ', 0);
	attron(COLOR_PAIR (StdoutPrint));
	mvprintw (monitor->_begy - 1, monitor->_begx,
	          "---------------------------StdOut, StdErr & Debug Print-------------------------");
	mvwprintw (monitor, 1, 0, "");
	scrollok (monitor, TRUE);
	wrefresh (monitor);

	refresh();
}

void titlePrintW (WINDOW *w, int y_start, int x_start){
	mvwprintw (w, y_start, x_start, " _   _                       ___  ___                 ");
	mvwprintw (w, y_start + 1, x_start, "| | | |                      |  \\/  |                 ");
	mvwprintw (w, y_start + 2, x_start, "| |_| | ___  _ __ ___   ___  | .  . | ___ _ __  _   _ ");
	mvwprintw (w, y_start + 3, x_start, "|  _  |/ _ \\| '_ ` _ \\ / _ \\ | |\\/| |/ _ \\ '_ \\| | | |");
	mvwprintw (w, y_start + 4, x_start, "| | | | (_) | | | | | |  __/ | |  | |  __/ | | | |_| |");
	mvwprintw (w, y_start + 5, x_start, "\\_| |_/\\___/|_| |_| |_|\\___| \\_|  |_/\\___|_| |_|\\__,_|");
}

void chatShowW (WINDOW *w, int y_start, int x_start){
	mvwprintw (w, y_start, x_start, "Sul Server sono attualmente presenti i seguenti gruppi:\n");
	nameList *chats = chatRoomExist ();
	for (int i = 0; i < chats->nMemb; i++){
		wprintw (w, "[%d]\t|--%s\n", i, chats->names[i]);
	}
	nameListFree (chats);
	wclrtobot (w);
	wrefresh (w);
}

void userShowW (WINDOW *w, int y_start, int x_start){
	mvwprintw (w, y_start, x_start, "Sul Server sono attualmente Iscritti gli utenti:\n");
	nameList *user = userExist ();
	for (int i = 0; i < user->nMemb; i++){
		wprintw (w, "[%d]\t|--%s\n", i, user->names[i]);
	}
	nameListFree (user);
	wclrtobot (w);
	wrefresh (w);
}


///Comandi di visualizzazione chat e user

void wtabPrint (WINDOW *w, table *t, int y_start){
	struct stat tabInfo;
	fstat (fileno (t->stream), &tabInfo);
	if (tabInfo.st_size == 0){
		dprintf (STDERR_FILENO, "File Vuoto, o Inesistente\n");
		return;
	}

	size_t lenFile = lenTabF (t->stream);

	mvwprintw (w, y_start, 0, "");
	wclrtobot (w);
	wprintw (w, "La tabella ha le seguenti caratteristiche:\n");
	wprintw (w, "size=%d\n", tabInfo.st_size);
	wprintw (w, "lenFile=%d\tlenFirst=%d\n", lenFile, t->head.len);
	wprintw (w, "sizeof(entry)=%d\tsizeof(firstFree)=%d\n", sizeof (entry), sizeof (firstFree));
	wprintw (w, "\n[][]La tabella contenuta nel file contiene:[][]\n\n");
	wfirstPrint (w, &t->head);
	wprintw (w, "##########\n\n");
	for (int i = 0; i < t->head.len; i++){
		wprintw (w, "--->entry[%d]:", i);
		wentryPrint (w, (&t->data[i]));
		wprintw (w, "**********\n");
	}
	wrefresh (w);
}

void wfirstPrint (WINDOW *w, firstFree *f){
	wprintw (w, "#1\tfirstFree data Store:\n");
	wprintw (w, "name\t\t-> %s\n", f->name);
	wprintw (w, "couterFree\t-> %d\n", f->counter);
	wprintw (w, "Len\t\t-> %d\n", f->len);
	wprintw (w, "nextFree\t-> %d\n", f->nf_id);
}

void wentryPrint (WINDOW *w, entry *e){
	wprintw (w, "Entry data Store:\n?-Last-Free -> %s\tempty  -> %s\nname\t\t-> %s\npoint\t\t-> %d\n",
	         booleanPrint (isLastEntry (e)), booleanPrint (isEmptyEntry (e)), e->name, e->point);
}

void wprintConv (WINDOW *w, conversation *c, int y_start){
	mvwprintw (w, y_start, 0, "");
	wclrtobot (w);
	wprintw (w, "La Conversazione ha salvati i seguenti messaggi:\n");
	wprintw (w, "sizeof(mex)=%d\tsizeof(mexInfo)=%d\tsizeof(convInfo)=%d\n", sizeof (mex), sizeof (mexInfo),
	         sizeof (convInfo));
	wprintw (w, "FILE stream pointer\t-> %p\n", c->stream);
	wprintw (w, "\n\t[][]La Conversazione è:[][]\n\n");
	wprintConvInfo (w, &c->head);

	wprintw (w, "##########\n\n");

	for (int i = 0; i < c->head.nMex; i++){
		wprintw (w, "--->Mex[%d]:\n", i);
		wprintMex (w, c->mexList[i]);
		wprintw (w, "**********\n");
	}
	wrefresh (w);
}

void wprintMex (WINDOW *w, mex *m){
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	wprintw (w, "Mex data Store locate in=%p:\n", m);
	wprintw (w, "info.usId\t-> %d\n", m->info.usId);
	wprintw (w, "time Message\t-> %s", timeString (m->info.timeM));
	if (m->text != NULL){
		wprintw (w, "Text:\n-->  %s\n", m->text);
	}
	else{
		wprintw (w, "Text: ##Non Presente##\n");
	}
}

void wprintConvInfo (WINDOW *w, convInfo *cI){
	/*
	cI->nMex
	cI->adminId
	cI->timeCreate
	*/
	wprintw (w, "#1\tConversation info data Store:\n");
	wprintw (w, "nMess\t\t-> %d\n", cI->nMex);
	nameList *user = userExist ();
	char *who;
	int want = idSearch (user, cI->adminId);
	if (want == -1){
		who = "Non più presente";
	}
	else{
		who = user->names[want];
	}
	wprintw (w, "adminId\t\t-> %d [%s]\n", cI->adminId, who);
	wprintw (w, "Time Creat\t-> %s\n", timeString (cI->timeCreate));
	nameListFree (user);
}

/** shell th funx**/

void shellThStdout (thShellArg *info){
	sleep (1);       //piccolo ritardo per permettere la sincronizzazione della main windows e dei thread
	sem_wait (&screewWrite);
	wattron(monitor, COLOR_PAIR (StdoutPrint));
	wprintw (monitor, "Monitor attivo\n");
	wrefresh (monitor);
	sem_post (&screewWrite);
	char fdBuf[PIPE_BUF];
	memset (fdBuf, 0, PIPE_BUF);
	int byteRead;
	while (1){
		byteRead = read (info->fd_Read, fdBuf, PIPE_BUF);
		sem_wait (&screewWrite);
		if (byteRead == -1){
			wattron(monitor, COLOR_PAIR (ErrorPrint));
			mvwprintw (monitor, 2, 1, "byteRead error %s\n", strerror (byteRead));
		}
		else{
			fdBuf[byteRead] = '\0';
			wattron(monitor, COLOR_PAIR (StdoutPrint));
			wprintw (monitor, "%s", fdBuf);
		}
		wclrtobot (monitor);
		wrefresh (monitor);
		sem_post (&screewWrite);
	}
	free (info);
}

void shellThStdErr (thShellArg *info){
	sleep (1);       //piccolo ritardo per permettere la sincronizzazione della main windows e dei thread
	sem_wait (&screewWrite);
	wattron(monitor, COLOR_PAIR (ErrorPrint));
	wprintw (monitor, "Monitor d'errore attivo\n");
	wrefresh (monitor);
	sem_post (&screewWrite);
	char fdBuf[PIPE_BUF];
	memset (fdBuf, 0, PIPE_BUF);
	int byteRead;
	while (1){
		byteRead = read (info->fd_Read, fdBuf, PIPE_BUF);
		sem_wait (&screewWrite);
		if (byteRead == -1){
			wattron(monitor, COLOR_PAIR (ErrorPrint));
			mvwprintw (monitor, 2, 1, "byteRead error %s\n", strerror (byteRead));
		}
		else{
			fdBuf[byteRead] = '\0';
			wattron(monitor, COLOR_PAIR (ErrorPrint));
			wprintw (monitor, "%s", fdBuf);
		}
		wclrtobot (monitor);
		wrefresh (monitor);
		sem_post (&screewWrite);
	}
	free (info);
}

void shellThDebug (thShellArg *info){
	sleep (1);       //piccolo ritardo per permettere la sincronizzazione della main windows e dei thread
	sem_wait (&screewWrite);
	debugView = true;
	wattron(monitor, COLOR_PAIR (DebugPrint));
	wprintw (monitor, "Monitor debug attivo\n");
	wrefresh (monitor);
	sem_post (&screewWrite);
	char fdBuf[PIPE_BUF];
	memset (fdBuf, 0, PIPE_BUF);
	int byteRead;
	while (1){
		byteRead = read (info->fd_Read, fdBuf, PIPE_BUF);
		sem_wait (&screewWrite);
		if (byteRead == -1){
			wattron(monitor, COLOR_PAIR (ErrorPrint));
			mvwprintw (monitor, 2, 1, "byteRead error %s\n", strerror (byteRead));
		}
		else{
			fdBuf[byteRead] = '\0';
			wattron(monitor, COLOR_PAIR (DebugPrint));
			if (debugView) wprintw (monitor, "%s", fdBuf);
		}
		wclrtobot (monitor);
		wrefresh (monitor);
		sem_post (&screewWrite);
	}
	free (info);
}