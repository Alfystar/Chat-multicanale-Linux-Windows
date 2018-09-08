#include <stdio.h>
#include <stdlib.h>

/** Thread lib **/
#include <unistd.h>
#include <pthread.h>

/** screen write lib **/
#include <ncurses.h>

#include "helpFunx.h"

#include "fileSystemUtylity.h"

#include "defineSets.h"


/** STRUTTURE & Typedef DEL MAIN **/
typedef struct thAcceptlArg_ {
	int id;
} thAcceptlArg;

typedef struct thUserlArg_ {
	int id;
} thUserlArg;


/** PROTOTIPI DEL MAIN **/
void acceptTh(thAcceptlArg *);

void userTh(thUserlArg *);

void titlePrintW(WINDOW *, int, int);



int errorRet;

pthread_t *acceptArray;

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("!#!#!#\tPrego inserire i dati mancanti:\n!#!#!#\t<Path-Server-Data>\n");
		exit(EXIT_FAILURE);
	}
	/** fase di avvio del server **/
	if (StartServerStorage(argv[1]) == -1) //errore di qualche tipo nell'avvio del server
	{
		printf("\n!! Il server non è stato in grado di essere inizializato\nEXIT_FAILURE\n");
		exit(EXIT_FAILURE);
	}

	/** Spawn dei thredh accetta user **/
	acceptArray = malloc(nAcceptTh * sizeof(pthread_t));
	for (int i = 0; i < nAcceptTh; i++) {
		thAcceptlArg *arg = malloc(sizeof(thAcceptlArg));
		arg->id = i;
		errorRet = pthread_create(&acceptArray[i], NULL, acceptTh, arg);
		if (errorRet != 0) {
			printErrno("La creazione del Thread Accept ha dato il seguente errore", errorRet);
			exit(-1);
		}
	}

	/** Visualizzazione chat già presenti all'avvio**/
	char **chats = chatRoomExist();
	printf("Current chatRoom created:\n");
	printDublePointeChar(chats);
	freeDublePointerArr(chats, sizeof(chats));

	printf("\nTerminale in avvio\n");
	usleep(500000);
	/** Il Main Thread  diventa il terminale con cui interagire da qui in poi è il terminale **/
	//printf("\n\n__________________________________________________________________\n[x][x][x][x][x][x]\tSono il terminale \t[x][x][x][x][x][x]\n");
	char *buf[4096];
	char exit = 1;

	WINDOW *mainWindows;
	WINDOW *titleW;
	WINDOW *cmdW;
	WINDOW *showPannel;

	mainWindows = initscr();
	titleW = newwin(10, 60, 1, 1);
	cmdW = newwin(20, 60, 15, 0);
	showPannel = newwin(40, 60, 1, 65);
	refresh();

	start_color();
	init_pair(Titoli, COLOR_GREEN, COLOR_BLUE);
	init_pair(Comandi, COLOR_WHITE, COLOR_BLACK);
	init_pair(ViewPan, COLOR_RED, COLOR_WHITE);

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
	box(cmdW, NULL, '#');    //sovrascrive il testo
	wrefresh(cmdW);

	/** Finestra di Visualizzazione setUp **/
	wbkgd(showPannel, COLOR_PAIR(ViewPan));
	wrefresh(showPannel);

	while (exit) {
		mvwprintw(cmdW, 1, 0, ">>  ");
		wclrtoeol(cmdW); //cancella tutto quello che si trova a destra sulla linea
		wscanw(cmdW, "%[^\n]", buf);


		if (strcmp(buf, "help") == 0 || strcmp(buf, "-h") == 0) menuHelpw(cmdW, 2, 1);
		else if ((strcmp(buf, "quit") == 0 || strcmp(buf, "-q") == 0)) exit = 0;
		else if ((strcmp(buf, "chatRoom") == 0 || strcmp(buf, "-cr") == 0)) chatShowW(showPannel, 1, 1);
		else if ((strcmp(buf, "UserRegister") == 0 || strcmp(buf, "-ur") == 0)) userShowW(showPannel, 1, 1);
		else {
			mvwprintw(cmdW, 2, 1, "Comando NON riconosciuto");
		}

		wrefresh(cmdW);
	}
	endwin();
	return 0;
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
	wrefresh(w);
}

