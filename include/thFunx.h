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
#include "socketConnect.h"

typedef struct thAcceptArg_ {
	int id;
    thConnArg conInfo;
} thAcceptArg;

typedef struct thUserArg_ {
    int id;             //idKey dell'user definito in fase di login
    char userPath[50];  // path relativo a home dell'user in questione, login
    infoUser *info;     // contiene tabella e nome per esteso, fase login
	thConnArg conUs;
} thUserArg;

typedef struct thRoomArg_ {
	int id;
	char roomPath[50];
	infoChat *info;
} thRoomArg;

///GLOBAL VARIABLE
extern int fdOutP;  //pipe di uscita per lo stdOut


/** PROTOTIPI   **/
void *acceptTh(thAcceptArg *);

void *userTh(thConnArg *);

void *thrServRX(thUserArg *);

void *thrServTX(thUserArg *);

void *roomTh(thRoomArg *);

void *thUserServer(thConnArg *argTh);


void makeThRoom(int keyChat, char *roomPath, infoChat *info);

void makeThUser(int keyId, char *userPath, infoUser *info);


#endif //CHAT_MULTILEVEL_THFUNX_H
