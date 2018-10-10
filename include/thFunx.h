//
// Created by alfylinux on 30/09/18.
//

#ifndef CHAT_MULTILEVEL_THFUNX_H
#define CHAT_MULTILEVEL_THFUNX_H

#define _GNU_SOURCE             /* See feature_test_macros(7) */

/** Thread lib **/
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <limits.h>


#include "fileSystemUtylity.h"
#include "mexData.h"
#include "tableFile.h"
#include "socketConnect.h"
#include "../treeFunx/include/avl.h"
#include "../globalSet.h"
#include "terminalShell.h"

typedef struct thAcceptArg_ {
	int id;
    thConnArg conInfo;
} thAcceptArg;

typedef struct thUserArg_ {
    int id;             //idKey dell'user definito in fase di login
	char userName[50];  // Nome dell'user (dopo :)
    infoUser *info;     // contiene tabella e nome per esteso, fase login
	thConnArg conUs;    //contiene Tab e path dir dell'user
} thUserArg;

typedef struct thRoomArg_ {
	int id;
	char roomPath[50];
	infoChat *info;
} thRoomArg;

///GLOBAL VARIABLE
extern int fdOut;  //pipe di uscita per lo stdOut


/** PROTOTIPI   **/
void *acceptTh(thAcceptArg *);

void *userTh(thConnArg *);

int loginServerSide(mail *pack, thUserArg *data);

void *thrServRX(thUserArg *);

void *thrServTX(thUserArg *);

void *roomTh(thRoomArg *);

void *thUserServer(thConnArg *argTh);


void makeThRoom(int keyChat, char *roomPath, infoChat *info);

int setUpThUser(int keyId, thUserArg *argUs);

//todo freeTh-s_arg

#endif //CHAT_MULTILEVEL_THFUNX_H
