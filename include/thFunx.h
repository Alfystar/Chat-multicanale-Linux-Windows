//
// Created by alfylinux on 30/09/18.
//

#ifndef CHAT_MULTILEVEL_THFUNX_H
#define CHAT_MULTILEVEL_THFUNX_H

/** Thread lib **/
#include <unistd.h>
#include <pthread.h>

#include "fileSystemUtylity.h"
#include "mexData.h"
#include "tableFile.h"

typedef struct thAcceptArg_ {
	int id;
	int fdStdout;
	int fdStderr;
} thAcceptArg;

typedef struct thUserArg_ {
	int id;
} thUserArg;

typedef struct thRoomArg_ {
	int id;
	char name[28];
	infoChat *info;
} thRoomArg;


///GLOBAL VARIABLE
extern int fdOutP;  //pipe di uscita per lo stdOut


/** PROTOTIPI   **/
void *acceptTh(thAcceptArg *);

void *userTh(thUserArg *);

void *roomTh(thRoomArg *);

void makeThRoom(int keyChat, char *roomPath, infoChat *info);

#endif //CHAT_MULTILEVEL_THFUNX_H
