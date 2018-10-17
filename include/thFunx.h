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


/** [][][][][][][][] PROTOTIPI [][][][][][][][]  **/

/** TH-ACCEPT, GENERA TH USER IN FASE DI ACCEPT **/
void *acceptTh(thAcceptArg *);

/** [][][] TH-USER GENERICO, prima di specializzarsi, HA IL COMPITO DI LOGIN **/
void *userTh(thConnArg *);

/** FUNZIONI PER IL TH-USER GENERICO, prima di specializzarsi **/
int loginServerSide(mail *pack, thUserArg *data);

int setUpThUser(int keyId, thUserArg *argUs);

int mkUserServerSide(mail *pack, thUserArg *data);

void *sendTab(table *t, int *len);

/** #### TH-USER SUL SERVER CON RUOLO DI RX **/
void *thrServRX(thUserArg *);

/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX **/

int mkRoomSocket(mail *pack, char *nameChatRet, int len);

void makeThRoom(int keyChat, char *roomPath, infoChat *info);

int joinRoomSocket(mail *pack, thUserArg *data);

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thrServTX(thUserArg *);

/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI TX **/


/** [][][] TH-ROOM GENERICO, prima di specializzarsi, HA IL COMPITO DI creare le strutture **/
void *roomTh(thRoomArg *);

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX(thRoomArg *info);

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX **/


/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX(thRoomArg *info);

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI TX **/


/** SEND PACK_inside e WRITE PACK_inside**/

int readPack_inside(int fdPipe, mail *pack);

int writePack_inside(int fdPipe, mail *pack);

int testConnection_inside(int fdPipe);
//todo freeTh-s_arg

#endif //CHAT_MULTILEVEL_THFUNX_H
