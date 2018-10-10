//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"



void *acceptTh(thAcceptArg *info) {

    thUserArg *arg;

	while (1) {
        arg = malloc(sizeof(thUserArg));
		dprintf(fdOutP, "acceptTh Creato, arg = %p creato.\n", arg);
		//definisco gli argomenti da passare al th user come puntati da arg di thConnArg.arg
        arg->id = -1;
		arg->conUs = info->conInfo; //copia negli argomenti del th una copia totale della connessione del server
        arg->info = NULL;
        arg->userPath[0] = 0;

        if (acceptCreate(&info->conInfo.con, userTh, arg) == -1) {
	        dprintf(STDERR_FILENO, "errore in accept\n");
        }
	}
	free(info);

	///Codice funzionante per la creazione di un thread user passandogli correttamente il parametro info
	/*
	infoUser *info;
	long idKey = atoi(info->myName);       //essendo myname xx:TEXT, la funzione termina ai : e ottengo la key
	char userDir[128];
	sprintf(userDir, "./%s/%ld:%s", userDirName, idKey, argv[1]);
	makeThUser(idKey, userDir, info);
	dprintf(fdOutP, "USER th creato, idKey=%d\n", idKey);
	*/
	return NULL;
}

void *userTh(thConnArg *info) {
	thUserArg *arg = info->arg;
	arg->conUs.con = info->con;
	dprintf(fdOutP, "TH-User creato\nId = %d\n", arg->id);

    //mail *packRecive= malloc(sizeof(mail));

    /*
    if(loginServerSide(argTh->con.ds_sock, packRecive) == -1){
        printf("Login fallito; disconnessione...\n");
        pthread_exit(NULL);
    }  /// RIVEDERE LA PARTE DELL'IF, RIMANGONO TROPPI ACCEPT NON LIBERATI COMPLETAMENTE IN QUESTO MODO
    */


	dprintf(fdOutP, "mi metto in ascolto\n");
    pthread_t tidRX, tidTX;

	pthread_create(&tidRX, NULL, thrServRX, arg);
    //pthread_create(&tidTX,NULL, thrServTX,info);

	while (1) pause();
	free(info);
	pthread_exit(NULL);

	return NULL;
}

void *thrServRX(thUserArg *argTh) {

    mail packRecive;

    while (1) {
	    dprintf(fdOutP, "thrServRx %d in attesa di messaggio da %d sock\n", argTh->id, argTh->conUs.con.ds_sock);
        if (readPack(argTh->conUs.con.ds_sock, &packRecive) == -1) {
            dprintf(STDERR_FILENO, "Read error, broken pipe\n");
            sleep(1);
            exit(-1); // todo gestione broken pipe e uscita thread
        }

        dprintf(fdOutP, "Numero byte pacchetto: %ld\n", packRecive.md.dim);
        dprintf(fdOutP, "Stringa da client: %s\n\n", packRecive.mex);

	    writePack(argTh->conUs.con.ds_sock, &packRecive);

        if (strcmp(packRecive.mex, "quit") == 0) {
            break;
        }


        //writePack(); da aggiungere il selettore chat
    }
	close(argTh->conUs.con.ds_sock);
    //free(argTh); //todo creare i free per ogni tipo di dato !!!!
    pthread_exit(0);
}

void *thrServTX(thUserArg *argTh) {

    mail packWrite;
    //fillPack(&packWrite,);

    while (1) {
        //readPack() da aggiungere il selettore in ingresso di chat


        if (writePack(argTh->conUs.con.ds_sock, &packWrite) == -1) {
            dprintf(STDERR_FILENO, "Write error, broken pipe\n");
            exit(-1);
        }
    }
}

void *roomTh(thRoomArg *info) {
	dprintf(fdOutP, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n", info->id, info->roomPath);
	pause();
	free(info);
	return NULL;

}

void makeThRoom(int keyChat, char *roomPath, infoChat *info) {
	if (info == NULL) {
		dprintf(STDERR_FILENO, "infoChat NULL, impossibile creare Tr-ROOM\n");
		return;
	}
	pthread_t roomtid;
	thRoomArg *arg = malloc(sizeof(thRoomArg));
	arg->id = keyChat;

	strncpy(arg->roomPath, roomPath, 50);
	arg->info = info;

	int errorRet;
	errorRet = pthread_create(&roomtid, NULL, roomTh, arg);
	if (errorRet != 0) {
		printErrno("La creazione del Thread ROOM ha dato il seguente errore", errorRet);
		exit(-1);
	}

}

void makeThUser(int keyId, char *userPath, infoUser *info) {
	if (info == NULL) {
		dprintf(STDERR_FILENO, "infoUser NULL, impossibile creare Tr-ROOM\n");
	}
	pthread_t usertid;
	thUserArg *arg = malloc(sizeof(thUserArg));
	arg->id = keyId;

	strncpy(arg->userPath, userPath, 50);
	arg->info = info;

	int errorRet;
	errorRet = pthread_create(&usertid, NULL, userTh, arg);
	if (errorRet != 0) {
		printErrno("La creazione del Thread USER ha dato il seguente errore", errorRet);
		exit(-1);
	}
}