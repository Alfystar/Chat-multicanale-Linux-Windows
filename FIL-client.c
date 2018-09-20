//
// Created by filippo on 20/09/18.
// PROTOTIPO FUNZIONANTE CLIENT PER SERVER LOCALE;
// Testato in multi-client, va ovviamente prima avviato il server
// poi tutti i client che servono
//

/*  ### DA FARE ###
 *  1. aggiungere le varie funzionalita' base del client
 *  2. gestione con NCURSES
 */


#include "FIL-client.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#define MAX_DIM 1024

int main() {
    int ds_sock, length, res;
    struct sockaddr_in client;
    struct hostent *hp = NULL;
    char buff[MAX_DIM];

    printf("Inizializzazione socket\n");

    ds_sock = socket(AF_INET, SOCK_STREAM, 0);
    client.sin_family = AF_INET;
    client.sin_port = htons(8888);
    client.sin_addr.s_addr = inet_addr("127.0.0.1");

    /*
    Metodo con indirizzo da pagina internet

    printf("Ottengo l'IP\n");

    hp = gethostbyname("hermes.dis.uniroma1.it");
    if ( hp == NULL ) {
        printf("Errore nell'ottenere l'IP\n");
        exit(-1);
    }
    printf("Copio l'IP nella struct:\nValore ip %s , Lunghezza %d\n", hp->h_addr,hp->h_length);

    memcpy(&client.sin_addr, hp->h_addr, hp->h_length);
    */

    printf("Tento la connessione\n");

    res = connect(ds_sock, (struct sockaddr *) &client, sizeof(client));
    if ( res == -1 ) {
        printf("Errore nella connect \n");
        exit(1);
    }

    printf("Digitare le stringhe da trasferire (quit per terminare): ");
    do {
        scanf("%s", buff);
        while (getchar() != '\n'); //fflush(stdin)

        write(ds_sock, buff, MAX_DIM);
    } while(strcmp(buff,"quit") != 0);

    read(ds_sock, buff, MAX_DIM);
    printf("Risposta del server: %s\n", buff);
    close(ds_sock);
}

