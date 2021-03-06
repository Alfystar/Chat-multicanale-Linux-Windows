//
// Created by alfylinux on 20/09/18.
//

#include "../include/tableFile.h"

/// Funzioni di Interfaccia operanti su Tabella

/** La funzione ha lo scopo di creare una tabella completamente nuova in memoria **/
table *init_Tab (char *path, char *nameFirst){
	//path è l'indirizzo della cartella/NOME_FILE sulla quale creare la tabella
	//roomPath è il nome scritto in first-Free
	//nameFirst = nome del scritto in first free
	table *t;
	FILE *f;
	f = openTabF (path);
	if (setUpTabF (f, nameFirst)){
		char buf[128]; // buff per creare la tringa di errore dinamicamente
		switch (errno){
			case EEXIST:
				//tutto ok, posso andare avanti, è una semplice apertura
				break;
			default:

				sprintf (buf, "open file %s, take error:", path);
				perror (buf);
				return NULL;
				break;
		}
	}
	t = makeTable (f);
	return t;
}

table *open_Tab (char *path){
	FILE *f;
	//dprintf(fdDebug,"open_Tab starting\n");
	f = openTabF (path);
	//dprintf(fdDebug,"open_Tab finish\n");

	return makeTable (f);
}

int addEntry (table *t, char *name, int data){
	//-1 errore
	//>=0 posizione

	//name= nome inserito nella nuova cella idKey:name
	//data= il pointer della tabella all'altra tabella
	if (addEntryTabF (t->stream, name, data)){
		//Aggiunta non avvenuta conInfo successo
		return -1;
	}

	int addPosition = t->head.nf_id;
	firstFree *first = &t->head;
	entry *freeData = &t->data[t->head.nf_id];
	if (isLastEntry (freeData))        //se è la fine si cambiano i valori e si crea un nuovo last-entry
	{
		//dprintf(fdDebug, "é un lastFree\n");
		/// last free diventa un dato
		freeData->point = data;
		strncpy (freeData->name, name, nameEntrySize);
		///first-free viene cambiato il luo next-free
		first->nf_id++;
		first->len++;
		first->counter = 1;
		/// viene generato un nuovo last-free a fine file
		/// setup di last
		t->data = reallocarray (t->data, t->head.len, sizeof (entry));
		entry *last = &t->data[t->head.len - 1];
		memset (last->name, 0, nameEntrySize);
		last->point = -1;
	}
	else        //si trasforma in una cella dati e first-free punta la successivaif
	{
		//dprintf(fdDebug, "é una cella deletata\n");

		first->nf_id = freeData->point;   //first free ora punta una nuova casella libera
		first->counter--;
		/// la prima casella libera diventa un dato
		freeData->point = data;
		strncpy (freeData->name, name, nameEntrySize);
	}
	//tabPrint(t);
	return addPosition;
}

int renameFirstEntry (table *t, char *name){
	//preparo quello da sovrascrivere su fileSystem
	firstFree newFr;
	memcpy (&newFr, &t->head, sizeof (firstFree));
	strncpy (newFr.name, name, nameFirstFreeSize);

	flockfile (t->stream);
	fseek (t->stream, 0, SEEK_SET);
	if (fileWrite (t->stream, sizeof (firstFree), 1, &newFr)){
		perror ("Override FirstFree take error:");
		return -1;
	}
	funlockfile (t->stream);

	//se arrivo qua allora il file è stato correttamente modificato
	//rinomino anche quello su ram
	strncpy (t->head.name, name, nameFirstFreeSize);
	return 0;
}

int delEntry (table *t, int index){
	if (delEntryTabF (t->stream, index)){
		//Eliminazione non avvenuta conInfo successo
		dprintf (STDERR_FILENO, "[delEntry]Fail on File-System\n");
		return -1;
	}
	firstFree *first = &t->head;
	entry *delData = &t->data[index];
	if (isEmptyEntry (delData)){
		// è già una cella cancellata, e non devo modificare nulla
		dprintf (fdDebug, "[delEntry]Entry just Empty\n");
		return 0;
	}
	delData->name[0] = 0;  //metto la stringa a ""
	delData->point = first->nf_id;
	first->nf_id = index;
	first->counter++;
	return 0;
}

