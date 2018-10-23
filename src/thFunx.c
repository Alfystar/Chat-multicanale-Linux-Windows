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
		arg->fdPipe[0] = -1;
		arg->fdPipe[1] = -1;

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
		case out_login_p:
			if (loginServerSide(pack, arg)) {
				perror("login fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				fillPack(&responde, out_failed_p, 0, 0, "Server", "Impossibile loggare");
				writePack(arg->conUs.con.ds_sock, &responde);
				pthread_exit(NULL);
			}
			break;
		case out_mkUser_p:
			if (mkUserServerSide(pack, arg)) {
				perror("make New User fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				fillPack(&responde, out_failed_p, 0, 0, "Server", "Can't create user");
				writePack(arg->conUs.con.ds_sock, &responde);
				pthread_exit(NULL);
			}
			break;
		default:
			dprintf(STDERR_FILENO, "Pack rivevuto inadatto a instaurazione com\n");
			fillPack(pack, out_failed_p, 0, NULL, "SERVER", "User Not yet Created");
			writePack(arg->conUs.con.ds_sock, pack);
			//todo in caso salvare possibili file (FUNX saveOnExit)
			pthread_exit(NULL);
			break;
	}

	dprintf(fdDebug, "Login success\nTx-th & Rx-th of %d now Create", arg->id);

	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2(arg->fdPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit(-1);
	}


	insert_avl_node_S(usAvlTree_Pipe, arg->id, arg->fdPipe[writePipe]);

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
	 * type=out_login_p
	 * Sender=xxxx
	 * whoOrWhy=idKey (string)
	 * dim=0
	 */

	mail response;

	if (setUpThUser(atoi(pack->md.whoOrWhy), data)) {
		perror("Impossible to create new ThUser of register User :");
		fillPack(&response, out_failed_p, 0, NULL, "Server", "setUpThUser error");

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
	fillPack(&response, out_dataUs_p, len, mex, "Server", NULL);
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
	 * type=out_mkUser_p
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
	fillPack(&response, out_dataUs_p, len, mex, "Server", idSend);
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
		dprintf(fdDebug, "thrServRx %d ricevuto packetto\n", argTh->id);
		printPack(&packRecive);
		*/
		switch (packRecive.md.type) {
			case out_mkRoom_p:
				if (mkRoomSocket(&packRecive, buffRet, 128)) {
					dprintf(STDERR_FILENO, "mkRoom coommand from socket fail\n");
					fillPack(&packSend, out_failed_p, 0, 0, "Server", "Impossible Create room");
					writePack(argTh->conUs.con.ds_sock, &packSend);
				} else {
					//ho successo, aggiungo la chat alla tabella dell'utente attuale
					//la prima entry delle tabelle è il proprietario (come first free)
					addEntry(argTh->info->tab, buffRet, 0);
					sem_wait(&screewWrite);
					wtabPrint(showPannel, argTh->info->tab, 0);
					sem_post(&screewWrite);

					fillPack(&packSend, out_success_p, 0, 0, "Server", buffRet);
					writePack(argTh->conUs.con.ds_sock, &packSend);
				}
				break;
			case out_joinRm_p:
				if (joinRoomSocket(&packRecive, argTh)) {
					dprintf(STDERR_FILENO, "joinRoomSocket take error, just send fail\n");
				}
				break;
			case out_mess_p:
				break;
			case out_logout_p:
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
	 * type= out_mkRoom_p
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

	int idKeyChat;
	char chatDir[64];
	char nameRoom[64];
	dprintf(fdDebug, "info->myPath = %s\n", info->myPath);
	sscanf(info->myPath, "./%[^/]/%ld:%s", chatDir, &idKeyChat, nameRoom);

	dprintf(fdDebug, "sscanf ha identificato:\n chatDir= %s\nidKeyChat= %d\nnameRoom =%s\n", chatDir, idKeyChat,
	        nameRoom);
	char nameChat[64];
	sprintf(nameChat, "%ld:%s", idKeyChat, nameRoom);
	strncpy(nameChatRet, nameChat, len);

	makeThRoom(idKeyChat, info->myPath, info);
	dprintf(fdDebug, "ROOM-th %d by idKeyUser=%d\n", idKeyChat, idKeyUser);

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

	strncpy(arg->roomName, roomPath, 50);
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
	 * type= out_joinRm_p
	 * sender=  userName(string) //non lo uso
	 * who= id_Chat_to_Join(string)
	 * mex= <Null>
	 */
	///ricerca room nell'avl
	mail respond;
	int idChatJoin = atoi(pack->md.whoOrWhy);
	int pipeRm = search_BFS_avl_S(rmAvlTree_Pipe, idChatJoin);
	if (pipeRm == -1) {
		dprintf(STDERR_FILENO, "Cercare la pipe ha creato un errore\nJoin Abortita");
		fillPack(&respond, out_failed_p, 0, 0, "Server", "Error in search room");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	} else if (pipeRm == -2) {
		dprintf(STDERR_FILENO, "Room id not found\nJoin Abortita");
		fillPack(&respond, out_failed_p, 0, 0, "Server", "Room id not found");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	}

	///invio alla room che mi deve aggiungere alla sua tabella

	mail roomPack;
	/*
	 * Join lato inside
	 * type= in_join_p
	 * sender=  userNameThread idkey:name (string)
	 * who= firstFree_EntryIndex(string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex= <myPipe> (int)
	 */
	char firstIndex[16];
	sprintf(firstIndex, "%d", data->info->tab->head.nf_id);
	dprintf(fdDebug, "Join th-User send in_join_p, at %d pipe\n", pipeRm);
	int *fdUsWritePipe = malloc(sizeof(int));
	*fdUsWritePipe = data->fdPipe[writePipe];
	fillPack(&roomPack, in_join_p, sizeof(int), fdUsWritePipe, data->userName, firstIndex);
	writePack_inside(pipeRm, &roomPack);



	///aspetto come risposta la posizione nella tabella della room del'user
	/*
	 * Join lato inside risposta
	 * type= in_entryIndex_p
	 * sender=  roomNameThread idkey:name (string)
	 * who= addPos(string)
	 * mex= <Null>
	 */
	///aggiungo alla mia tabella la nuova room
	RITENTA:
	readPack_inside(data->fdPipe[readPipe], &roomPack);
	switch (roomPack.md.type) {
		case in_entryIndex_p:

			break;
		case out_failed_p:
			dprintf(STDERR_FILENO, "Room take error\nJoin Abortita");
			fillPack(&respond, out_failed_p, 0, 0, "Server", "Room th take error");
			writePack(data->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto da chissà dove, ignoro e ritento
			goto RITENTA;
			break;
	}

	///la room mi ha inviato dove ha agiunto me stesso, ora la aggiungo alla mia tabella
	int addPos = addEntry(data->info->tab, roomPack.md.sender, atoi(roomPack.md.whoOrWhy));
	if (addPos == -1) {
		dprintf(STDERR_FILENO, "Add Entry for join take error\n");
		fillPack(&respond, out_failed_p, 0, 0, "ROOM", "addEntry fail");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	}
	///invio risposta positiva con i dati corretti
	entry enToSend;
	memcpy(&enToSend, &data->info->tab->data[addPos], sizeof(entry));

	fillPack(&respond, out_dataRm_p, sizeof(entry), &enToSend, "Server", 0);
	writePack(data->conUs.con.ds_sock, &respond);


	return 0;

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

	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2(info->fdPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit(-1);
	}
	insert_avl_node_S(rmAvlTree_Pipe, info->id, info->fdPipe[writePipe]);

	dprintf(fdOut, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n\tmi ragiungi da %d\n", info->id, info->roomName,
	        info->fdPipe[writePipe]);

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
	//todo: usando read_inside leggo i dati di comando  al th-room eseguo e rispondo
	mail packRecive;
	mail packSend;
	bool exit = true;
	char buffRet[128];
	while (exit) {
		dprintf(fdOut, "th-RoomRx %d in attesa di messaggio da [%d] pipe\n", info->id, info->fdPipe[readPipe]);

		if (readPack_inside(info->fdPipe[readPipe], &packRecive) == -1) {
			dprintf(STDERR_FILENO, "Read error, broken pipe\n");
			dprintf(STDERR_FILENO, "th-RoomRx %d in chiusura\n", info->id);
			sleep(1);
			free(info); //todo creare i free per ogni tipo di dato !!!!
			pthread_exit(-1); // todo gestione broken pipe e uscita thread
		}


		switch (packRecive.md.type) {
			case in_join_p:
				if (joinRoom_inside(&packRecive, info)) {
					dprintf(STDERR_FILENO, "Impossible Join for Th-room\n");
				}
				break;
			case in_mess_p:

				break;

			default:
				dprintf(fdDebug, "th-RoomRx %d ricevuto type=%d, NON GESTITO\n", info->id, packRecive.md.type);
				printPack(&packRecive);
				break;
		}

		if (!packRecive.mex) free(packRecive.mex);
	}
	delete_avl_node_S(rmAvlTree_Pipe, info->id);
	close(info->fdPipe[0]);
	close(info->fdPipe[1]);

	if (!packRecive.mex) free(packRecive.mex);
	if (!packSend.mex) free(packSend.mex);
	free(info); //todo creare i free per ogni tipo di dato !!!!
	pthread_exit(0);

	while (1) pause();


}

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX **/
int joinRoom_inside(mail *pack, thRoomArg *data) {
	/*
	 * con Data ho accesso alla tabella della room in questione
	 * e posso aggiungere alla tabella della room il nuovo user appena connesso
	 *
	 * Dal Th-user che capisco il nuovo utente da aggiungere alla tabella
	 *
	 *
	 * devo ritornare come risposta la posizione nella mia tabella, del'user appena collegato
	 *
	 */

	/*
	 * Join lato inside
	 * type= in_join_p
	 * sender=  userNameThread idkey:name (string)
	 * who= firstFree_EntryIndex(string)    //la posizione in cui viene aggiunto nella tabella del'user
	 * mex= <Null>
	 */



	mail respond;
	int pipeUser = *((int *) pack->mex);
	free(pack->mex);

	dprintf(fdDebug, "Join th-Room recive in_join_p, and pipe user is %d pipe\n", pipeUser);
	///aggiungo alla mia tabella la nuova room
	int addPos = addEntry(data->info->tab, pack->md.sender, atoi(pack->md.whoOrWhy));
	if (addPos == -1) {
		dprintf(STDERR_FILENO, "Add Entry for join side room take error\n");
		fillPack(&respond, out_failed_p, 0, 0, "ROOM", "addEntry fail");
		writePack_inside(pipeUser, &respond);
		return -1;
	}

	///invio risposta positiva con i dati corretti all'userTh
	/*
	 * Join lato inside risposta
	 * type= in_entryIndex_p
	 * sender=  roomNameThread idkey:name (string)
	 * who= addPos(string)
	 * mex= <Null>
	 */
	char addPosBuf[16];
	sprintf(addPosBuf, "%d", addPos);

	fillPack(&respond, in_entryIndex_p, 0, 0, data->roomName, addPosBuf);
	writePack_inside(pipeUser, &respond);


	return 0;

}

/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX(thRoomArg *info) {
	while (1) pause();


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
	int iterazione;
	do {
		dprintf(fdDebug, "reedPack_inside Funx [%d] e scrivo sulla pipe %d\n", iterazione, fdPipe);
		iterazione++;
		ret = read(fdPipe, pack + bRead, sizeof(mail) - bRead);
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
	int iterazione = 0;

	/*
	mail *test=malloc(sizeof(mail));
	fillPack(test,in_join_p,0,0,"test","test");
	*/
	//signal(SIGPIPE,SIG_IGN);
	do {
		dprintf(fdDebug, "writePack_inside Funx [%d] e scrivo sulla pipe %d\n", iterazione, fdPipe);
		iterazione++;
		printPack(pack);

		ret = write(fdPipe, pack + bWrite, sizeof(mail) - bWrite);
		if (ret == -1) {
			switch (errno) {
				case EPIPE:
					dprintf(STDERR_FILENO, "writePack_inside pipe break 1\n");
					return -1;
					//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
				default:
					perror("writePack_inside take error:\n");
					return -1;
					break;
			}
		}
		bWrite += ret;

	} while (sizeof(mail) - bWrite != 0);
	//signal(SIGPIPE,SIG_DFL);
	dprintf(fdDebug, "writePack_inside Funx send sulla pipe %d\n", iterazione, fdPipe);
	return 0;
}

int testConnection_inside(int fdPipe) {
	mail packTest;
	fillPack(&packTest, out_test_p, 0, NULL, "SERVER", "testing_code");

	if (writePack_inside(fdPipe, &packTest) == -1) {
		return -1;
	}
	return 0;
}