void userShowW(WINDOW *w, int y_start, int x_start) {
	mvwprintw(w, 1, 0, "Sul Server sono attualmente Inscritti:");
	char **user = UserDefine();
	char **userStartPoint = user;
	int i = 1;
	for (; *user != NULL; user++) {
		mvwprintw(w, y_start + i, x_start, "[%d]\t|--%s\n", i, *user);
		i++;
	}
	freeDublePointerArr(userStartPoint, sizeof(userStartPoint));
	wrefresh(w);
}


void menuHelp() {
	printf("-h\thelp\t-> elenco comandi disponibili\n");
	printf("-q\tquit\t-> termina server\n");
}

void acceptTh(thAcceptlArg *info) {
	printf("\t###accettatore n°%d\n", info->id);
	pause();
}

void userTh(thUserlArg *info) {

}


//TODO:login funx

//todo: insert in file
/*
 * Funzione trovata online che inserisce nella posizione attuale di fseek la stringa contenuta in *buffer
 int finsert (FILE* file, const char *buffer) {

    long int insert_pos = ftell(file);
    if (insert_pos < 0) return insert_pos;

    // Grow from the bottom
    int seek_ret = fseek(file, 0, SEEK_END);
    if (seek_ret) return seek_ret;
    long int total_left_to_move = ftell(file);
    if (total_left_to_move < 0) return total_left_to_move;

    char move_buffer[4096];
    long int ammount_to_grow = strlen(buffer);
    if (ammount_to_grow >= sizeof(move_buffer)) return -1;

    total_left_to_move -= insert_pos;

    for(;;) {
        u16 ammount_to_move = sizeof(move_buffer);  //prende la dimensione del buffer
        if (total_left_to_move < ammount_to_move) ammount_to_move = total_left_to_move;

        long int read_pos = insert_pos + total_left_to_move - ammount_to_move;

        seek_ret = fseek(file, read_pos, SEEK_SET);
        if (seek_ret) return seek_ret;
        fread(move_buffer, ammount_to_move, 1, file);
        if (ferror(file)) return ferror(file);

        seek_ret = fseek(file, read_pos + ammount_to_grow, SEEK_SET);
        if (seek_ret) return seek_ret;
        fwrite(move_buffer, ammount_to_move, 1, file);
        if (ferror(file)) return ferror(file);

        total_left_to_move -= ammount_to_move;

        if (!total_left_to_move) break;

    }

    seek_ret = fseek(file, insert_pos, SEEK_SET);
    if (seek_ret) return seek_ret;
    fwrite(buffer, ammount_to_grow, 1, file);
    if (ferror(file)) return ferror(file);

    return 0;
}


 **** esempio di uso
	FILE * file= fopen("test.data", "r+");
	ASSERT(file);

	const char *to_insert = "INSERT";

	fseek(file, 3, SEEK_SET);
	finsert(file, to_insert);

	ASSERT(ferror(file) == 0);
	fclose(file);

 *
 */
//todo: delete and compress data in file


//TODO: Id-name
/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere Id utenti messaggio*/

/*
 * Il file dovra contenere gli id degli utenti e contenere
 * anche il loro nome equivalente in codice ascii
 *
 *
 * (Opzionale)
 * Tenere traccia anche di una password per poter accedere.
 *
 *
 * /---|----|---|-----------|---|-----------|----|---\
 * | 1 | Id | 2 |   NAME    |':'| password  |'\0'| 3 |
 * \---|----|---|-----------|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: -2: in ascii indica "End of Trasmission"
 */

//TODO: Messaggi
/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere pacchetti messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * (i numeri sono i valori decimali dell'equivalente ascii)
 *
 * /---|----|-----|---|-----------|----|---\
 * | 1 | Id | Ora | 2 |   TEXT    |'\0'| 3 |
 * \---|----|-----|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: in ascii indica "End of Trasmission"
 */


/*TODO: creazione connessione tramite socket
 * 
 */

/*TODO: creazione thread utente e sue funzioni di appoggio
 *
 */