table *compressTable (table *t){

	if (t->head.counter == 1){
		//il file è già alla dimensione minima
		return 0;
	}
	int enNotEmpty_Id[t->head.len];
	int newLen = 0;
	///ottengo gli id per i quali ho entry valide e anche la nuova lunghezza
	for (int i = 0; i < t->head.len; ++i){
		if (!isEmptyEntry (&t->data[i])){
			//se non è una cella vuota allora posso copiarla
			enNotEmpty_Id[newLen] = i;
			newLen++;
		}
	}

	/// creo una nuova lista della dimensione giusta e ci copio solo le entry non vuote
	entry *newData = (entry *)calloc (newLen, sizeof (entry));
	for (int j = 0; j < newLen; ++j){
		memcpy (&newData[j], &t->data[enNotEmpty_Id[j]], sizeof (entry));
	}
	free (t->data);          //libero la vecchia lista non più utile
	t->data = newData;        //punto la nuova lista creata e compatta
	t->head.len = newLen;     //first aggiorna la lunghezza
	t->head.counter = 1;      //ora sarà presente solo last-free
	t->head.nf_id = newLen - 1; //last-free si trova a len-1

	flockfile (t->stream);
	if (ftruncate (fileno (t->stream), sizeof (firstFree) + sizeof (entry) * newLen)){
		perror ("Tunck File take error:");
		return NULL;
	}
	rewind (t->stream);
	fileWrite (t->stream, sizeof (firstFree), 1, &t->head);
	fileWrite (t->stream, sizeof (entry), newLen, t->data);
	funlockfile (t->stream);

	return t;
}

int searchFirstOccurence (table *t, char *search){
	for (int i = 0; i < t->head.len; i++){
		if (strcmp (t->data[i].name, search) == 0){
			return i;
		}
	}
	return -1;
}

int searchFirstOccurenceKey (table *t, int ID){
	for (int i = 0; i < t->head.len; i++){
		if (atoi (t->data[i].name) == ID){
			return i;
		}
	}
	return -1;
}

int searchEntryBy (table *t, char *search, int idStart){
	for (int i = idStart; i < t->head.len; i++){
		if (strcmp (t->data[i].name, search) == 0){
			return i;
		}
	}
	return -1;
}

/// Funzioni di supporto operanti sul file
FILE *openTabF (char *path){
	/**
	 tab1=fopen(tab1Name,"a+");   // a+ ==│ O_RDWR | O_CREAT | O_APPEND
	 l'interfaccia standard non funziona a causa dell' =_APPEND, che
	 fa SEEEEMPRE scrivere alla fine, ed è un problema, di consegueza creaiamo
	 il fd e succeddivamente il file* per creare una libreria il più possibile
	 standard C
	 Fatto ciò possediamo il nostro file R/W in standard C
	 **/
	int tabFd = open (path, O_RDWR | O_CREAT, 0666);
	if (tabFd == -1){
		perror ("open FD for Tab take error:");
		return NULL;
	}
	FILE *f = fdopen (tabFd, "r+");
	if (f == NULL){
		perror ("tab open error:");
		return NULL;
	}
	return f;

}

int setUpTabF (FILE *tab, char *nameFirst){
	//nameFirst = nome nel first-free
	struct stat tabInfo;
	fstat (fileno (tab), &tabInfo);
	if (tabInfo.st_size != 0)     //il file era già esistente e contiene dei dati
	{
		errno = EEXIST; //file descriptor non valido, perchè il file contiene già qualcosa
		return -1;
	}

	/// setup di firstFree
	firstFree head;
	strncpy (head.name, nameFirst, nameFirstFreeSize);
	head.counter = 1;
	head.len = 1;
	head.nf_id = 0;
	/// setup di last
	entry last;
	memset (last.name, 0, nameEntrySize);
	last.point = -1;

	///File write
	flockfile (tab);
	if (fileWrite (tab, sizeof (firstFree), 1, &head)){
		perror ("Write FirstFree setup take error:");
		return -1;
	}
	dprintf (fdOut, "first-free setup\n");
	if (fileWrite (tab, sizeof (entry), 1, &last)){
		perror ("Write last-entry setup take error:");
		return -1;
	}
	dprintf (fdOut, "last-entry setup\n");
	funlockfile (tab);
	return 0; //all ok
}

