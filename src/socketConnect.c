//
// Created by filippo on 25/09/18.
//

#include "../include/socketConnect.h"

/// GLOBAL FUNCTION
connection *initSocket(u_int16_t port, char *IP) {
    connection *con = malloc(sizeof(connection));

    con->ds_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (keepAlive(&con->ds_sock) == -1) {
        return NULL;
    };

    bzero(&con->sock, sizeof(con->sock));
    con->sock.sin_family = AF_INET;
    con->sock.sin_port = htons(port);
    if (strcmp(IP, "INADDR_ANY") == 0) {
        con->sock.sin_addr.s_addr = INADDR_ANY;
    } else {
        con->sock.sin_addr.s_addr = inet_addr(IP);
    }
    return con;
}

int keepAlive(int *ds_sock) {
    /// KEEPALIVE FUNCTION: vedere header per breve documentazione
    int optval;
    socklen_t optlen;
    /// Impostamo i valori temporali degli ACK

    // Tempo di primo ACK (tcp_keepalive_time)
    optval = 30; //tempo in secondi
    optlen = sizeof(optval);
    if (setsockopt(*ds_sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    // Numero di "sonde" prima dell'abort (tcp_keepalive_probes)
    optval = 5; // n. di tentativi
    optlen = sizeof(optval);
    if (setsockopt(*ds_sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    //Tempo tra una sonda e l'altra (tcp_keepalive_intvl)
    optval = 6; // tempo in secondi tra l'uno e l'altro
    optlen = sizeof(optval);
    if (setsockopt(*ds_sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    // IN CASO DI MANCATA RISPOSTA IN UN MINUTO, L'UTENTE RISULTERA' SCOLLEGATO!

    // Attiviamo il keepalive
    optval = 1;
    optlen = sizeof(optval);
    if (setsockopt(*ds_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    printf("SO_KEEPALIVE set on socket\n");
    return 0;
}

int writePack(int ds_sock, mail *pack) //dentro il thArg deve essere puntato un mail
{
    /// la funzione si aspetta che il buffer non sia modificato durante l'invio
    ssize_t bWrite = 0;
    ssize_t ret = 0;

    do {
        ret = send(ds_sock, pack + bWrite, sizeof(metadata) - bWrite, MSG_NOSIGNAL);
        if (ret == -1) {
            if (errno == EPIPE) {
                dprintf(STDERR_FILENO, "write pack pipe break 1\n");

                //GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
            }
        }
        bWrite += ret;

    } while (sizeof(metadata) - bWrite != 0);

    bWrite = 0;

    do {
        ret = send(ds_sock, pack->mex + bWrite, pack->md.dim - bWrite, MSG_NOSIGNAL);
        if (ret == -1) {
            if (errno == EPIPE) {
                dprintf(STDERR_FILENO, "write pack pipe break 2\n");

                //GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
            }
        }
        bWrite += ret;

    } while (pack->md.dim - bWrite != 0);

    return 0;
}

int readPack(int ds_sock, mail *pack) //todo: implementare controllo sulle read
{
    ssize_t bRead = 0;
    dprintf(STDERR_FILENO, "readPack\n");
    do {
        bRead += read(ds_sock, &pack->md + bRead, sizeof(metadata) - bRead);
        dprintf(STDERR_FILENO, "readPack metadata\n");
        //todo read in caso di sick pipe entra in while perpetuo qui
    } while (sizeof(metadata) - bRead != 0);

    size_t dimMex = pack->md.dim;

    if (dimMex == 0) {
        pack->mex = NULL;
        return 0;
    }

    pack->mex = malloc(dimMex);

    bRead = 0; //rimetto a zero per la nuova lettura
    do {
        bRead += read(ds_sock, pack->mex + bRead, dimMex - bRead);
        dprintf(STDERR_FILENO, "readPack mex\n");

    } while (dimMex - bRead != 0);

    return 0;
}

void freeConnection(connection *con) {
    free(con);
}


int fillPack(mail *pack, int type, char *sender, char *whoOrWhy, void *mex, int len) {
    pack->md.dim = len;
    pack->md.type = type;
    strncpy(pack->md.whoOrWhy, whoOrWhy, 24);
    strncpy(pack->md.sender, sender, 28);
    pack->mex = mex;
    return 0;
}

void printPack(mail *pack) {
    dprintf(fdOutP, "######[][]I METADATI SONO[][]######\n");
    dprintf(fdOutP, "Dim pack = %ld\n", pack->md.dim);
    dprintf(fdOutP, "Type = %d\n", pack->md.type);
    dprintf(fdOutP, "Sender = %s\n", pack->md.sender);
    dprintf(fdOutP, "whoOrWhy = %s\n", pack->md.whoOrWhy);
    dprintf(fdOutP, "------[][]IL MESSAGGIO[][]------\n");
    dprintf(fdOutP, "TEXT :\n--> %s\n\n", pack->mex); //non sempre stringa
}
///Server FUNCTION

int initServer(connection *c, int coda) {
    if (bind(c->ds_sock, (struct sockaddr *) &c->sock, sizeof(c->sock))) {
        perror("Bind failed, cause:");
        return -1;
    }
    if (listen(c->ds_sock, coda)) {
        perror("Listen failed, cause:");
        return -1;
    }
    return 0;
}

int acceptCreate(connection *c, void *(*thUserServer)(void *), void *arg) {
    //in caso arrivi una connessione crea un th di tipo void* NAME (thConnArg*) in thConnArg.arg si trovano i parametri per il th
    dprintf(fdOutP, "Dentro acceptCreate-funx\n");
    // Si suppone che arg sia stata precedentemente malloccata
    connection *conNew;
    conNew = malloc(sizeof(connection));
    unsigned int len = sizeof(conNew->sock);

    conNew->ds_sock = accept(c->ds_sock, (struct sockaddr *) &conNew->sock, &len);
    if (conNew->ds_sock == -1) {
        perror("Accept client error; cause:");
        close(conNew->ds_sock);
        return -1;
    }
    pthread_t tid;

    thConnArg *argTh = malloc(sizeof(thConnArg));

    dprintf(fdOutP, "arg prima e': %p\n", argTh->arg);

    argTh->arg = arg;
    argTh->con = *conNew;

    dprintf(fdOutP, "arg adesso e': %p\n", argTh->arg);

    pthread_create(&tid, NULL, thUserServer, argTh);
    return 0;
}

int loginServerSide(int ds_sock, mail *pack) {
    // vedere sendfile() su man, potrebbe servire per il login
    dprintf(fdOutP, "Utente in fase di collegamento; socket univoco:%d\n", ds_sock);

    readPack(ds_sock, pack);

    dprintf(fdOutP, "Utente = %s\n", (char *) pack->mex);
    dprintf(fdOutP, "Cerco corrispondenza utente e chat associate.\n");

    /// DEFINIRE DOVE TROVARE GLI UTENTI

    writePack(ds_sock, pack);


    return 0;
}

///Client FUNCTION

int initClient(connection *c) {
    if (connect(c->ds_sock, (struct sockaddr *) &c->sock, sizeof(c->sock))) {
        perror("Connect error; cause:");
        close(c->ds_sock);
        return -1;
    }
    return 0;
}

int loginUserSide(int ds_sock, mail *pack) {

    char buffUser[24];
    char buffPass[28];

    printf("Inserire credenziali per login.\nUtente: ");
    scanf("%s", buffUser);
    fflush(stdin);
    printf("\nPassword:");
    scanf("%s", buffPass);
    fflush(stdin);
    printf("\n");

    if (fillPack(pack, 0, buffUser, buffPass, NULL, 0) == -1) {
        return (-1);
    }
    writePack(ds_sock, pack);

    readPack(ds_sock, pack);


    printf("Login effettuato\n");

    return 0;
}