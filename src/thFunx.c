//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"


/** TH-ACCEPT, GENERA TH USER IN FASE DI ACCEPT **/

void *acceptTh(thAcceptArg *info) {

	thUserArg *arg;
	dprintf(fdOut, "acceptTh Creato\n");

	while (1) {
		arg = malloc(sizeof(thUserArg));
		//definisco gli argomenti da passare al th user come puntati da arg di thConnArg.arg
		arg->id = -1;
		arg->conUs = info->conInfo; //copia negli argomenti del th una copia totale della connessione del server
		arg->info = NULL;
		arg->userName[0] = 0;

		if (acceptCreate(&info->conInfo.con, userTh, arg) == -1) {
			dprintf(STDERR_FILENO, "errore in accept\n");
		}
		dprintf(fdOut, "userTh Creato da accept,i suoi arg = %p sono stati creati.\n", arg);

	}
	free(info);
	return NULL;
}

/** TH-/** #### TH-USER GENERICO, prima di specializzarsi, HA IL COMPITO DI LOGIN **/
void *userTh(thConnArg *info) {
	thUserArg *arg = info->arg; //da impostare in base al login, l'accept lascia campi "vuoti"
	arg->conUs.con = info->con; //copio i dati di info
	free(info); //il tipo thConnArg non serve più tutto è stato copiato

	dprintf(fdDebug, "TH-User creato, In Attesa primo pack\n");
	//pause();
	mail *pack = malloc(sizeof(mail));
	mail responde;

	readPack(arg->conUs.con.ds_sock, pack);        //ottengo il primo pack per capire il da fare.
	///in base al tipo di connessione si riepe arg con i giusti dati
	switch (pack->md.type) {
		case login_p:
			if (loginServerSide(pack, arg)) {
				perror("login fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				fillPack(&responde, failed_p, 0, 0, "Server", "Impossibile loggare");
				writePack(arg->conUs.con.ds_sock, &responde);
				pthread_exit(NULL);
			}
			break;
		case mkUser_p:
			if (mkUserServerSide(pack, arg)) {
				perror("make New User fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				fillPack(&responde, failed_p, 0, 0, "Server", "Can't create user");
				writePack(arg->conUs.con.ds_sock, &responde);
				pthread_exit(NULL);
			}
			break;
		default:
			dprintf(STDERR_FILENO, "Pack rivevuto inadatto a instaurazione com\n");
			fillPack(pack, failed_p, 0, NULL, "SERVER", "User Not yet Created");
			writePack(arg->conUs.con.ds_sock, pack);
			//todo in caso salvare possibili file (FUNX saveOnExit)
			pthread_exit(NULL);
			break;
	}

	dprintf(fdDebug, "Login success\nTx-th & Rx-th of %d now Create", arg->id);

	int fdUserPipe[2];
	int errorRet = pipe2(fdUserPipe,
	                     O_DIRECT); // dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	if (errorRet != 0) {
		printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit(-1);
	}


	insert_avl_node_S(usAvlTree_Pipe, arg->id, fdUserPipe[1]);

	pthread_t tidRX, tidTX;
	//i thUser condividono lo stesso oggetto thUserArg arg, quindi stare attenti !!!!
	pthread_create(&tidRX, NULL, thrServRX, arg);
	//pthread_create(&tidTX,NULL, thrServTX,info);

	while (1) pause();
	free(arg);  //todo da sostituire con free_thUserArg quando fatta
	pthread_exit(NULL);

	return NULL;
}

/** FUNZIONI PER IL TH-USER GENERICO, prima di specializzarsi **/
int loginServerSide(mail *pack, thUserArg *data) {
	//imposto i thUserArg correttamente

	dprintf(fdDebug, "LOGIN FASE\nUtente = %s & iDKey=%d\n Type= %d\n", pack->md.sender, atoi(pack->md.whoOrWhy),
	        pack->md.type);
	/*
	 * Login:
	 * type=login_p
	 * Sender=xxxx
	 * whoOrWhy=idKey (string)
	 * dim=0
	 */

	mail response;

	if (setUpThUser(atoi(pack->md.whoOrWhy), data)) {
		perror("Impossible to create new ThUser of register User :");
		fillPack(&response, failed_p, 0, NULL, "Server", "setUpThUser error");

		return -1;
	}

	firstFree *head = &data->info->tab->head;
	entry *cell = data->info->tab->data;

	//for per eliminare room non più esistenti
	for (int i = 0; i < head->len; i++) {
		if ((isEmptyEntry(&cell[i]) || isLastEntry(&cell[i])) == true) {
			continue;
		}
		if (search_BFS_avl_S(rmAvlTree_Pipe, atoi(cell[i].name)) == -2) {
			//name:= keyRoom:NAME  se key non è trovata nell'avl allora la room non esiste più
			dprintf(fdDebug, "Room %s not found in Avl-Room. Deleting from userTab\n", cell[i].name);
			if (delEntry(data->info->tab, i)) {
				dprintf(fdDebug, "Deleting from userTab FAIL\n");
			}
		} else {
			dprintf(fdDebug, "room %s trovata\n", cell[i].name);
		}

	}

	int len;
	void *mex = sendTab(data->info->tab, &len);

	/// Invio risposta affermativa
	fillPack(&response, dataUs_p, len, mex, "Server", NULL);
	writePack(data->conUs.con.ds_sock, &response);
	free(mex);

	return 0;
}

int setUpThUser(int keyId, thUserArg *argUs) {
	//modifica argUs per salvare dentro i file
	/// keyId:= id da cercare       argUs:= puntatore alla struttura da impostare
	nameList *user = userExist();

	int want = idSearch(user, keyId);

	if (want == -1) {
		dprintf(STDERR_FILENO, "Id richiesto inesistente\n");
		errno = ENOENT;
		return -1;
	}

	///Inizio la creazione del thread
	char userDir[128];
	sprintf(userDir, "./%s/%s", userDirName, user->names[want]);

	infoUser *info = openUser(userDir);
	if (info == NULL) {
		dprintf(STDERR_FILENO, "creazione dell'User impossibile\n");
		errno = ENOENT;
		return -1;
	}
	sem_wait(&screewWrite);
	wtabPrint(showPannel, info->tab, 0);
	sem_post(&screewWrite);

	pthread_t usertid;

	argUs->id = keyId;
	argUs->info = info;


	/** Tokenizzazione di user->names[want] per ottenere name **/
	// !!!!rompo user ma tanto non serve più

	char *sArgv[2];  //consento lo storage fino a 64 comandi
	int sArgc = 0;
	char *savePoint;

	sArgc = 0;
	sArgv[sArgc] = strtok_r(user->names[want], ":", &savePoint);
	while (sArgv[sArgc] != NULL && sArgc < 2) {
		sArgc++;
		sArgv[sArgc] = strtok_r(NULL, ":", &savePoint);
	}

	strncpy(argUs->userName, sArgv[1], 50);   //serve a prendere solo il nome dell'utente
	nameListFree(user);

	return 0;
}


int mkUserServerSide(mail *pack, thUserArg *data) {
	//imposto i thUserArg correttamente

	dprintf(fdDebug, "LOGIN FASE, create user\n");
	/*
	 * mkUser:
	 * type=mkUser_p
	 * Sender=Name new (string)
	 * whoOrWhy= null
	 * dim=0
	 */

	mail response;

	infoUser *infoNewUs = newUser(pack->md.sender);
	if (infoNewUs == NULL) {
		dprintf(STDERR_FILENO, "creazione della chat impossibile");
		return -1;
	}
	data->info = infoNewUs;
	//todo da vedere se sscanf funziona
	// dentro infoNewUs pathName è 		sprintf(userPath, "./%s/%s", userDirName, nameUser); e nameUser è 	sprintf(nameUser, "%ld:%s", newId, name);
	// => ./%s/%ld:%s
	char userDir[64];   // non mi serve, ma devo tenere i dati di appoggio
	sscanf(infoNewUs->pathName, "./%[^/]/%ld:%s", userDir, &data->id, data->userName);

	dprintf(fdDebug, "infoNewUs->pathName = %s\n", infoNewUs->pathName);
	dprintf(fdDebug, "sscanf ha identificato:\n userDir= %s\ndata->id= %d\ndata->userName =%s\n", userDir, data->id,
	        data->userName);

	int len;
	char *mex = sendTab(infoNewUs->tab, &len);

	/// Invio dataSend
	char idSend[16];
	sprintf(idSend, "%d", data->id);
	fillPack(&response, dataUs_p, len, mex, "Server", idSend);
	writePack(data->conUs.con.ds_sock, &response);

	free(mex);

	return 0;
}

void *sendTab(table *t, int *len) {
	firstFree *head = &t->head;
	entry *cell = t->data;


	int sizeTab = (head->len) * sizeof(entry) + sizeof(firstFree);
	void *mex = malloc(sizeTab);

	memcpy(mex, head, sizeof(firstFree));
	memcpy(mex + sizeof(firstFree), cell, sizeTab - sizeof(firstFree));
	*len = sizeTab;
	return mex;
}


/** #### TH-USER SUL SERVER CON RUOLO DI RX **/
void *thrServRX(thUserArg *argTh) {

	mail packRecive;
	mail packSend;
	bool exit = true;
	char buffRet[128];
	while (exit) {
		dprintf(fdOut, "thrServRx %d in attesa di messaggio da %d sock\n", argTh->id, argTh->conUs.con.ds_sock);

		if (readPack(argTh->conUs.con.ds_sock, &packRecive) == -1) {
			dprintf(STDERR_FILENO, "Read error, broken pipe\n");
			dprintf(STDERR_FILENO, "thrServRx %d in chiusura\n", argTh->id);
			sleep(1);
			free(argTh); //todo creare i free per ogni tipo di dato !!!!
			pthread_exit(-1); // todo gestione broken pipe e uscita thread
		}
		/*
		dprintf(fdDebug, "thrServRx %d ricevuto type=%d\n", packRecive.md.type);
		printPack(&packRecive);
		 */

		switch (packRecive.md.type) {
			case mkRoom_p:
				if (mkRoomSocket(&packRecive, buffRet, 128)) {
					dprintf(STDERR_FILENO, "mkRoom coommand from socket fail\n");
					fillPack(&packSend, failed_p, 0, 0, "Server", "Impossible Create room");
					writePack(argTh->conUs.con.ds_sock, &packSend);
				} else {
					//ho successo, aggiungo la chat alla tabella dell'utente attuale
					//la prima entry delle tabelle è il proprietario (come first free)
					addEntry(argTh->info->tab, buffRet, 0);
					sem_wait(&screewWrite);
					wtabPrint(showPannel, argTh->info->tab, 0);
					sem_post(&screewWrite);

					fillPack(&packSend, success_p, 0, 0, "Server", buffRet);
					writePack(argTh->conUs.con.ds_sock, &packSend);
				}
				break;
			case joinRm_p:

				break;
			case mess_p:
				break;
			case logout_p:
				exit = false;
				continue;
				break;

			default:
				dprintf(fdDebug, "thrServRx %d ricevuto type=%d, NON GESTITO\n", packRecive.md.type);
				printPack(&packRecive);
				break;
		}

		if (!packRecive.mex) free(packRecive.mex);
	}
	close(argTh->conUs.con.ds_sock);
	if (!packRecive.mex) free(packRecive.mex);
	if (!packSend.mex) free(packSend.mex);
	free(argTh); //todo creare i free per ogni tipo di dato !!!!
	pthread_exit(0);
}

/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX **/

int mkRoomSocket(mail *pack, char *nameChatRet, int len) {
	//nameChatRet= puntatore a un buffer dove viene salvato il nome (id:name) della chat creata
	/*
	 * type= mkRoom_p
	 * sender= user (string) //non lo uso
	 * who=id (string)
	 * mex="<nameRoom>
	 */
	nameList *user = userExist();
	//user = idKeyUser:Name
	char nameUsAdmin[64];
	long idKeyUser;       //essendo myname xx:TEXT, la funzione termina ai : e ottengo la key

	int want = idSearch(user, atoi(pack->md.whoOrWhy));
	dprintf(fdDebug, "user->names[want]=%s\n", user->names[want]);
	sscanf(user->names[want], "%ld:%s", &idKeyUser, nameUsAdmin);

	infoChat *info = newRoom(pack->mex, atoi(pack->md.whoOrWhy));
	nameListFree(user);

	if (info == 0) {
		dprintf(STDERR_FILENO, "creazione della chat impossibile");
		nameListFree(user);
		return -1;
	}

	strncpy(nameChatRet, info->myName, len);

	char roomDir[128];
	sprintf(roomDir, "./%s/%s", chatDirName, info->myName);


	//info->myName = idKeyChat:name
	makeThRoom(atoi(info->myName), roomDir, info);
	dprintf(fdDebug, "ROOM-th by idKeyUser=%d\n", idKeyUser);

	return 0;

}

void makeThRoom(int keyChat, char *roomPath, infoChat *info) {
	if (info == NULL) {
		dprintf(STDERR_FILENO, "infoChat NULL, impossibile creare Tr-ROOM\n");
		return;
	}
	pthread_t roomtid;
	thRoomArg *arg = malloc(sizeof(thRoomArg));
	arg->id = keyChat;

	strncpy(arg->roomPath, roomPath, 50);
	arg->info = info;

	int errorRet;
	errorRet = pthread_create(&roomtid, NULL, roomTh, arg);
	if (errorRet != 0) {
		printErrno("La creazione del Thread ROOM ha dato il seguente errore", errorRet);
		exit(-1);
	}

}


int joinRoomSocket(mail *pack, thUserArg *data) {
	/*
	 * con Data ho accesso alla tabella dell'utente in questione
	 * e posso aggiungere alla tabella dell'utente la nuova room alla quale sono collegato
	 *
	 * devo anche inviare al Th-room che esiste un nuovo utente e deve essere aggiunto alla tabella
	 *
	 *
	 * devo ritornare al mio utente in quale indice della tabella della room mi trovo, per settare point
	 *
	 */

	/*
	 * type= joinRm_p
	 * sender=  (string) //non lo uso
	 * who= (string)
	 * mex= <>
	 */


}

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thrServTX(thUserArg *argTh) {

	mail packWrite;
	//fillPack(&packWrite,);

	while (1) {
		//readPack() da aggiungere il selettore in ingresso di chat


		if (writePack(argTh->conUs.con.ds_sock, &packWrite) == -1) {
			dprintf(STDERR_FILENO, "Write error, broken pipe\n");
			exit(-1);
		}

	}
}
/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI TX **/





/** [][][] TH-ROOM GENERICO, prima di specializzarsi, HA IL COMPITO DI creare le strutture **/
void *roomTh(thRoomArg *info) {

	///creo le pipe con cui venir ragiunto

	int fdRoomPipe[2];
	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2(fdRoomPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit(-1);
	}
	insert_avl_node_S(rmAvlTree_Pipe, info->id, fdRoomPipe[1]);

	dprintf(fdOut, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n\tmi ragiungi da %d\n", info->id, info->roomPath,
	        fdRoomPipe[1]);


	pthread_t tidRX, tidTX;
	//i thRoom condividono lo stesso oggetto thRoomArg info, quindi stare attenti
	pthread_create(&tidRX, NULL, thRoomRX, info);
	//pthread_create(&tidTX,NULL, thRoomTx,info);

	while (1) pause();

	free(info);     //todo fare free-thRoomArg
	pthread_exit(NULL);

	return NULL;

}

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX(thRoomArg *info) {

}

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX **/


/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX(thRoomArg *info) {

}
/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI TX **/


/** SEND PACK_inside e WRITE PACK_inside**/

/*
 * I dati inviati tramite mex, devono essere freezati dopo dal ricevente, e allocati a parte prima dal mittente
 */

int readPack_inside(int fdPipe, mail *pack) {
	//rispetto alla versione normale ricevo il puntatore di mex, per cui prima è stato malloccato,e ora il ricevente lo freezerà

	//se mex è presente DEVE essere Freezato fuori

	int iterContr = 0; // vediamo se la read fallisce
	ssize_t bRead = 0;
	ssize_t ret = 0;
	dprintf(fdDebug, "readPack_inside Funx\n");
	do {
		ret = read(fdPipe, &pack->md + bRead, sizeof(mail) - bRead);
		if (ret == -1) {
			perror("Read error; cause:");
			return -1;
		}
		if (ret == 0) {
			iterContr++;
			if (iterContr > 4) {
				dprintf(STDERR_FILENO, "Seems Read can't go further; test connection... [%d]\n", iterContr);
				if (testConnection_inside(fdPipe) == -1) {
					dprintf(STDERR_FILENO, "test Fail\n");
					return -1;
				} else {
					iterContr = 0;
				}
			}
		} else {
			iterContr = 0;
		}
		bRead += ret;
	} while (sizeof(mail) - bRead != 0);

	return 0;
}

int writePack_inside(int fdPipe, mail *pack) //dentro il thArg deve essere puntato un mail
{
	//rispetto alla versione normale invio il puntatore di mex, per cui prima lo si mallocca ,e il ricevente lo freeza
	/// la funzione si aspetta che il buffer non sia modificato durante l'invio
	ssize_t bWrite = 0;
	ssize_t ret = 0;

	do {
		ret = send(fdPipe, pack + bWrite, sizeof(mail) - bWrite, MSG_NOSIGNAL);
		if (ret == -1) {
			if (errno == EPIPE) {
				dprintf(STDERR_FILENO, "write pack pipe break 1\n");
				return -1;
				//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
			}
		}
		bWrite += ret;

	} while (sizeof(mail) - bWrite != 0);

	return 0;
}

int testConnection_inside(int fdPipe) {
	mail packTest;
	fillPack(&packTest, test_p, 0, NULL, "SERVER", "testing_code");

	if (writePack_inside(fdPipe, &packTest) == -1) {
		return -1;
	}
	return 0;
}