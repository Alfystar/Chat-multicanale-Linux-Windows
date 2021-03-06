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

/*library for message tag*/
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>


#include "fileSystemUtylity.h"
#include "mexData.h"
#include "tableFile.h"
#include "socketConnect.h"
#include "terminalShell.h"

#include "../treeFunx/include/avl.h"
#include "../treeFunx/include/dlist.h"
#include "../globalSet.h"

typedef struct thAcceptArg_{
	int id;
	thConnArg conInfo;
} thAcceptArg;

#define stringLen 50

typedef struct thUserArg_{
	int id;             //idKey dell'user definito in fase di login
	char userName[stringLen];  // Nome dell'user (dopo :)
	char idNameUs[stringLen];
	infoUser *info;     // contiene tabella e nome per esteso (path), assegnato in fase di login
	thConnArg conUs;    //contiene Tab e path dir dell'user
	long fdMSGPipe;      //contiene i valori delle pipe per parlare alla room, Definita da thRoom-generico
	pthread_t tidRx, tidTx;
	int keyIdRmFF, pipeRmFF; //parametri della room a cui inoltrare mex incoming
} thUserArg;

typedef struct thRoomArg_{
	int id;
	char roomName[stringLen];  // Nome dell'user (dopo :)
	char idNameRm[stringLen];
	char roomPath[stringLen];  // Path completo room  ("./%s/%s", chatDirName, chats->names[i]); e chats->names = %d:%s
	infoChat *info;     // contiene tabella,Conversazione e nome per esteso (path), assegnato in fase di creazione
	long fdMSGPipe;      //contiene i valori delle pipe per parlare alla room, Definita da thRoom-generico
	pthread_t tidRx, tidTx;
	listHead_S mailList;  // è la coda dove viene segnato a chi si deve inoltrare i messaggi in arrivo
} thRoomArg;
enum insidePack{
	in_join_p = 1000, in_delRm_p, in_leave_p, in_entryIndex_p, in_openRm_p, in_exit_p, in_kConv_p, in_mess_p,
};

#define noTag 0
#define txTag 1
#define rxTag 2

typedef struct{
	long mDest;     //In base alla priorità stabilisco se il th di destinazione è un Tx o un Rx
	mail pack;
} msg;


typedef struct listData_{
	int keyId;
	long fdPipeSend;
} listData, *listData_p;


///GLOBAL VARIABLE

/** [][][][][][][][] PROTOTIPI [][][][][][][][]  **/

/** TH-ACCEPT, GENERA TH USER IN FASE DI ACCEPT **/
void *acceptTh (thAcceptArg *);

/** [][][] TH-USER GENERICO, prima di specializzarsi, HA IL COMPITO DI LOGIN **/
void *userTh (thConnArg *);
/* FUNZIONI PER IL TH-USER GENERICO, prima di specializzarsi */
int loginServerSide (mail *pack, thUserArg *data);
int setUpThUser (int keyId, thUserArg *argUs);
int mkUserServerSide (mail *pack, thUserArg *data);
void *sendTab (table *t, int *len);

/** #### TH-USER SUL SERVER CON RUOLO DI RX **/
void *thUs_ServRX (thUserArg *);
/* FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX */

int mkRoomSocket (mail *pack, thUserArg *uData);
void makeThRoom (int keyChat, char *roomPath, infoChat *info);
int joinRoomSocket (mail *pack, thUserArg *uData);
int delRoomSocket (mail *pack, thUserArg *uData);
int leaveRoomSocket (mail *pack, thUserArg *uData);
int openRoomSocket (mail *pack, thUserArg *uData);
int exitRoomSocket (mail *pack, thUserArg *data);
int mexReciveSocket (mail *pack, thUserArg *data);

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thUs_ServTX (thUserArg *);

/* FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI TX */
int delRoomForwarding (mail *pack, thUserArg *data);

int mexForwarding (mail *pack, thUserArg *data);

/*==============================================================*/
/*==============================================================*/
/*==============================================================*/


/** [][][] TH-ROOM GENERICO, prima di specializzarsi, HA IL COMPITO DI creare le strutture **/
void *roomTh (thRoomArg *);

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX (thRoomArg *rData);
/* FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX */
int joinRoom_inside (mail *pack, thRoomArg *data);
int delRoom_inside (mail *pack, thRoomArg *data);
int leaveRoom_inside (mail *pack, thRoomArg *data, int *exit);
int openRoom_inside (mail *pack, thRoomArg *data);
int exitRoom_inside (mail *pack, thRoomArg *data);

/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX (thRoomArg *rData);
/* FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI TX **/
int mexRecive_inside (mail *pack, thRoomArg *data);
/** SEND PACK_inside e WRITE PACK_inside**/

int readPack_inside (long fdMSG, int targetTag, mail *pack);
int writePack_inside (long fdMSG, int targetTag, mail *pack);
int closeMSG (long fd);
void freeUserArg (thUserArg *p);
void freeRoomArg (thRoomArg *p);

/** List utility**/
dlist_p makeNode (int keyId, long fdPSend);
dlist_p nodeSearchKey (listHead_S_p head, int key);
int deleteNodeByList (listHead_S_p head, dlist_p nodeDel);
void printDlist (dlist_pp head, int fd);
char *typeToText (int type);
void printPack (mail *pack, int fd);

#endif //CHAT_MULTILEVEL_THFUNX_H
