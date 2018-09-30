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
	return NULL;
}

void *userTh(thUserArg *info) {
	pause();
	free(info);
	return NULL;
}

void *roomTh(thRoomArg *info) {
	dprintf(fdOutP, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n", info->id, info->name);
	pause();
	free(info);
	return NULL;

}

void makeThRoom(int keyChat, char *roomPath, infoChat *info) {
	if (info == NULL) {
		dprintf(STDERR_FILENO, "infoChat NULL, impossibile creare Tr-ROOM\n");
	}
	pthread_t roomtid;
	thRoomArg *arg = malloc(sizeof(thRoomArg));
	arg->id = keyChat;

	strncpy(arg->name, roomPath, 28);
	arg->info = info;

	int errorRet;
	errorRet = pthread_create(&roomtid, NULL, roomTh, arg);
	if (errorRet != 0) {
		printErrno("La creazione del Thread ROOM ha dato il seguente errore", errorRet);
		exit(-1);
	}
}