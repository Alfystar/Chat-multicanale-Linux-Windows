//
// Created by alfylinux on 27/09/18.
//

//Versione 1.0


#ifndef MEXDAT_DEMO_MEXDATA_H
#define MEXDAT_DEMO_MEXDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>

extern int fdOut;  //pipe di uscita per lo stdOut


typedef struct mexInfo_ {
	int usId;
	time_t timeM;
} mexInfo;

typedef struct mex_ {
	mexInfo info;
	char *text;
} mex;

typedef struct convInfo_ {
	int nMex;
	time_t timeCreate;
	int adminId;
} convInfo;


typedef struct conversation_ {
	convInfo head;
	mex **mexList;      //per permettere un add più semplice e variabile è gestita a liste
	FILE *stream;
} conversation;


/**		 Prototipi 		**/

///Funzioni di interfaccia

conversation *initConv(char *path, int adminId);

FILE *openConfStream(char *path);

conversation *openConf(char *convPath);

int addMex(conversation *conversation, mex *message);

mex *makeMex(char *text, int usId);

int endConv(conversation *c);


///Funzioni verso File
int setUpConvF(int adminId, FILE *stream);

int overrideHeadF(convInfo *cI, FILE *stream);

int saveNewMexF(mex *m, FILE *stream);

conversation *loadConvF(FILE *stream);

///Funzioni di supporto
int fWriteF(FILE *stream, size_t sizeElem, int nelem, void *data);

int fReadF(FILE *stream, size_t sizeElem, int nelem, void *save);

int freeMex(mex *m);

time_t currTimeSys();

///Funzioni di visualizzazione
void printConv(conversation *c, int fdOutP);

void printMex(mex *m, int fdOutP);

void printConvInfo(convInfo *cI, int fdOutP);

char *timeString(time_t t);


#endif //MEXDAT_DEMO_MEXDATA_H

/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere pacchetti messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * (i numeri sono i valori decimali dell'equivalente ascii)
 *
 *    Scartato              /---|----|-----|---|-----------|----\
 *    poichè tropo          | 1 | Id | Ora | 2 |   TEXT    |'\0'|
 *    complicato            \---|----|-----|---|-----------|----/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: in ascii indica "End of Trasmission"
 *
 *          OPPURE
 *
 * (+)(+)(+)   /----|-----|-----------|----\
 * (+)(+)(+)   | Id | Ora |   TEXT    |'\0'|       IN USO PER ORA
 * (+)(+)(+)   \----|-----|-----------|----/
 *

 *
 * essendo noi al lavoro su file, quindi siamo sicuri della struttura
 * ogni nuovo 1 è un nuovo messaggio dal quale scopriamo Id e time_t
 * mentre dopo un 2 andiamo a copiare la stringa conInfo strcpy
 *
 *
 * In testa al file è presente un piccolo spazio in memoria dove sono presenti i metadati
 * della conversazione, tipo ora di creazione, amministratore, e numero messaggi presenti
 *
 *
 *
 * CONVERSATION TYPE STRUCTURE:
 *
 *    --------------
 *    |    HEAD    |       [0]         [1]         [2]         [3]
 *    --------------     _______     _______     _______     _______
 *    | **MexList  | -->| *mex1 |-->| *mex2 |-->| *mex3 |-->| *mex3 |
 *    --------------        |               \      |               \
 *    |  *Stream   |        |                \     |                \
 *    --------------        |                 \    |                 \
 *                          |   -------------- \   |   -------------- \
 *                          \-->|    DATA 1  |  \  \-->|    DATA 3  |  \
 *                              --------------   |     --------------   |
 *                                               +                      +
 *                                         --------------         --------------
 *                                         |    DATA 2  |         |    DATA 4  |
 *                                         --------------         --------------
 *
 * Nel file viene salvata la testa e i dati puntati da mexList vengono incolonnati uno dietro l'altro
 * la forma di ogni DATA è quella dei messaggi sopra descritti, HEDER+Text, e in lettura si fa il procedimento
 * opposto per ottenere una struttura di tipo conversation
 *
 */