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

#define fflush(stdin) while(getchar() != '\n')

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
    connection con;
    void *arg;
} thConnArg;

// PROTOTIPI DI FUNZIONE:

// GLOBALI

connection *initSocket(u_int16_t port, char *IP);

int keepAlive(int *ds_sock);

void freeConnection(connection *con);

int readPack(int ds_sock, mail *pack);  // queste due funzioni prendono il pacchetto thread-specifico
int writePack(int ds_sock, mail *pack); // ma all'interno contengono la struct mail con i dati

int fillPack(mail *pack, int type, char *sender, char *whoOrWhy, void *mex, int len);

void printPack(mail *pack);

///Server FUNCTION

int initServer(connection *connection, int coda);

//tira su un th del tipo void th(thConnArg* info) e dentro info.arg viene messo argFx
int acceptCreate(connection *connection, void *(*thUserServer)(void *), void *argFx);

int loginServerSide(int sock, mail *pack);

///Client FUNCTION
int initClient(connection *c);

int loginUserSide(int sock, mail *pack);

#endif //SOCKETDEMO_SOCKETCONNECT_H

/* todo: creazione di due thread, uno che stara' in invio dati dal client, (threadFromClient)
 *       e l'altro che stara' in ricezione dati verso il client; (threadToClient)
 *       ognuno di essi processa i pacchetti che arrivano (o delega il lavoro a un terzo thread)
 *       il ToC parte dopo il FromC (che si occupa della fase di login inizialmente);
 *       vediamo se possiamo far spawnare un thread ad un altro thread, altrimenti usiamo un semaforo;
 */

/* TIPI PACCHETTO
 * type = 0 : LOGIN, mex non esiste (dim = 0)
 * type = 1 :
 *
 *
 *
 */


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







