//
// Created by filippo on 20/09/18.
// PROTOTIPO FUNZIONANTE DI SERVER LOCALE
//

// ### DA FARE ###
/* 1. implementare il forward verso i client
 * 2. differenziare i client e cambiare il fork in threads
 * 3. implementare NCURSES
 *
 * VEDERE https://www.binarytides.com/server-client-example-c-sockets-linux/
 */


#include "FIL-server.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_DIM 1024
#define CODA 3

int main() {
    int ds_sock, ds_sock_a;
    struct sockaddr_in server;
    struct sockaddr client;
    char buff[MAX_DIM];
    ds_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(ds_sock, (struct sockaddr *) &server, sizeof(server));
    listen(ds_sock, CODA);
    unsigned int length = sizeof(client);
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        while ((ds_sock_a = accept(ds_sock, &client, &length)) == -1);

        if (fork() == 0) {

            close(ds_sock);
            do {
                read(ds_sock_a, buff, MAX_DIM);
                printf("messaggio del client = %s\n", buff);
            } while (strcmp(buff, "quit") != 0);

            write(ds_sock_a, "letto", strlen("letto") + 1);
            close(ds_sock_a);
            exit(0);
        }
        else close(ds_sock_a);
    }
}