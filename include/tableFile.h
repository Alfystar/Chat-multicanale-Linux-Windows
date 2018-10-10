//
// Created by alfylinux on 20/09/18.
//

// Versione 1.0

#ifndef FILETAB_DEMO_TABLEFILE_H
#define FILETAB_DEMO_TABLEFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


#define nameFirstFreeSize 50
#define nameEntrySize 28

extern int fdOut;  //pipe di uscita per lo stdOut


typedef struct firstFree_ {
	char name[nameFirstFreeSize];      //nomi da 19 caratteri per il nome della tabella
	int counter;        //numero di caselle libere nel file
	int len;
	int nf_id;      //indice nel'array delle entry che contiene la prima casella libera
} firstFree;

typedef struct entry_ {
	char name[nameEntrySize];      //nomi da 27 caratteri
	int point;          //se roomPath='\0' indica la sucessiva entry libera, se roomPath="xxx" indica in quel file l'attuale file dove si trova
} entry;

typedef struct table_ {
	firstFree head;
	entry *data;    //array da malloccare contenente le entry della tamella
	FILE *stream;
} table;         /// il tipo table serve per avere una copia in ram concui lavorare

/** Prototipi **/

/// Funzioni di Interfaccia operanti su Tabella
table *init_Tab(char *path_file, char *name_tab);

table *open_Tab(char *path_file);

int addEntry(table *table, char *name, int data);

int delEntry(table *table, int index);

table *compressTable(table *table);

int searchFirstEntry(table *tabel, char *search);

int searchEntryBy(table *tabel, char *search, int startIndex);


/// Funzioni di supporto operanti sul file
FILE *openTabF(char *path_file);

int setUpTabF(FILE *fdTable, char *name_Table);

int addEntryTabF(FILE *fdTable, char *name, int data);

int delEntryTabF(FILE *fdTable, int index);

int entrySeekF(FILE *fdTable, int indexEntry);

size_t lenTabF(FILE *fdTable);

int fileWrite(FILE *fdTable, size_t sizeElem, int nelem, void *dataWrite);


///Show funciton
void firstPrint(firstFree *head);

void entryPrint(entry *en);

void tabPrint(table *tab);

void tabPrintFile(FILE *fdTable);

///funzioni di supporto
int isLastEntry(entry *en);

int isEmptyEntry(entry *en);

char *booleanPrint(int val);

table *makeTable(FILE *fdTable);

void freeTable(table *table);

#endif //FILETAB_DEMO_TABLEFILE_H


/** Struttura delle entry di una tabella **/
/*
 * queste tabelle sono usate dalle chat e dagli utenti, ogniuno conInfo la sua.
 * [1]  Le chat tengono traccia di chi è iscritto alla chat
 * [2]  Gli utenti le usano per ricordare a quali chat sono iscritti
 *
 *          ||||-- Entry parameter documentation --||||
 *
 * 1)   Il parametro "roomPath" contiene il nome dell'utente/chat
 * 2)   Il parametro "point" contiene l'indice della entry in cui ci si trova l'utente/chat nella tabella della chat/utente
 * 3)   Se "roomPath" è nullo allora "point" indica nell'attuale tabella la prossima entry libera
 * 4)   Se "roomPath" è nullo e  "point" a -1 allora si tratta di LAST-Entry
 *
 *       ||||-- Firts-Free parameter documentation --||||
 *
 * 1)   Il parametro "roomPath" contiene il nome dell'utente/chat a cui è riferita la tabella
 * 2)   Il parametro "counter" contiene il numero di caselle vuote nel file (utile a decidere quando compattare il file)
 * 3)   Il parametro "len" è il numero di caselle totali presenti nel file
 * 4)   Il parametro "nf_id" i contiene l'indice della entry in cui ci si trova la prima casella libera
 *      se tutta la tabella è occupata, allora punta a Last-Entry
 *
 * [!]  è imperativo che i dati mantengano di consistenza
 *
 *
 * index
 *       --------------
 *  [-]  | first-free |       nome tabella; punta NULL1---------------\
 *       --------------                                               |
 *  [0]  |    DATA    |                                               |
 *       --------------                                               |
 *  [1]  |    DATA    |                                               |
 *       --------------                                               |
 *  [2]  |    NULL2   |       nome NULL; punta LAST-ENTRY---------\   |
 *       --------------                                           |   |
 *  [3]  |    DATA    |       ^                                   |   |
 *       --------------       |                                   |   |
 *  [4]  |    NULL1   |       nome NULL; punta NULL2 <------------+---/
 *       --------------                                           |
 *  [5]  | LAST-ENTRY |  <----------------------------------------/
 *       --------------
 *
 * una struttura simile fa sì che in caso di cancellazione lo spazio rimane disponibile e non si perde
 * se tutta la tabella è occupata allora last-entry divenda un dato, e la tabella cresce di un livello diventando il nuovo livello last-entry
 *
 * ADD-Entry
 * in caso di aggiuta, si accede a first-free e si vede dove posizionare, se è un dato lo si sovrascrive,
 * in caso sia last-Free, la si sovrascrive e al file viene aggiunto una nuova casella seduta stante, la nuova lastFree
 *
 * DELETE-Entry
 * ci si posiziona all'index indicato, la si fa diventare una casella senza nome
 * viene copiato il valore di nf_id dentro point e nf_id diventa l'indice della casella attuale
 *
 */
