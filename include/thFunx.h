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
#include "terminalShell.h"

#include "../treeFunx/include/avl.h"
#include "../globalSet.h"

typedef struct thAcceptArg_ {
	int id;
    thConnArg conInfo;
} thAcceptArg;

typedef struct thUserArg_ {
    int id;             //idKey dell'user definito in fase di login
	char userName[50];  // Nome dell'user (dopo :)
	char idNameUs[50];
	infoUser *info;     // contiene tabella e nome per esteso (path), assegnato in fase di login
	thConnArg conUs;    //contiene Tab e path dir dell'user
	int fdPipe[2];      //contiene i valori delle pipe per parlare alla room, Definita da thRoom-generico
	pthread_t tidRx, tidTx;
} thUserArg;

typedef struct thRoomArg_ {
	int id;
	char roomName[50];  // Nome dell'user (dopo :)
	char idNameRm[50];
	char roomPath[50];  // Path completo room  ("./%s/%s", chatDirName, chats->names[i]); e chats->names = %d:%s
	infoChat *info;     // contiene tabella,Conversazione e nome per esteso (path), assegnato in fase di creazione
	int fdPipe[2];      //contiene i valori delle pipe per parlare alla room, Definita da thRoom-generico
	pthread_t tidRx, tidTx;
} thRoomArg;


enum insidePack {
	in_join_p = 1000, in_entryIndex_p, in_delRm_p, in_leave_p, in_mess_p
};


///GLOBAL VARIABLE

/** [][][][][][][][] PROTOTIPI [][][][][][][][]  **/

/** TH-ACCEPT, GENERA TH USER IN FASE DI ACCEPT **/
void *acceptTh(thAcceptArg *);

/** [][][] TH-USER GENERICO, prima di specializzarsi, HA IL COMPITO DI LOGIN **/
void *userTh(thConnArg *);

/* FUNZIONI PER IL TH-USER GENERICO, prima di specializzarsi */
int loginServerSide(mail *pack, thUserArg *data);

int setUpThUser(int keyId, thUserArg *argUs);

int mkUserServerSide(mail *pack, thUserArg *data);

void *sendTab(table *t, int *len);

/** #### TH-USER SUL SERVER CON RUOLO DI RX **/
void *thUs_ServRX(thUserArg *);

/* FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX */

int mkRoomSocket(mail *pack, thUserArg *data);

void makeThRoom(int keyChat, char *roomPath, infoChat *info);

int joinRoomSocket(mail *pack, thUserArg *data);

int delRoomSocket(mail *pack, thUserArg *data);

int leaveRoomSocket(mail *pack, thUserArg *data);

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thUs_ServTX(thUserArg *);

/* FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI TX */


/** [][][] TH-ROOM GENERICO, prima di specializzarsi, HA IL COMPITO DI creare le strutture **/
void *roomTh(thRoomArg *);

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX(thRoomArg *info);

/* FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX */
int joinRoom_inside(mail *pack, thRoomArg *data);

int delRoom_inside(mail *pack, thRoomArg *data);

int leaveRoom_inside(mail *pack, thRoomArg *data);

/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX(thRoomArg *info);

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI TX **/


/** SEND PACK_inside e WRITE PACK_inside**/

int readPack_inside(int fdPipe, mail *pack);

int writePack_inside(int fdPipe, mail *pack);

int testConnection_inside(int fdPipe);

void freeUserArg(thUserArg *p);

void freeRoomArg(thRoomArg *p);


#endif //CHAT_MULTILEVEL_THFUNX_H
