//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"


void *acceptTh(thAcceptArg *info) {
	while (1) {
		dprintf(fdOutP, "ciao sono un th Accept\n");
		pause();
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
	dprintf(fdOutP, "Ciao sono Un Tr-USER\n\tsono la %d\tmi chiamo %s\n", info->id, info->userPath);
	pause();
	free(info);
	return NULL;
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
	return;

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