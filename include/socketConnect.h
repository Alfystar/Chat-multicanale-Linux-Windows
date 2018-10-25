//
// Created by filippo on 25/09/18.
//

#ifndef SOCKETDEMO_SOCKETCONNECT_H
#define SOCKETDEMO_SOCKETCONNECT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#include "../globalSet.h"

#define fflush(stdin) while(getchar() != '\n')

enum typePack {
	/** SYSTEM **/    success_p = 0, failed_p, out_mess_p, out_test_p,
	/**  USER  **/    out_login_p, out_logout_p, out_delUs_p, out_mkUser_p, out_dataUs_p,
	/**  ROOM  **/    out_mkRoom_p, out_joinRm_p, out_openRm_p, out_dataRm_p, out_leaveRm_, out_delRm_p, out_exitRm_p
};

typedef struct metadata_ {
	size_t dim;
	int type; // dobbiamo definire dei tipi di comandi: es. 0 per il login => password in campo mex, ...
	char sender[28];
	char whoOrWhy[24];
} metadata;

typedef struct mail_ {
	metadata md;
	void *mex;
} mail;

typedef struct connection_ {
	int ds_sock;
	struct sockaddr_in sock;
} connection;

typedef struct thConnArg_ {
	connection con;     //connessioni soket attuale
	void *arg;          //possibili parametri extra
} thConnArg;

// PROTOTIPI DI FUNZIONE:

// GLOBALI

connection *initSocket(u_int16_t port, char *IP);

int keepAlive(int *ds_sock);

void freeConnection(connection *con);

int readPack(int ds_sock, mail *pack);  // queste due funzioni prendono il pacchetto thread-specifico
int writePack(int ds_sock, mail *pack); // ma all'interno contengono la struct mail conInfo i dati

int testConnection(int ds_sock);

int fillPack(mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy);

void printPack(mail *pack);

///Server FUNCTION

int initServer(connection *connection, int coda);

//tira su un th del tipo void th(thConnArg* info) e dentro info.arg viene messo argFx
int acceptCreate(connection *connection, void *(*thUserServer)(void *), void *argFx);

///Client FUNCTION
int initClient(connection *c);


#endif //SOCKETDEMO_SOCKETCONNECT_H



/// ### DOCUMENTAZIONE ### ///

/* SO_KEEPALIVE
 *
 * http://tldp.org/HOWTO/TCP-Keepalive-HOWTO/usingkeepalive.html
 * Al paragrafo 3.1.1, e' mostrato come modificare il parametro temporale di sistema tramite CAT.
 * Sara' necessario ridurre i tempi per rendersene conto;
 * I cambiamenti dovranno essere impostati ad ogni avvio;
 *
 * E' POSSIBILE FARLO CON DEI VALORI DA MODIFICARE!!! VEDERE PARAGRAFO 4.2
 *
 */







