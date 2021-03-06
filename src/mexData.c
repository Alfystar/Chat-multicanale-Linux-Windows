//
// Created by alfylinux on 27/09/18.
//

#include "../include/mexData.h"

///Funzioni di interfaccia

conversation *initConv (char *path, int adminId){
	conversation c;
	c.stream = openConfStream (path);
	if (setUpConvF (adminId, c.stream)){
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

	return loadConvF (c.stream);
}

FILE *openConfStream (char *path){
	int confFd = open (path, O_RDWR | O_CREAT, 0666);
	if (confFd == -1){
		perror ("open FD for Conv take error:");
		return NULL;
	}
	FILE *f = fdopen (confFd, "r+");
	if (f == NULL){
		perror ("Conv open error:");
		return NULL;
	}
	return f;
}

conversation *openConf (char *convPath){
	FILE *f = openConfStream (convPath);
	if (f == NULL){
		perror ("Conv open error:");
		return NULL;
	}
	return loadConvF (f);
}

convRam *copyConv (conversation *c){
	convRam *cR = calloc (1, sizeof (convRam));
	if (!cR){
		return NULL;
	}
	memcpy (&cR->head, &c->head, sizeof (convInfo));
	cR->mexList = calloc (c->head.nMex, sizeof (mex *));
	if (!cR->mexList){
		free (cR);
		return NULL;
	}
	mex *mexNode;

	for (int i = 0; i < cR->head.nMex; i++){
		///genero un nuovo nodo dei messaggi in ram
		mexNode = malloc (sizeof (mex));
		if (!mexNode){

			for (int j = 0; j < i; j++){
				freeMex (cR->mexList[j]);
			}
			free (cR->mexList);
			free (cR);

			return NULL;
		}
		///Copio i metadati
		memcpy (mexNode, &c->mexList[i]->info, sizeof (mexInfo));

		///Crea in ram la stringa di dim arbitraria e mex la punta
		mexNode->text = malloc (strlen (c->mexList[i]->text) + 1);
		if (!mexNode->text){
			for (int j = 0; j < i; j++){
				freeMex (cR->mexList[j]);
			}
			free (cR->mexList);
			free (cR);
			return NULL;
		}
		strcpy (mexNode->text, c->mexList[i]->text);

		cR->mexList[i] = mexNode;   //salvo il puntatore nell'array
	}
	return cR;
}

int addMex (conversation *c, mex *m){
	if (saveNewMexF (m, c->stream)){
		perror ("[addMex]error during write :");
		return -1;
	}
	c->head.nMex++;
	if (overrideHeadF (&c->head, c->stream)){
		dprintf (STDERR_FILENO, "[addMex]Override fail\n");
		return -1;
	}
	if (c->head.nMex <= 1) c->mexList = calloc (1, sizeof (mex *));
	else c->mexList = reallocarray (c->mexList, c->head.nMex, sizeof (mex *));
	if (c->mexList == NULL){
		dprintf (STDERR_FILENO, "[addMex]calloc/reallocarray fail(nMex = %d)\n", c->head.nMex);
		return -1;
	}
	c->mexList[c->head.nMex - 1] = m;

	return 0;
}

mex *makeMex (char *text, int usId){
	/// text su un buf temporaneo
	mex *m = malloc (sizeof (mex));
	if (m == NULL){
		return NULL;
	}
	//dprintf (fdDebug, "[makeMex]TESTO DEL MESSAGGIO:\n%s\n", text);
	m->info.usId = usId;
	m->info.timeM = currTimeSys ();
	m->text = malloc (strlen (text) + 1);
	strcpy (m->text, text);
	return m;
}

mex *makeMexBuf (size_t len, char *bufMex){
	bufMex[len - 1] = 0; //non dovrebbe essere necessario, per sicurezza
	/// text su un buf temporaneo
	mex *m = malloc (sizeof (mex));
	if (m == NULL){
		return NULL;
	}
	m->text = malloc (len - sizeof (mexInfo));
	if (!m->text){
		free (m);
		return NULL;
	}
	memcpy (&m->info, bufMex, sizeof (mexInfo));
	strcpy (m->text, bufMex + sizeof (mexInfo));
	return m;
}

int freeConv (conversation *c){

	//libero tutti i messaggi
	for (int i = 0; i < c->head.nMex; i++){
		freeMex (c->mexList[i]);
	}
	fclose (c->stream);
	free (c);    //dopo aver liberato e chiuso tutto libero la memoria
	return 0;
}

///Funzioni verso File

int setUpConvF (int adminId, FILE *stream){
	struct stat streamInfo;
	fstat (fileno (stream), &streamInfo);
	if (streamInfo.st_size != 0)     //il file era già esistente e contiene dei dati
	{
		errno = EEXIST; //file descriptor non valido, perchè il file contiene già qualcosa
		return -1;
	}
	conversation newCon;
	newCon.head.adminId = adminId;
	newCon.head.nMex = 0;
	newCon.head.timeCreate = currTimeSys ();
	overrideHeadF (&newCon.head, stream);
	return 0;
}

int overrideHeadF (convInfo *cI, FILE *stream){
	flockfile (stream);
	rewind (stream);
	if (fWriteF (stream, sizeof (convInfo), 1, cI)){
		funlockfile (stream);
		perror ("fwrite fail in overrideHeadF");
		return -1;
	}
	fflush (stream);
	funlockfile (stream);

	return 0;
}

int saveNewMexF (mex *m, FILE *stream){
	/// Scrive in maniera atomica rispetto al Processo
	size_t lenText = strlen (m->text) + 1;

	flockfile (stream);
	fseek (stream, 0, SEEK_END); //mi porto alla fine per aggiungere

	if (fWriteF (stream, sizeof (m->info), 1, &m->info)){
		funlockfile (stream);
		return -1;
	}
	if (fWriteF (stream, lenText, 1, m->text)){
		funlockfile (stream);
		return -1;
	}
	funlockfile (stream);
	return 0;
}

conversation *loadConvF (FILE *stream){
	conversation *conv = malloc (sizeof (conversation));
	conv->stream = stream;
	///accedo al file e lo copio in un buffer in blocco
	flockfile (stream);
	fflush (stream);
	struct stat streamInfo;
	fstat (fileno (stream), &streamInfo);
	if (streamInfo.st_size == 0){
		funlockfile (stream);
		conv->head.nMex = 0;
		conv->mexList = NULL;
		return conv;
	}
	char buf[streamInfo.st_size];
	rewind (stream);
	fReadF (stream, 1, streamInfo.st_size, buf);
	funlockfile (stream);
	///accesso eseguito inizio il compattamento dati

	void *dataPoint;

	///copio i dati di testa

	memcpy (&conv->head, buf, sizeof (conv->head));
	dataPoint = buf + sizeof (conv->head);
	if (streamInfo.st_size == sizeof (conv->head)){
		//non sono presenti messaggi e ho una conversazione vuota
		dprintf (fdOut, "[loadConvF]File conInfo solo testa\n");
		conv->mexList = NULL;
		return conv;
	}
	conv->mexList = calloc (conv->head.nMex, sizeof (mex *));   //creo un array di puntatori a mex
	if (conv->mexList == NULL && conv->head.nMex != 0){
		dprintf (STDERR_FILENO, "[loadConvF]Error in Calloc\n");
		return NULL;
	}
	mex *mexNode;
	size_t len;
	for (int i = 0; i < conv->head.nMex; i++){
		///genero un nuovo nodo dei messaggi in ram
		mexNode = malloc (sizeof (mex));

		///Copio i metadati
		memcpy (mexNode, dataPoint, sizeof (mexInfo));
		dataPoint += sizeof (mexInfo);
		///Crea in ram la stringa di dim arbitraria e mex la punta
		len = strlen (dataPoint) + 1;
		mexNode->text = malloc (len);
		strcpy (mexNode->text, dataPoint);
		dataPoint += len;

		conv->mexList[i] = mexNode;   //salvo il puntatore nell'array
	}
	return conv;
}

///Funzioni di supporto

int fWriteF (FILE *f, size_t sizeElem, int nelem, void *dat){
	//signal Free
	sigset_t newSet, oltSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oltSet);
	fflush (f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W

	fflush (f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont = 0;
	while (cont != sizeElem * nelem){
		if (ferror (f) != 0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			// è presente un errore in scrittura
			errno = EBADFD;   //file descriptor in bad state
			return -1;
		}
		//dprintf(fdOut,"prima fwrite; dat=%p\n",dat);
		cont += fwrite (dat + cont, 1, sizeElem * nelem - cont, f);
	}
	pthread_sigmask (SIG_SETMASK, &oltSet, &newSet);   //restora tutto
	return 0;
}

int fReadF (FILE *f, size_t sizeElem, int nelem, void *save){
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
		cont += fread (save + cont, 1, sizeElem * nelem - cont, f);
	}
	pthread_sigmask (SIG_SETMASK, &oltSet, &newSet);   //restora tutto
	return 0;
}

int freeMex (mex *m){
	free (m->text);
	free (m);
	return 0;
}

time_t currTimeSys (){
	time_t current_time;

/* Obtain current time. */
	current_time = time (NULL);

	if (current_time == ((time_t)-1)){
		fprintf (stderr, "Failure to obtain the current time.\n");
	}
	return current_time;
}

///Funzioni di visualizzazione

void printConv (conversation *c, int fdOutP){
	dprintf (fdOutP, "-------------------------------------------------------------\n");
	dprintf (fdOutP, "\tLa Conversazione ha salvati i seguenti messaggi:\n");
	dprintf (fdOutP, "\tsizeof(mex)=%ld\tsizeof(mexInfo)=%ld\tsizeof(convInfo)=%ld\n", sizeof (mex), sizeof (mexInfo),
	         sizeof (convInfo));
	dprintf (fdOutP, "FILE stream pointer\t-> %p\n", c->stream);
	dprintf (fdOutP, "\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo (&c->head, fdOutP);

	//mex *currMex=c->mexList;
	dprintf (fdOutP, "##########\n\n");

	for (int i = 0; i < c->head.nMex; i++){
		dprintf (fdOutP, "--->Mex[%d]:\n", i);
		printMex (c->mexList[i], fdOutP);
		dprintf (fdOutP, "**********\n");
	}
	dprintf (fdOutP, "-------------------------------------------------------------\n");
	return;
}

void printConvRam (convRam *c, int fdOutP){
	printf ("-------------------------------------------------------------\n");
	printf ("\tLa Conversazione ha salvati i seguenti messaggi:\n");
	printf ("\tsizeof(mex)=%ld\tsizeof(mexInfo)=%ld\tsizeof(convInfo)=%ld\n", sizeof (mex), sizeof (mexInfo),
	        sizeof (convInfo));
	printf ("FILE stream pointer\t-> NULL (only ram version)\n");
	printf ("\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo (&c->head, fdOutP);

	printf ("##########\n\n");

	for (int i = 0; i < c->head.nMex; i++){
		printf ("--->Mex[%d]:\n", i);
		printMex (c->mexList[i], fdOutP);
		printf ("**********\n");
	}
	printf ("-------------------------------------------------------------\n");
	return;
}

void printMex (mex *m, int fdOutP){
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	dprintf (fdOutP, "Mex data Store locate in=%p:\n", m);
	dprintf (fdOutP, "info.usId\t-> %d\n", m->info.usId);
	dprintf (fdOutP, "time Message\t-> %s", timeString (m->info.timeM));
	if (m->text){
		dprintf (fdOutP, "Text:\n-->  %s\n", m->text);
	}
	else{
		dprintf (fdOutP, "Text: ##Non Presente##\n");
	}
}

void printMexBuf (char *buf, int fdOutP){
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	mex m;
	memcpy (&m, buf, sizeof (mexInfo));
	m.text = buf + sizeof (mexInfo);
	dprintf (fdOutP, "Mex BUF data Store locate in=%p:\n", buf);
	dprintf (fdOutP, "info.usId\t-> %d\n", m.info.usId);
	dprintf (fdOutP, "time Message\t-> %s", timeString (m.info.timeM));
	if (m.text != NULL){
		dprintf (fdOutP, "Text:\n-->  %s\n", m.text);
	}
	else{
		dprintf (fdOutP, "Text: ##Non Presente##\n");
	}
}

void printConvInfo (convInfo *cI, int fdOutP){
	/*
	cI->nMex
	cI->adminId
	cI->timeCreate
	*/
	dprintf (fdOutP, "#1\tConversation info data Store:\n");
	dprintf (fdOutP, "nMess\t\t-> %d\n", cI->nMex);
	dprintf (fdOutP, "adminId\t\t-> %d\n", cI->adminId);
	dprintf (fdOutP, "Time Creat\t-> %s\n", timeString (cI->timeCreate));
}

char *timeString (time_t t){
	char *c_time_string;
/* Convert to local time format. */
	//dprintf(fdOut,"ctime before\n");
	c_time_string = ctime (&t);   /// è thread safe, ha una memoria interna
	//dprintf(fdOut,"ctime after\n");
	if (c_time_string == NULL){
		fprintf (stderr, "Failure to convert the current time.\n");
	}
	return c_time_string;

}