//
// Created by filippo on 25/09/18.
//

#include "../include/socketConnect.h"

/// GLOBAL FUNCTION
connection *initSocket (u_int16_t port, char *IP){
	connection *con = malloc (sizeof (connection));

	con->ds_sock = socket (AF_INET, SOCK_STREAM, 0);

	if (keepAlive (&con->ds_sock) == -1){
		return NULL;
	};

	bzero (&con->sock, sizeof (con->sock));
	con->sock.sin_family = AF_INET;
	con->sock.sin_port = htons (port);
	if (strcmp (IP, "INADDR_ANY") == 0){
		con->sock.sin_addr.s_addr = INADDR_ANY;
	}
	else{
		con->sock.sin_addr.s_addr = inet_addr (IP);
	}
	return con;
}

int keepAlive (int *ds_sock){
	/// KEEPALIVE FUNCTION: vedere header per breve documentazione
	int optval;
	socklen_t optlen;
	/// Impostamo i valori temporali degli ACK

	// Tempo di primo ACK (tcp_keepalive_time)
	optval = 30; //tempo in secondi
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	// Numero di "sonde" prima dell'abort (tcp_keepalive_probes)
	optval = 5; // n. di tentativi
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	//Tempo tra una sonda e l'altra (tcp_keepalive_intvl)
	optval = 6; // tempo in secondi tra l'uno e l'altro
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	// IN CASO DI MANCATA RISPOSTA IN UN MINUTO, L'UTENTE RISULTERA' SCOLLEGATO!

	// Attiviamo il keepalive
	optval = 1;
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	printf ("SO_KEEPALIVE set on socket\n");
	return 0;
}

int readPack (int ds_sock, mail *pack){

	int iterContr = 0; // vediamo se la read fallisce
	ssize_t bRead = 0;
	ssize_t ret = 0;

	sigset_t newSet, oldSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oldSet);

	mailChar mailCRead;

	do{
		ret = read (ds_sock, &mailCRead.md + bRead, sizeof (metadataChar) - bRead);
		if (ret == -1){
			perror ("Read error; cause:");
			pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
			return -1;
		}
		if (ret == 0){
			iterContr++;
			if (iterContr > 2){
				dprintf (STDERR_FILENO, "Seems Read can't go further; test connection...\n");
				if (testConnection (ds_sock) == -1){
					pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
					return -1;
				}
			}
		}
		bRead += ret;
	}
	while (sizeof (metadataChar) - bRead != 0);

	//** CONVERSIONE MAIL IN INTERI **//

	sscanf (mailCRead.md.dim, "%ld", &pack->md.dim);
	sscanf (mailCRead.md.type, "%d", &pack->md.type);
	memcpy (pack->md.sender, mailCRead.md.sender, sendDim);
	memcpy (pack->md.whoOrWhy, mailCRead.md.whoOrWhy, wowDim);

	//****//

	size_t dimMex = pack->md.dim; // manteniamo in ordine della macchina il valore del messaggio

	//****//

	if (dimMex == 0){
		pack->mex = NULL;
		pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
		return 0;
	}

	pack->mex = malloc (dimMex);

	bRead = 0; //rimetto a zero per la nuova lettura
	ret = 0;
	iterContr = 0;
	do{
		ret = read (ds_sock, pack->mex + bRead, dimMex - bRead);
		if (ret == -1){
			perror ("Read error; cause:");
			pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
			return -1;
		}
		if (ret == 0){
			iterContr++;
			if (iterContr > 2){
				dprintf (STDERR_FILENO, "Seems Read can't go further; test connection...\n");
				if (testConnection (ds_sock) == -1){
					pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
					return -1;
				}
			}
		}
		bRead += ret;
	}
	while (dimMex - bRead != 0);
	pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
	return 0;
}

