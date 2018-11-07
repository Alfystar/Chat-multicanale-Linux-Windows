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
				pthread_exit(NULL);
			}
			break;
		case out_mkUser_p:
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
			pthread_exit(NULL);
			break;
	}

	dprintf(fdDebug, "Login success\nTx-th & Rx-th of %d now Create\n", arg->id);
	sprintf(arg->idNameUs, "%d:%s", arg->id, arg->userName);

	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2(arg->fdPipe, O_DIRECT);
	if (errorRet != 0) {
		printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit(-1);
	}
	dprintf(fdDebug, "Th-user arg->fdPipe[0]=%d , arg->fdPipe[1]=%d\n", arg->fdPipe[0], arg->fdPipe[1]);

	insert_avl_node_S(usAvlTree_Pipe, arg->id, arg->fdPipe[writeEndPipe]);

	//i thUser condividono lo stesso oggetto thUserArg arg, quindi stare attenti !!!!
	pthread_create(&arg->tidRx, NULL, thUs_ServRX, arg);
	pthread_create(&arg->tidRx, NULL, thUs_ServTX, arg);
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
	 * Sender= usName
	 * whoOrWhy=idKey (string)
	 * dim=0
	 */

	mail response;
	//setUpThUser prendendo l'id dell'user mette i dati necessari in data
	if (setUpThUser(atoi(pack->md.whoOrWhy), data)) {
		perror("Impossible to create new ThUser of register User :");
		fillPack(&response, failed_p, 0, NULL, "Server", "setUpThUser error");
		writePack(data->conUs.con.ds_sock, &response);
		return -1;
	}

	if (strcmp(data->userName, pack->md.sender) != 0) {
		fillPack(&response, failed_p, 0, NULL, "Server", "USER NAME UN-CORRECT");
		writePack(data->conUs.con.ds_sock, &response);
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
				dprintf(STDERR_FILENO, "Deleting from userTab FAIL\n");
			}
		} else {
			dprintf(fdOut, "room %s trovata\n", cell[i].name);
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

	// dentro infoNewUs pathName è  => ./%s/%ld:%s
	char userDir[64];   // non mi serve, ma devo tenere i dati di appoggio
	sscanf(infoNewUs->pathName, "./%[^/]/%d:%s", userDir, &data->id, data->userName);

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
void *thUs_ServRX(thUserArg *argTh) {

	mail packRecive;
	mail packSend;
	bool exit = true;
	int retTh = 0;
	while (exit) {
		dprintf(fdOut, "thrServRx %d in attesa di messaggio da %d sock\n", argTh->id, argTh->conUs.con.ds_sock);

		if (readPack(argTh->conUs.con.ds_sock, &packRecive) == -1) {
			dprintf(STDERR_FILENO, "Read error, broken pipe\n");
			dprintf(STDERR_FILENO, "thrServRx and Tx %d in chiusura\n", argTh->id);
			retTh = -1;
			break;
		}
		/*
		dprintf(fdDebug, "thrServRx %d ricevuto packetto\n", argTh->id);
		printPack(&packRecive);
		*/
		switch (packRecive.md.type) {
			case out_mkRoom_p:
				if (mkRoomSocket(&packRecive, argTh)) {
					dprintf(STDERR_FILENO, "mkRoom coommand from socket fail\n");
					fillPack(&packSend, failed_p, 0, 0, "Server", "Impossible Create room");
					writePack(argTh->conUs.con.ds_sock, &packSend);
				}
				break;
			case out_joinRm_p:
				//mi unisco agli iscritti a una chat
				if (joinRoomSocket(&packRecive, argTh)) {
					dprintf(STDERR_FILENO, "joinRoomSocket take error, just send fail\n");
				}
				dprintf(fdDebug, "Join Room success\n");
				break;
			case out_delRm_p:
				//cancello tutta la room se amministratore
				if (delRoomSocket(&packRecive, argTh)) {
					dprintf(STDERR_FILENO, "delRoomSocket take error, just send fail\n");
				}
				dprintf(fdDebug, "Del Room success\n");
				break;
			case out_leaveRm_p:
				if (leaveRoomSocket(&packRecive, argTh)) {
					dprintf(STDERR_FILENO, "leaveRoomSocket take error, just send fail\n");
				}
				dprintf(fdDebug, "Leave Room success\n");
				break;
			case out_mess_p:
				break;
			case out_logout_p:
				exit = false;
				break;

			default:
				dprintf(fdDebug, "thrServRx %d ricevuto type=%d, NON GESTITO\n", packRecive.md.type);
				printPack(&packRecive);
				break;
		}

		if (!packRecive.mex) free(packRecive.mex);
	}

	delete_avl_node_S(usAvlTree_Pipe, argTh->id);   //mi levo dall'avl e inizio la pulizia
	sleep(1);
	pthread_cancel(argTh->tidTx);
	close(argTh->fdPipe[0]);
	close(argTh->fdPipe[1]);
	freeUserArg(argTh);
	if (!packRecive.mex) free(packRecive.mex);
	if (!packSend.mex) free(packSend.mex);
	freeUserArg(argTh);
	pthread_exit(retTh);
}

/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX **/

int mkRoomSocket(mail *pack, thUserArg *data) {
	//nameChatRet= puntatore a un buffer dove viene salvato il nome (id:name) della chat creata
	/*
	 * type= out_mkRoom_p
	 * sender= user (string) //non lo uso
	 * who=id (string)
	 * mex="<nameRoom>
	 */

	infoChat *info = newRoom(pack->mex, data->id);
	if (info == 0) {
		dprintf(STDERR_FILENO, "creazione della chat impossibile");
		//nameListFree(user);
		return -1;
	}
	//==============================================
	//OTTENGO I DATI DELLA ROOM
	int idKeyChat;
	char chatDir[64];
	char nameRoom[64];
	dprintf(fdDebug, "info->myPath = %s\n", info->myPath);
	sscanf(info->myPath, "./%[^/]/%d:%s", chatDir, &idKeyChat, nameRoom);

	dprintf(fdDebug, "sscanf ha identificato:\n\tchatDir= %s\n\tidKeyChat= %d\n\tnameRoom =%s\n", chatDir, idKeyChat,
	        nameRoom);
	char nameChat[64];
	sprintf(nameChat, "%d:%s", idKeyChat, nameRoom);
	//==============================================

	//segno che il primo elemento della tabella ROOM sono io, l'admin
	int addIndex = addEntry(data->info->tab, nameChat, 0);

	addEntry(info->tab, data->idNameUs, addIndex);      //aggiunge alla tabella della ROOM l'admin com primo elemento

	makeThRoom(idKeyChat, info->myPath, info);
	dprintf(fdDebug, "ROOM-th %d by idKeyUser=%d\n", idKeyChat, data->id);


	//ho successo, aggiungo la chat alla tabella dell'utente attuale
	//la prima entry delle tabelle è il proprietario (come first free)
	sem_wait(&screewWrite);
	wtabPrint(showPannel, data->info->tab, 0);
	sem_post(&screewWrite);

	/*  INVIO AL CLIENT
	 * type= success_p
	 * sender=  SERVER(string) //non lo uso
	 * who= keyChat:Name(string)
	 * mex= <Null>
	 */

	mail packSend;
	fillPack(&packSend, success_p, 0, 0, "Server", nameChat);
	writePack(data->conUs.con.ds_sock, &packSend);


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
		fillPack(&respond, failed_p, 0, 0, "Server", "Error in search room");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	} else if (pipeRm == -2) {
		dprintf(STDERR_FILENO, "Room id not found\nJoin Abortita");
		fillPack(&respond, failed_p, 0, 0, "Server", "Room id not found");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	}

	//check room just join
	if (searchFirstOccurenceKey(data->info->tab, idChatJoin) != -1) {
		//sono già joinato nella room
		dprintf(STDERR_FILENO, "Just Join inRoom %d\n", idChatJoin);
		fillPack(&respond, failed_p, 0, 0, "Server", "Room id just Join");
		writePack(data->conUs.con.ds_sock, &respond);
	}

	/*===========================================================================================*/
	/*============================== Cominicazione con Th-Room ==================================*/
	/*===========================================================================================*/
	///invio alla room che mi deve aggiungere alla sua tabella

	/*
	 * Join lato inside
	 * type= in_join_p
	 * sender=  userNameThread idkey:name (string)
	 * who= firstFree_EntryIndex(string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex= <myPipe> (int)
	 */
	mail roomPack;
	char firstIndex[16];
	sprintf(firstIndex, "%d", data->info->tab->head.nf_id);
	dprintf(fdDebug, "Join th-User send in_join_p, at %d pipe\n", pipeRm);
	int *fdUsWritePipe = malloc(sizeof(int));
	*fdUsWritePipe = data->fdPipe[writeEndPipe];
	fillPack(&roomPack, in_join_p, sizeof(int), fdUsWritePipe, data->idNameUs, firstIndex);
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
	readPack_inside(data->fdPipe[readEndPipe], &roomPack);
	switch (roomPack.md.type) {
		case in_entryIndex_p:

			break;
		case failed_p:
			dprintf(STDERR_FILENO, "Room take error\nJoin Abortita");
			fillPack(&respond, failed_p, 0, 0, "Server", "Room th take error");
			writePack(data->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto da chissà o in risposta a chissà dove, ignoro e rimetto in circolo ritento
			writePack_inside(data->fdPipe[writeEndPipe], &roomPack);
			goto RITENTA;
			break;
	}
	/*===========================================================================================*/


	///la room mi ha inviato dove ha agiunto me stesso, ora la aggiungo alla mia tabella
	//printPack(&roomPack);
	int addPos = addEntry(data->info->tab, roomPack.md.sender, atoi(roomPack.md.whoOrWhy));
	if (addPos == -1) {
		dprintf(STDERR_FILENO, "Add Entry for join take error\n");
		fillPack(&respond, failed_p, 0, 0, "ROOM", "addEntry fail");
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

int delRoomSocket(mail *pack, thUserArg *data) {
	/*RICEVO dal client
	 * type= out_delRm_p
	 * sender=  userName(string) ID:NAME //non lo uso
	 * who= idRoom:MyEntryIndexOfChat(string)
	 * mex= <Null>
	 */
	mail respond;
	int idRoom, indexEntryRoom;
	sscanf(pack->md.whoOrWhy, "%d:%d", &idRoom, &indexEntryRoom);

	/*  TRASMETTO AL TH-ROOM
	 * type= in_delRm_p
	 * sender=  userName(string) ID:NAME
	 * who= pipeRespond(string)
	 * mex= <Null>
	 */
	mail sendRoom;
	char myPipe[24];
	sprintf(myPipe, "%d", data->fdPipe[writeEndPipe]);
	fillPack(&sendRoom, in_delRm_p, 0, 0, data->idNameUs, myPipe);

	int pipeRm = search_BFS_avl_S(rmAvlTree_Pipe, idRoom);
	if (pipeRm == -1) {
		dprintf(STDERR_FILENO, "Cercare la pipe ha creato un errore\nDel Abortita");
		fillPack(&respond, failed_p, 0, 0, "Server", "Error in search room");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	} else if (pipeRm == -2) {
		//se arrivo qua allora SICURAMENTE è già scomparso dalla mia tabella, comunico al client di eliminarlo anche dalla sua
		dprintf(STDERR_FILENO, "Room id not found\nDel Abortita");
		fillPack(&respond, out_delRm_p, 0, 0, "Server", "Room id delete");
		writePack(data->conUs.con.ds_sock, &respond);
		return -1;
	}

	writePack_inside(pipeRm, &sendRoom);

	//attendo risposta dalla room
	///...
	///...
	mail reciveRoom;
	RITENTA:
	readPack_inside(data->fdPipe[readEndPipe], &reciveRoom);
	//se ricevo una succes, allora posso eliminare la room anche dalla mia tabella
	switch (reciveRoom.md.type) {
		case success_p:
			delEntry(data->info->tab, indexEntryRoom);
			dprintf(fdDebug, "destroy room succes\n");
			fillPack(&respond, success_p, 0, 0, "Server", "destroy room succes");
			writePack(data->conUs.con.ds_sock, &respond);
			break;
		case failed_p:
			dprintf(STDERR_FILENO, "destroy room fail\nDel Abortita");
			fillPack(&respond, failed_p, 0, 0, "Server", reciveRoom.md.whoOrWhy);
			writePack(data->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto da chissà o in risposta a chissà dove, ignoro e rimetto in circolo ritento
			writePack_inside(data->fdPipe[writeEndPipe], &reciveRoom);
			goto RITENTA;
			break;
	}
	return 0;

}

int leaveRoomSocket(mail *pack, thUserArg *data) {
	/*
	 * con Data ho accesso alla tabella dell'utente in questione
	 * e posso eliminare alla tabella dell'utente la room alla quale voglio leavare
	 *
	 * devo anche inviare al Th-room che deve rimuovermi, e invio la mia posizione nella sua tabella (il val di point)
	 * dal thRoom mi aspetto success/fail, verifica che il mio id corrisponda.
	 *
	 * devo ritornare al mio utente in success
	 *
	 */

	/*
	 * type= out_leaveRm_p
	 * sender=  userName(string) //non lo uso
	 * who= idKeyRoom:IndexOfMyTable(string)
	 * mex= <Null>
	 */
	mail respondUser;
	int idRoom, indexTabUs;
	sscanf(pack->md.whoOrWhy, "%d:%d", &idRoom, &indexTabUs);
	int pipeRm = search_BFS_avl_S(rmAvlTree_Pipe, idRoom);

	if (pipeRm == -2) {
		//se arrivo qua allora SICURAMENTE è già scomparso dalla mia tabella (altri metodi), comunico al client di eliminarlo anche dalla sua
		dprintf(STDERR_FILENO, "Room id not found\njoin Abortita");
		fillPack(&respondUser, out_delRm_p, 0, 0, "Server", "Room id delete");
		writePack(data->conUs.con.ds_sock, &respondUser);
		return -1;
	}

	///Invio al thRoom i dati necessari
	/*
	 * type= in_leave_p
	 * sender=  idNameUs(string)
	 * who= IndexOfTable:myPipe(string)
	 * mex= <Null>
	 */
	char buf[24];
	sprintf(buf, "%d:%d", data->info->tab->data[indexTabUs].point, data->fdPipe[writeEndPipe]);
	mail roomPack;
	fillPack(&roomPack, in_leave_p, 0, 0, data->idNameUs, buf);
	dprintf(fdDebug, "leaveRoomSocket write to rm\n");
	writePack_inside(pipeRm, &roomPack);

	///aspetto risposta dalla room

	READ_ROOM_LEAVE:
	readPack_inside(data->fdPipe[readEndPipe], &roomPack);
	switch (roomPack.md.type) {
		case success_p:
			delEntry(data->info->tab, indexTabUs);
			//warning del può fallire se ci sono problemi nel file sistem
			dprintf(fdDebug, "leaveRoomSocket send success at user\n");
			fillPack(&respondUser, success_p, 0, 0, data->idNameUs, "Leave compleate");
			writePack(data->conUs.con.ds_sock, &respondUser);
			return 0;
			break;
		case failed_p:
			dprintf(fdDebug, "leaveRoomSocket send fail at user\n");
			fillPack(&respondUser, failed_p, 0, 0, data->idNameUs, "Leave fail in room-Th");
			writePack(data->conUs.con.ds_sock, &respondUser);
			return -1;
			break;
		default:
			writePack_inside(data->fdPipe[writeEndPipe], &roomPack);
			goto READ_ROOM_LEAVE;
			break;

	}
	return 0;
}

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thUs_ServTX(thUserArg *argTh) {

	mail packRead_in;
	mail sendClient;
	//fillPack(&packWrite,);
	int exit = 1;
	while (exit) {
		//readPack() da aggiungere il selettore in ingresso di chat
		dprintf(fdDebug, "(TH-US-tx) reed pack\n");
		if (readPack_inside(argTh->fdPipe[readEndPipe], &packRead_in)) {
			switch (errno) {
				case EBADF:
					dprintf(STDERR_FILENO, "readEndPipe=%d now is close\nClose Th\n", argTh->fdPipe[readEndPipe]);
					pthread_exit(NULL);  //uccide il th (il pthread_close ha fallito)
					break;
				default:
					break;
			}
		}

		switch (packRead_in.md.type) {
			case in_delRm_p:
				/* RICEVUTO AL TH-User_tx
				 * type= in_delRm_p
				 * sender=  roomName(string) ID:NAME //non lo uso
				 * who= point of entry(string)
				 * mex= <Null>
				 */
				delEntry(argTh->info->tab, atoi(packRead_in.md.whoOrWhy));
				dprintf(fdDebug, "Th-usTx %s delete entry %d because closig room\n", argTh->idNameUs,
				        atoi(packRead_in.md.whoOrWhy));


				/*  INVIO AL CLIENT
				 * type= out_exitRm_p
				 * sender=  SERVER(string) //non lo uso
				 * who= point of entry(string)
				 * mex= <Null>
				 */
				fillPack(&sendClient, out_exitRm_p, 0, 0, "SERVER", packRead_in.md.whoOrWhy);
				writePack(argTh->conUs.con.ds_sock, &sendClient);
				break;

			default:
				//pacchetto non gestito, quindi non indirizzato a noi, lo ri infilo nella pipe
				writePack_inside(argTh->fdPipe[writeEndPipe], &packRead_in);
				usleep(5000);   //dorme 5ms per permettere al rx o chi per lui di prendere il pacchetto
				break;
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
	insert_avl_node_S(rmAvlTree_Pipe, info->id, info->fdPipe[writeEndPipe]);

	//char roomPath[50];  // Path completo room  ("./%s/%s", chatDirName, chats->names[i]); e chats->names = %d:%s
	// room path  ./%s/%d:%s  chatDirName id name
	char inizio[40];
	char chatDir[40];
	int id;
	char name[40];
	sscanf(info->roomPath, "%[^/]/%[^/]/%d:%s", inizio, chatDir, &id, name);
	strncpy(info->roomName, name, 50);
	sprintf(info->idNameRm, "%d:%s", id, name);


	dprintf(fdOut, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n\tmi ragiungi da %d\n", info->id, info->roomPath,
	        info->fdPipe[writeEndPipe]);

	//i thRoom condividono lo stesso oggetto thRoomArg info, quindi stare attenti
	pthread_create(&info->tidRx, NULL, thRoomRX, info);
	pthread_create(&info->tidTx, NULL, thRoomTX, info);

	pthread_exit(NULL); //ho generato le componenti posso terminare

	return NULL;

}

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX(thRoomArg *info) {
	mail packRecive;
	mail packSend;
	bool exit = true;
	int retTh = 0;
	while (exit) {
		dprintf(fdOut, "th-RoomRx %d in attesa di messaggio da [%d] pipe\n", info->id, info->fdPipe[readEndPipe]);
		dprintf(fdDebug, "(TH-rm-rx) reed pack\n");
		if (readPack_inside(info->fdPipe[readEndPipe], &packRecive) == -1) {
			dprintf(STDERR_FILENO, "Read error, broken pipe\n");
			dprintf(STDERR_FILENO, "th-RoomRx %d in chiusura\n", info->id);
			retTh = -1;
			break;
		}


		switch (packRecive.md.type) {
			case in_join_p:
				if (joinRoom_inside(&packRecive, info)) {
					dprintf(STDERR_FILENO, "Impossible Join for Th-room\n");
				}
				break;
			case in_delRm_p:
				if (delRoom_inside(&packRecive, info) == 0) {
					//devo chiudere tutti i th-collegari a questo
					pthread_cancel(info->tidTx);    //per ora state e type di default, meditare se modificare il type
					recursive_delete(info->roomPath);
					exit = false;
				} else {
					dprintf(STDERR_FILENO, "Impossible Destroy this Th-room\n");
				}
				break;
			case in_leave_p:
				if (leaveRoom_inside(&packRecive, info, &exit)) {
					dprintf(STDERR_FILENO, "Impossible leave user %s of this Th-room\n", packRecive.md.sender);
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
	delete_avl_node_S(usAvlTree_Pipe, info->id);   //mi levo dall'avl e inizio la pulizia
	sleep(1);
	pthread_cancel(info->tidTx);
	close(info->fdPipe[0]);
	close(info->fdPipe[1]);
	if (!packRecive.mex) free(packRecive.mex);
	if (!packSend.mex) free(packSend.mex);
	freeRoomArg(info);
	pthread_exit(retTh);
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
		fillPack(&respond, failed_p, 0, 0, "ROOM", "addEntry fail");
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

	fillPack(&respond, in_entryIndex_p, 0, 0, data->idNameRm, addPosBuf);
	writePack_inside(pipeUser, &respond);


	return 0;

}

int delRoom_inside(mail *pack, thRoomArg *data) {

	///eseguibile solo se sono amministratore della room

	/*  RICEVO DAL TH-User
	 * type= in_delRm_p
	 * sender=  userName(string) ID:NAME
	 * who= pipeRespond(string)
	 * mex= <Null>
	 */

	mail respondThUs;
	int pipeUserCallMe = atoi(pack->md.whoOrWhy);
	int chatAdmin = atoi(data->info->tab->head.name);
	//verifichiamo solo gli id per test amministratore
	if (chatAdmin != atoi(pack->md.sender)) {
		//non ero amministratore
		dprintf(STDERR_FILENO, "Utente %d non amministratore, è AMMINISTRATORE :%d\n", data->id, chatAdmin);
		fillPack(&respondThUs, failed_p, 0, 0, data->idNameRm, "Non hai il diritto");
		writePack_inside(pipeUserCallMe, &respondThUs);
		return -1;
	}

	delete_avl_node_S(rmAvlTree_Pipe, data->id); //mi tolgo dalle room ragiungibili
	sleep(1);   //aspetto per dare tempo di parlare a chi ancora avesse il mio recapito


	//Comincio a cancellare tutti gli utenti della chat, cancellando la room nella loro tabella.
	//se sono collegati gli invio un messaggio via pipe, altrimenti la cancello a mano
	mail respond;
	entry *en = data->info->tab->data;
	char userDir[64];
	table *usTabOffline;

	char pointStr[32];//string di en->point
	int idUserScr;
	int pipeUserScr;
	for (int i = 0; i < data->info->tab->head.len; ++i) {
		idUserScr = atoi(en[i].name);
		pipeUserScr = search_BFS_avl_S(usAvlTree_Pipe, idUserScr);
		if (pipeUserScr == pipeUserCallMe) //quando arrivo a me medesimo
		{
			///salta, quando rispondo success alla fine l'user chiamante la eliminerà
			continue;
		}
		switch (pipeUserScr) {
			case -1:    //errore
				dprintf(STDERR_FILENO, "Search avl create error, abort\n");
				fillPack(&respond, failed_p, 0, 0, "ROOM", "search user take error");
				writePack_inside(pipeUserCallMe, &respond);
				return -1;
				break;
			case -2:    //non trovato, utente offline
				///non faccio nulla, al suo successivo login lo elimina da solo
				/*
				//elimino la room dalla tabella di questo utente offline
				sprintf(userDir,"./%s/%s",userDirName,en->name);
				usTabOffline=open_Tab(userDir);
				delEntry(usTabOffline,en->point);
				closeTable(usTabOffline);
				*/

				break;
			default:    // >=0 trovato
				//invio un messaggio al tx di quel th-user per dirgli di eliminare la tabella
				//se il th-user è anche collegato ora alla room, è suo compito dimenticarsi di noi
				/*  INVIO AL TH-User_tx
				 * type= in_delRm_p
				 * sender=  roomName(string) ID:NAME //non lo uso
				 * who= point of entry(string)
				 * mex= <Null>
				 */
				sprintf(pointStr, "%d", en->point);
				fillPack(&respond, in_delRm_p, 0, 0, data->idNameRm, pointStr);
				writePack_inside(pipeUserScr, &respond);
				break;
		}

	}
	///ho eliminato da tutti i miei inscritti questa room dalla tabella
	//elimino la room sul fileSistem
	fillPack(&respond, success_p, 0, 0, data->idNameRm, "chiusura della room");
	writePack_inside(pipeUserCallMe, &respond);
	//todo quando ci sarà la lista dei connessi, cancellare uno ad uno i nodi freeando memoria

	//fuori chiudo l'altro thread room fuori
	return 0;

}

int leaveRoom_inside(mail *pack, thRoomArg *data, int *exit) {
	/*
	 * type= in_leave_p
	 * sender=  idNameUs(string)
	 * who= IndexOfTable:myPipe(string)
	 * mex= <Null>
	 */
	mail respond;
	int indexTabRoom, pipeUs;
	sscanf(pack->md.whoOrWhy, "%d:%d", &indexTabRoom, &pipeUs);
	dprintf(fdDebug, "leaveRoom_inside check id user\n");

	//check utente da leavare è quello che lo sta chiedendo
	if (atoi(pack->md.sender) == atoi(data->info->tab->data[indexTabRoom].name))
	{
		///l'eliminazione avviene dall'utente salvato in quella linea

		if (delEntry(data->info->tab, indexTabRoom)) {
			dprintf(STDERR_FILENO, "(rm-TH) Impossible remove entry\n");
			fillPack(&respond, failed_p, 0, 0, data->idNameRm, "Impossible remove");
			writePack_inside(pipeUs, &respond);
			return -1;
		}
		dprintf(fdDebug, "(rm-TH) leave user Do\n");
		fillPack(&respond, success_p, 0, 0, data->idNameRm, "Entry remove");
		writePack_inside(pipeUs, &respond);
		//todo quando ci sarà la lista dei connessi, cancellare uno ad uno i nodi freeando memoria

	} else {
		//utente errato
		dprintf(STDERR_FILENO, "(rm-TH) user not the same (id):\n%s != %s\n", pack->md.sender,
		        data->info->tab->data[indexTabRoom].name);
		fillPack(&respond, failed_p, 0, 0, data->idNameRm, "You aren't user");
		writePack_inside(pipeUs, &respond);
		return -1;
	}

	//check se l'uscito era l'amministratore, devo rinominare  un nuovo l'amministratore
	if(strcmp(data->info->tab->head.name,pack->md.sender)==0)
	{
		firstFree *fr=&data->info->tab->head;
		entry *en;
		//sono l'amministratore, devo rinominare un nuovo amministratore:
		for (int i = 0; i < fr->len; ++i)
		{
			en=&data->info->tab->data[i];
			if(!isEmptyEntry(en) && !isLastEntry(en)) //il primo non vuoto nuovo e non lastFree è il nuovo admin
			{
				if (renameFirstEntry(data->info->tab, en->name)) {
					dprintf(STDERR_FILENO, "Admin in room '%s' can't be Change to %s\n", data->idNameRm, fr->name);
					perror("cause of fail is:");
					return -1;
				}
				dprintf(fdOut, "Admin in room '%s' is now Change to %s\n", data->idNameRm, fr->name);
				return 0;
				break;
			}
		}
		///se arrivo qui significa che la tabella non ha più utenti, e va quindi eliminata
		dprintf(STDERR_FILENO,"New Admin not Found for room %s\nROOM DELETING",data->idNameRm);
		delete_avl_node_S(rmAvlTree_Pipe, data->id); //mi tolgo dalle room ragiungibili
		sleep(1);   //aspetto per dare tempo di parlare a chi ancora avesse il mio recapito
		pthread_cancel(data->tidTx);    //per ora state e type di defoult, meditare se modificare il type
		recursive_delete(data->roomPath);
		*exit = false;
		return 0;
	}
	return 0;
}


/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX(thRoomArg *info) {
	//todo se pthread_close fallisce tramite read pack si nota che il FD è chiuso, allora termino
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
	int iterazione = 0;
	do {
		dprintf(fdDebug, "reedPack_inside Funx [%d] e leggo dalla pipe %d\n", iterazione, fdPipe);
		iterazione++;
		ret = read(fdPipe, pack + bRead, sizeof(mail) - bRead);
		if (ret == -1) {
			perror("Read_in error; cause:");
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

void freeUserArg(thUserArg *p) {
	freeInfoUser(p->info);
	free(p);
}

void freeRoomArg(thRoomArg *p) {
	freeInfoChat(p->info);
	free(p);
}