int addEntryTabF (FILE *tab, char *name, int data){
	/** L'operazione è eseguita in modo atomico rispetto ai Tread del processo **/
	firstFree first;
	entry freeData;

	flockfile (tab);

	rewind (tab);        //posiziono il seek all'inizio per leggere la prima entry libera
	fflush (tab);    //garantisco che tutto quello che va scritto venga scritto nel file e quindi letto
	fread (&first, 1, sizeof (firstFree), tab);
	int enSeek = entrySeekF (tab, first.nf_id);
	if (enSeek == -1){
		perror ("in addEntryTabF entrySeekF take error:");
		dprintf (fdOut, "index richiesto:%d\n", first.nf_id);
		exit (-1);
	}

	fseek (tab, enSeek, SEEK_SET);
	fread (&freeData, 1, sizeof (freeData), tab);

	if (isLastEntry (&freeData))        //se è la fine si cambiano i valori e si crea un nuovo last-entry

	{
		/// last free diventa un dato
		freeData.point = data;
		strncpy (freeData.name, name, nameEntrySize);

		/// viene generato un nuovo last-free a fine file
		/// setup di last
		entry last;
		memset (last.name, 0, nameEntrySize);
		last.point = -1;

		///first-free viene cambiato il luo next-free
		first.nf_id++;
		first.len++;
		first.counter = 1;

		///File write
		fseek (tab, enSeek, SEEK_SET);
		if (fileWrite (tab, sizeof (entry), 1, &freeData)){
			perror ("Write entry empty take error:");
			return -1;
		}
		if (fileWrite (tab, sizeof (entry), 1, &last)){
			perror ("Write new-last-entry  take error:");
			return -1;
		}
		fseek (tab, 0, SEEK_SET);
		if (fileWrite (tab, sizeof (firstFree), 1, &first)){
			perror ("Override FirstFree take error:");
			return -1;
		}
		fflush (tab);

	}
	else        //si trasforma in una cella dati e first-free punta la successiva
	{
		first.nf_id = freeData.point;   //first free ora punta una nuova casella libera
		first.counter--;
		/// la prima casella libera diventa un dato
		freeData.point = data;
		strncpy (freeData.name, name, nameEntrySize);
		fseek (tab, enSeek, SEEK_SET);
		if (fileWrite (tab, sizeof (entry), 1, &freeData)){
			perror ("Override entry take error:");
			return -1;
		}
		rewind (tab);
		if (fileWrite (tab, sizeof (firstFree), 1, &first)){
			perror ("Override FirstFree take error:");
			return -1;
		}
		fflush (tab);

	}

	funlockfile (tab);

	return 0;
}

int delEntryTabF (FILE *tab, int index){
	firstFree first;
	entry delData;
	int enDelSeek = entrySeekF (tab, index);
	if (enDelSeek == -1){
		perror ("in delEntryTabF entrySeekF take error:");
		dprintf (fdOut, "index richiesto:%d\n", index);
		exit (-1);
	}
	/** L'operazione è eseguita in modo atomico rispetto ai Tread del processo **/
	flockfile (tab);

	fflush (tab);    //garantisco che tutto quello che va scritto venga scritto
	rewind (tab);        //posiziono il seek all'inizio per leggere la prima entry libera
	fread (&first, 1, sizeof (first), tab);
	fseek (tab, enDelSeek, SEEK_SET);
	fread (&delData, 1, sizeof (delData), tab);

	if (isEmptyEntry (&delData)){
		// è già una cella cancellata, e non devo modificare nulla
		return 0;
	}

	delData.name[0] = 0;  //metto la stringa a ""
	delData.point = first.nf_id;
	first.nf_id = index;
	first.counter++;

	fseek (tab, enDelSeek, SEEK_SET);
	if (fileWrite (tab, sizeof (entry), 1, &delData)){
		perror ("Override entry on delete take error:");
		return -1;
	}
	rewind (tab);
	if (fileWrite (tab, sizeof (firstFree), 1, &first)){
		perror ("Override FirstFree on delete take error:");
		return -1;
	}

	funlockfile (tab);
	return 0;
}

int entrySeekF (FILE *tab, int i){
	struct stat tabInfo;
	fstat (fileno (tab), &tabInfo);

	int seek =
			sizeof (firstFree) + sizeof (entry) * i; // se i=0 fseek viene posizionato al byte successivo a first free
	if (seek > tabInfo.st_size - sizeof (entry)) //se seek va oltre la posizione dell'ultima entry DEVE essere errato
	{
		errno = ELNRNG;
		return -1;
	}
#ifdef DEB_STR
	dprintf(fdOut,"Entry seek:\nsize=%d\nindex=%d\nseek=%d\n",tabInfo.st_size,i,seek);
#endif
	return seek;
}