int writePack (int ds_sock, mail pack) //dentro il thArg deve essere puntato un mail
{
	size_t dimMex = pack.md.dim; // manteniamo in ordine della macchina il valore del messaggio

	//** CONVERSIONE IN MAIL A CARATTERI **//

	mailChar mailCWrite;
	sprintf (mailCWrite.md.dim, "%ld", pack.md.dim);
	sprintf (mailCWrite.md.type, "%d", pack.md.type);
	memcpy (mailCWrite.md.sender, pack.md.sender, sendDim);
	memcpy (mailCWrite.md.whoOrWhy, pack.md.whoOrWhy, wowDim);
	mailCWrite.mex = pack.mex;

	//****//

	/// la funzione si aspetta che il buffer non sia modificato durante l'invio
	ssize_t bWrite = 0;
	ssize_t ret = 0;

	sigset_t newSet, oldSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oldSet);

	do{
		ret = send (ds_sock, &mailCWrite + bWrite, sizeof (metadataChar) - bWrite, MSG_NOSIGNAL);
		if (ret == -1){
			if (errno == EPIPE){
				dprintf (STDERR_FILENO, "write pack pipe break 1\n");
				pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
				return -1;
				//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
			}
		}
		bWrite += ret;

	}
	while (sizeof (metadataChar) - bWrite != 0);

	if (dimMex == 0){
		pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
		return 0;
	} //cosi' evitiamo un periodo di scrittura

	bWrite = 0;

	do{
		ret = send (ds_sock, mailCWrite.mex + bWrite, dimMex - bWrite, MSG_NOSIGNAL);
		if (ret == -1){
			if (errno == EPIPE){
				dprintf (STDERR_FILENO, "write pack pipe break 2\n");
				pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
				return -1;
				//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
			}
		}
		bWrite += ret;

	}
	while (dimMex - bWrite != 0);
	pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
	return 0;
}

int testConnection (int ds_sock){
	mail packTest;

	fillPack (&packTest, out_test_p, 0, NULL, "SERVER", "testing_code");

	if (writePack (ds_sock, packTest) == -1){
		return -1;
	}
	return 0;
}

void freeConnection (connection *con){
	free (con);
}

int fillPack (mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy){
	if (pack == NULL){
		errno = EFAULT;
		return -1;
	}

	pack->md.type = type;
	pack->md.dim = (size_t)dim;

	if (dim == 0)pack->mex = NULL;
	else pack->mex = mex;

	if (sender == NULL) strncpy (pack->md.sender, "", sendDim);
	else{
		strncpy (pack->md.sender, sender, sendDim);
		pack->md.sender[sendDim - 1] = '\0'; // sono sicuro che possa venir letto come una stringa
	}

	if (whoOrWhy == NULL) strncpy (pack->md.whoOrWhy, "", wowDim);
	else{
		strncpy (pack->md.whoOrWhy, whoOrWhy, wowDim);
		pack->md.whoOrWhy[wowDim - 1] = '\0'; // sono sicuro che possa venir letto come una stringa
	}

	return 0;
}

void freePack (mail *p){
	freeMexPack (p);
	if (p) free (p);
}

void freeMexPack (mail *p){
	if (p->mex){
		free (p->mex);
		p->mex = NULL;
	}
}
///Server FUNCTION

int initServer (connection *c, int coda){
	if (bind (c->ds_sock, (struct sockaddr *)&c->sock, sizeof (c->sock))){
		perror ("Bind failed, cause:");
		return -1;
	}
	if (listen (c->ds_sock, coda)){
		perror ("Listen failed, cause:");
		return -1;
	}
	return 0;
}

int acceptCreate (connection *c, void *(*thUserServer) (void *), void *arg){
	//in caso arrivi una connessione crea un th di tipo void* NAME (thConnArg*) in thConnArg.arg si trovano i parametri per il th
	// Si suppone che arg sia stata precedentemente malloccata
	connection *conNew;
	conNew = malloc (sizeof (connection));
	unsigned int len = sizeof (conNew->sock);

	conNew->ds_sock = accept (c->ds_sock, (struct sockaddr *)&conNew->sock, &len);
	if (conNew->ds_sock == -1){
		perror ("Accept client error; cause:");
		close (conNew->ds_sock);
		return -1;
	}
	pthread_t tid;
	thConnArg *argTh = malloc (sizeof (thConnArg));

	argTh->arg = arg;
	argTh->con = *conNew;

	pthread_create (&tid, NULL, thUserServer, argTh);
	return 0;
}


///Client FUNCTION

int initClient (connection *c){
	if (connect (c->ds_sock, (struct sockaddr *)&c->sock, sizeof (c->sock))){
		perror ("Connect error; cause:");
		close (c->ds_sock);
		return -1;
	}
	return 0;
}

