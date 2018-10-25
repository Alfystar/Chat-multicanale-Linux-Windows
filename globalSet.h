//
// Created by alfylinux on 29/07/18.
//
#ifndef CHAT_MULTILEVEL_GLOBALSETS_H
#define CHAT_MULTILEVEL_GLOBALSETS_H


/*GLOBAL INCLUDE*/
#include <signal.h>

/*GLOBAL DEFINE*/

#define firmwareVersion "0.06"
#define userDirName "Users"
#define chatDirName "Chats_ROOM"
#define serverConfFile "serverStat.conf"

#define chatTable "chatTableList.tab"
#define userTable "userTableList.tab"
#define chatConv "chatConversation.conv"



/** Define di setup delle funx **/
#define nAcceptTh 2

enum colorText {
    Titoli = 1, Comandi, ViewPan, StdoutPrint, ErrorPrint, DebugPrint
};

/** SERVER STORAGE PATH **/
char *storagePathServer;

/** SERVER CONNECT IP AND PORT**/
int portProces; //port of process

/** GLOBAL PIPE TO TALK **/
/*
 * LaS libreria ncurse usa lo stdout per printare gli schermi, di conseguenza redirizzando il flusso si perde
 * la possibilità di visualizzare a schermo le finestre.
 * Per lo stdErr non è così, di conseguenza vengono create 2 pipe, in cui quella dello stdErr viene redirezionata
 * a un thread per farla visualizzare quando serve, mentre se si vuole printare a schermo delle informazioni normali
 * si deve usare la pipe Stdout la quale ha dietro un thread che si occupa di visualizzare la cosa
 */

#define readPipe 0
#define writePipe 1

int FdStdOutPipe[2];  // dal manuale: FdStdOutPipe[0] refers to the read end of the pipe. FdStdOutPipe[1] refers to the write end of the pipe.
int FdStdErrPipe[2];
int FdDebugPipe[2];
int fdOut;           //il riferimento per scrivere [dprintf(...)]
int fdDebug;


/** AVL TREEs Thread-Safe**/
///i nodi a livello informativo possiedono:
/// key :=keyId
/// data := Fd of Pipe to write at the thread same-things
#include "treeFunx/include/avl.h"

avl_pp_S usAvlTree_Pipe;
avl_pp_S rmAvlTree_Pipe;

#endif //CHAT_MULTILEVEL_DEFINESETS_H