size_t lenTabF (FILE *tab){
	struct stat tabInfo;
	fstat (fileno (tab), &tabInfo);
	return (tabInfo.st_size - sizeof (firstFree)) /
	       sizeof (entry); // nbyte di tipo entry, diviso la dimensione ritorna il numero di elementi
}

int fileWrite (FILE *f, size_t sizeElem, int nelem, void *dat){
	//signal Free
	sigset_t newSet, oltSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oltSet);

	fflush (f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont = 0;
	while (cont != sizeElem * nelem){
		if (ferror (f) != 0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			// è presente un errore in scrittura
			errno = EBADFD;   //file descriptor in bad state
			return -1;
		}
		cont += fwrite (dat + cont, 1, sizeElem * nelem - cont, f);
	}
	pthread_sigmask (SIG_SETMASK, &oltSet, &newSet);   //restora tutto
	return 0;
}

///Show funciton
void firstPrint (firstFree *f){
	dprintf (fdOut, "#1\tfirstFree data Store:\nname\t\t-> %s\ncouterFree\t-> %d\nLen\t\t-> %d\nnextFree\t-> %d\n",
	         f->name, f->counter, f->len, f->nf_id);
	return;
}

void entryPrint (entry *e){
	dprintf (fdOut, "Entry data Store:\n??-Last-Free -> %s\tempty  -> %s\nname\t\t-> %s\npoint\t\t-> %d\n",
	         booleanPrint (isLastEntry (e)), booleanPrint (isEmptyEntry (e)), e->name, e->point);
	return;
}

void tabPrint (table *tab){
	struct stat tabInfo;
	fstat (fileno (tab->stream), &tabInfo);
	if (tabInfo.st_size == 0){
		dprintf (STDERR_FILENO, "File Vuoto, o Inesistente\n");
		return;
	}

	size_t lenFile = lenTabF (tab->stream);

	dprintf (fdOut, "-------------------------------------------------------------\n");
	dprintf (fdOut, "\tLa tabella ha le seguenti caratteristiche:\n\tsize=%ld\n\tlenFile=%ld\tlenFirst=%d\n",
	         tabInfo.st_size, lenFile, tab->head.len);
	dprintf (fdOut, "\tsizeof(entry)=%ld\tsizeof(firstFree)=%ld\n", sizeof (entry), sizeof (firstFree));
	dprintf (fdOut, "\n\t[][]La tabella contenuta nel file contiene:[][]\n\n");
	firstPrint (&tab->head);
	dprintf (fdOut, "##########\n\n");
	for (int i = 0; i < tab->head.len; i++){
		dprintf (fdOut, "--->entry[%d]:", i);
		entryPrint ((&tab->data[i]));
		dprintf (fdOut, "**********\n");
	}
	dprintf (fdOut, "-------------------------------------------------------------\n");
	return;
}

void tabPrintFile (FILE *tab){
	table *t = makeTable (tab);
	dprintf (fdOut, "\t#@#@#@[] Print da File System []@#@#@#\n");
	tabPrint (t);
	freeTable (t);
	return;
}

///funzioni di supporto operanti in ram


int isLastEntry (entry *e){
	if (e->name[0] == 0 && e->point == -1) return 1;
	return 0;
}

int isEmptyEntry (entry *e){
	if (e->name[0] == 0 && e->point != -1) return 1;
	return 0;
}

char *booleanPrint (int i){
	if (i) return "True";
	else return "False";
}

table *makeTable (FILE *tab){

	fflush (tab);    //garantisco che tutto quello che va scritto venga scritto

	struct stat tabInfo;
	fstat (fileno (tab), &tabInfo);

	if (tabInfo.st_size == 0){
		dprintf (fdOut, "File Vuoto, o Inesistente\n");
		return NULL;
	}

	size_t len = lenTabF (tab);
	table *t = (table *)malloc (sizeof (table));
	t->data = (entry *)calloc (len, sizeof (entry));
	if (t->data == NULL && len != 0){
		dprintf (STDERR_FILENO, "[makeTable]Error in Calloc\n");
		return NULL;
	}
	//dprintf(fdDebug,"makeTable flock\n");
	flockfile (tab);
	rewind (tab);
	fread (&t->head, 1, sizeof (firstFree), tab);
	fread (t->data, 1, sizeof (entry) * len, tab);
	funlockfile (tab);
	//dprintf(fdDebug,"makeTable unflock\n");

	t->stream = tab;
	return t;
}

void freeTable (table *t){
	free (t->data);
	free (t);
}

void closeTable (table *t){
	fclose (t->stream);
	freeTable (t);
}