//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"



void *acceptTh(thAcceptArg *info) {

    thUserArg *arg;

	while (1) {
        arg = malloc(sizeof(thUserArg));
        printf("arg = %p creato.\n", arg);
        //definisco gli argomenti da passare al th user
        arg->id = -1;
        arg->conUs = &info->conInfo;
        arg->info = NULL;
        arg->userPath[0] = 0;

        if (acceptCreate(&info->conInfo.con, userTh, arg) == -1) {
            dprintf(fdOutP, "errore in accept\n");
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

void *userTh(thUserArg *info) {
    printf("TH creato\nId = %d\n", info->id);

    //mail *packRecive= malloc(sizeof(mail));

    /*
    if(loginServerSide(argTh->con.ds_sock, packRecive) == -1){
        printf("Login fallito; disconnessione...\n");
        pthread_exit(NULL);
    }  /// RIVEDERE LA PARTE DELL'IF, RIMANGONO TROPPI ACCEPT NON LIBERATI COMPLETAMENTE IN QUESTO MODO
    */


    printf("mi metto in ascolto\n");
    pthread_t tidRX, tidTX;

    pthread_create(&tidRX, NULL, thrServRX, info);
    //pthread_create(&tidTX,NULL, thrServTX,info);

    pause();
    pthread_exit(NULL);
    /*
	dprintf(fdOutP, "Ciao sono Un Tr-USER\n\tsono la %d\tmi chiamo %s\n", info->id, info->userPath);
	pause();
     */
	free(info);
	return NULL;
}

void *thrServRX(thUserArg *argTh) {

    mail packRecive;

    while (1) {

        readPack(argTh->conUs->con.ds_sock, &packRecive);
        dprintf(fdOutP, "Numero byte pacchetto: %d\n", packRecive.md.dim);
        dprintf(fdOutP, "Stringa da client: %s\n\n", packRecive.mex);

        writePack(argTh->conUs->con.ds_sock, &packRecive);

        if (strcmp(packRecive.mex, "quit") == 0) {
            break;
        }


        //writePack(); da aggiungere il selettore chat
    }
    close(argTh->conUs->con.ds_sock);
    //free(argTh); //todo creare i free per ogni tipo di dato !!!!
    pthread_exit(0);
}

void *thrServTX(thUserArg *argTh) {

    mail packWrite;
    //fillPack(&packWrite,);

    while (1) {
        //readPack() da aggiungere il selettore in ingresso di chat


        writePack(argTh->conUs->con.ds_sock, &packWrite);
        if (errno) {
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