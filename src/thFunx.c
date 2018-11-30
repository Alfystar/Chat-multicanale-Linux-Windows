//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"


/** TH-ACCEPT, GENERA TH USER IN FASE DI ACCEPT **/

void *acceptTh (thAcceptArg *info){

	thUserArg *arg;
	dprintf (fdOut, "acceptTh %d Creato\n", info->id);

	while (1){
		arg = malloc (sizeof (thUserArg));
		//definisco gli argomenti da passare al th user come puntati da arg di thConnArg.arg
		arg->id = -1;
		arg->conUs = info->conInfo; //copia negli argomenti del th una copia totale della connessione del server
		arg->info = NULL;
		arg->userName[0] = 0;
		arg->fdPipe[0] = -1;
		arg->fdPipe[1] = -1;
		arg->keyIdRmFF = -1;
		arg->pipeRmFF = -1;

		if (acceptCreate (&info->conInfo.con, userTh, arg) == -1){
			dprintf (STDERR_FILENO, "errore in accept\n");
		}
		dprintf (fdOut, "userTh Creato da accept,i suoi arg = %p sono stati creati.\n", arg);

	}
	free (info);
	return NULL;
}

/** TH-/** #### TH-USER GENERICO, prima di specializzarsi, HA IL COMPITO DI LOGIN **/
void *userTh (thConnArg *info){
	thUserArg *arg = info->arg; //da impostare in base al login, l'accept lascia campi "vuoti"
	arg->conUs.con = info->con; //copio i dati di info
	free (info); //il tipo thConnArg non serve più tutto è stato copiato

	dprintf (fdDebug, "TH-User creato, In Attesa primo pack\n");

	mail pack, responde;

	readPack (arg->conUs.con.ds_sock, &pack);        //ottengo il primo pack per capire il da fare.
	//in base al tipo di connessione si riempe arg con i giusti dati
	switch (pack.md.type){
		case out_login_p:
			if (loginServerSide (&pack, arg)){
				perror ("login fase fail :");
				dprintf (STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				pthread_exit (NULL);
			}
			break;
		case out_mkUser_p:
			if (mkUserServerSide (&pack, arg)){
				perror ("make New User fase fail :");
				dprintf (STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				fillPack (&responde, failed_p, 0, 0, "Server", "Can't create user");
				writePack (arg->conUs.con.ds_sock, &responde);
				pthread_exit (NULL);
			}
			break;
		default:
			dprintf (STDERR_FILENO, "Pack rivevuto inadatto a instaurazione com\n");
			fillPack (&pack, failed_p, 0, 0, "SERVER", "User Not yet Created");
			writePack (arg->conUs.con.ds_sock, &pack);
			pthread_exit (NULL);
			break;
	}

	dprintf (fdOut, "Login success\nTx-th & Rx-th of %d now Create\n", arg->id);
	sprintf (arg->idNameUs, "%d:%s", arg->id, arg->userName);

	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2 (arg->fdPipe, O_DIRECT);
	if (errorRet != 0){
		printErrno ("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		return NULL;
	}
	dprintf (fdDebug, "Th-user arg->fdPipe[0]=%d , arg->fdPipe[1]=%d\n", arg->fdPipe[0], arg->fdPipe[1]);

	insert_avl_node_S (usAvlTree_Pipe, arg->id, arg->fdPipe[writeEndPipe]);

	//i thUser condividono lo stesso oggetto thUserArg arg, quindi stare attenti !!!!
	pthread_create (&arg->tidRx, NULL, thUs_ServRX, arg);
	pthread_create (&arg->tidRx, NULL, thUs_ServTX, arg);
	//pthread_exit (NULL);

	mail packSocket;
	while (1)   //snodo di reindirizzamento dal soket per leggere tutto solo dalla pipe
	{
		dprintf (fdOut, "[Us-GEN-(%s)]wait message from [%d] sock\n", arg->idNameUs, arg->conUs.con.ds_sock);
		if (readPack (arg->conUs.con.ds_sock, &packSocket) == -1){
			dprintf (STDERR_FILENO, "Read error, broken pipe\n");
			dprintf (STDERR_FILENO, "thrServRx and Tx %s in chiusura\n", arg->idNameUs);
			break;
		}
		writePack_inside (arg->fdPipe[writeEndPipe], &packSocket);
	}
	delete_avl_node_S (usAvlTree_Pipe, arg->id);   //mi levo dall'avl e inizio la pulizia
	sleep (1);

	void *resRX, *resTX;

	do{ //devo ciclare perche' non e' detto che il CANCEL avvenga subito

		pthread_cancel (arg->tidRx);
		pthread_cancel (arg->tidTx);
		pthread_join (arg->tidRx, &resRX);
		pthread_join (arg->tidTx, &resTX);
	}
	while (resRX == PTHREAD_CANCELED || resTX == PTHREAD_CANCELED);

	close (arg->fdPipe[0]);
	close (arg->fdPipe[1]);
	freeUserArg (arg);
	freeMexPack (&arg);
	freeMexPack (&arg);
	pthread_exit (&arg);

	return NULL;
}

/** FUNZIONI PER IL TH-USER GENERICO, prima di specializzarsi **/
int loginServerSide (mail *pack, thUserArg *data){
	//imposto i thUserArg correttamente

	dprintf (fdDebug, "LOGIN FASE\nUtente = %s & iDKey=%d\n Type= %d\n", pack->md.sender, atoi (pack->md.whoOrWhy),
	         pack->md.type);
	/* Login:
	 * type=out_login_p
	 * Sender= usName
	 * whoOrWhy=idKey (string)
	 * dim=0
	 */

	mail response;
	//setUpThUser prendendo l'id dell'user mette i dati necessari in data
	if (setUpThUser (atoi (pack->md.whoOrWhy), data)){
		perror ("Impossible to create new ThUser of register User :");
		fillPack (&response, failed_p, 0, 0, "Server", "setUpThUser error");
		writePack (data->conUs.con.ds_sock, &response);
		return -1;
	}

	if (strcmp (data->userName, pack->md.sender) != 0){
		fillPack (&response, failed_p, 0, 0, "Server", "USER NAME UN-CORRECT");
		writePack (data->conUs.con.ds_sock, &response);
		return -1;
	}

	firstFree *head = &data->info->tab->head;
	entry *cell = data->info->tab->data;

	//for per eliminare room non più esistenti
	for (int i = 0; i < head->len; i++){
		if ((isEmptyEntry (&cell[i]) || isLastEntry (&cell[i])) == true){
			continue;
		}
		int keySearch = search_BFS_avl_S (rmAvlTree_Pipe, atoi (cell[i].name));
		if (keySearch == -2){
			//name:= keyRoom:NAME  se key non è trovata nell'avl allora la room non esiste più
			dprintf (fdDebug, "Room %s not found in Avl-Room. Deleting from userTab\n", cell[i].name);
			if (delEntry (data->info->tab, i)){
				dprintf (STDERR_FILENO, "Deleting from userTab FAIL\n");
			}
		}
		else if (keySearch == -1){
			fillPack (&response, failed_p, 0, 0, "Server", "ERROR IN SEARCH");
			writePack (data->conUs.con.ds_sock, &response);
			return -1;
		}
		else{
			dprintf (fdOut, "room %s trovata\n", cell[i].name);
		}

	}

	int len;
	void *mex = sendTab (data->info->tab, &len);

	// Invio risposta affermativa
	fillPack (&response, out_dataUs_p, len, mex, "Server", NULL);
	writePack (data->conUs.con.ds_sock, &response);
	//freeMexPack (&response);
	return 0;
}

int setUpThUser (int keyId, thUserArg *argUs){
	//modifica argUs per salvare dentro i file

	// @keyId:= id da cercare
	// @argUs:= puntatore alla struttura da impostare

	nameList *user = userExist ();
	int want = idSearch (user, keyId);
	if (want == -1){
		dprintf (STDERR_FILENO, "Id richiesto inesistente\n");
		errno = ENOENT;
		return -1;
	}

	//Inizio la creazione del thread
	char userDir[128];
	sprintf (userDir, "./%s/%s", userDirName, user->names[want]);
	infoUser *info = openUser (userDir);
	if (info == NULL){
		dprintf (STDERR_FILENO, "creazione dell'User impossibile\n");
		errno = ENOENT;
		return -1;
	}
	sem_wait (&screewWrite);
	wtabPrint (showPannel, info->tab, 0);
	sem_post (&screewWrite);

	argUs->id = keyId;
	argUs->info = info;

	int id;
	char name[stringLen];
	sscanf (user->names[want], "%d:%s", &id, name);
	strncpy (argUs->userName, name, stringLen);   //serve a prendere solo il nome dell'utente
	nameListFree (user);

	return 0;
}

int mkUserServerSide (mail *pack, thUserArg *data){
	//imposto i thUserArg correttamente

	dprintf (fdDebug, "LOGIN FASE, create user\n");
	/* mkUser:
	 * type=out_mkUser_p
	 * Sender=Name new (string)
	 * whoOrWhy= null
	 * dim=0
	 */

	mail response;
	infoUser *infoNewUs = newUser (pack->md.sender);
	if (infoNewUs == NULL){
		dprintf (STDERR_FILENO, "creazione della chat impossibile");
		return -1;
	}
	data->info = infoNewUs;

	// dentro infoNewUs pathName è  => ./%s/%ld:%s
	char userDir[stringLen];   // non mi serve, ma devo tenere i dati di appoggio
	sscanf (infoNewUs->pathName, "./%[^/]/%d:%s", userDir, &data->id, data->userName);

	dprintf (fdDebug, "infoNewUs->pathName = %s\n", infoNewUs->pathName);
	dprintf (fdDebug, "sscanf ha identificato:\n userDir= %s\ndata->id= %d\ndata->userName =%s\n", userDir, data->id,
	         data->userName);

	int len;
	char *mex = sendTab (infoNewUs->tab, &len);

	/// Invio dataSend
	char idSend[16];
	sprintf (idSend, "%d", data->id);
	fillPack (&response, out_dataUs_p, len, mex, "Server", idSend);
	writePack (data->conUs.con.ds_sock, &response);
	//freeMexPack (&response);
	return 0;
}

void *sendTab (table *t, int *len){
	firstFree *head = &t->head;
	entry *cell = t->data;
	int sizeTab = (head->len) * sizeof (entry) + sizeof (firstFree);
	void *mex = malloc (sizeTab);

	memcpy (mex, head, sizeof (firstFree));
	memcpy (mex + sizeof (firstFree), cell, sizeTab - sizeof (firstFree));
	*len = sizeTab;
	return mex;
}

/** #### TH-USER SUL SERVER CON RUOLO DI RX **/
void *thUs_ServRX (thUserArg *uData){

	mail packClient, packRoom;
	bool exit = true;
	int retTh = 0;
	while (exit){
		dprintf (fdOut, "[Us-rx-(%s)]wait message from [%d] pipe\n", uData->idNameUs, uData->fdPipe[readEndPipe]);
		if (readPack_inside (uData->fdPipe[readEndPipe], &packClient) == -1){
			switch (errno){
				case EBADF:
					dprintf (STDERR_FILENO, "readEndPipe=%d now is close\nClose Th\n", uData->fdPipe[readEndPipe]);
					pthread_exit (NULL);  //uccide il th (il pthread_close ha fallito)
					break;
				default:
					break;
			}
		}

		switch (packClient.md.type){
			case out_mkRoom_p:
				if (mkRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]mkRoomSocket take error, just send fail\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]Make New Room success\n", uData->idNameUs);
				}
				break;
			case out_joinRm_p:
				if (joinRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]joinRoomSocket take error, just send fail\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]Join Room success\n", uData->idNameUs);
				}
				break;
			case out_delRm_p:
				if (delRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]delRoomSocket take error, just send fail\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]Del Room success\n", uData->idNameUs);
				}
				break;
			case out_leaveRm_p:
				if (leaveRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]leaveRoomSocket take error, just send fail\n",
					         uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]Leave Room success\n", uData->idNameUs);
				}
				break;
			case out_openRm_p:
				if (openRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]openRoomSocket take error, just send fail\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]OpenRm success\n", uData->idNameUs);
				}
				break;
			case out_mess_p:
				dprintf (fdOut, "[Us-rx-(%s)]MEX Incoming:\n", uData->idNameUs);
				//dprintf (fdOut, "[Us-rx-(%s)]MEX Incoming:\n%s\n", uData->idNameUs, packClient.mex);
				//printPack (&packClient,fdOut);
				//sleep (1);
				if (mexReciveSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]mexReciveSocket take error, just send fail\n",
					         uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]Mex Forwarding success\n", uData->idNameUs);
				}
				break;
			case out_exitRm_p:
				if (exitRoomSocket (&packClient, uData)){
					dprintf (STDERR_FILENO, "[Us-rx-(%s)]exitRmSocket take error, NOT SEND FAIL\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-rx-(%s)]exitRmSocket success\n", uData->idNameUs);
				}
				break;
			case out_logout_p:
				exit = false;
				break;

			default:
				dprintf (fdDebug, "[Us-rx-(%s)]Unexpected pack type = %d[%s]\n", uData->idNameUs, packClient.md.type,
				         typeToText (packClient.md.type));
				printPack (&packClient, fdDebug);
				dprintf (fdDebug, "\n");
				writePack_inside (uData->fdPipe[writeEndPipe], &packClient);
				break;
		}
		//freeMexPack (&packClient);
		//freeMexPack (&packRoom);
	}//#end while

	delete_avl_node_S (usAvlTree_Pipe, uData->id);   //mi levo dall'avl e inizio la pulizia
	sleep (1);

	void *resRX, *resTX;

	do{ //devo ciclare perche' non e' detto che il CANCEL avvenga subito

		pthread_cancel (uData->tidTx);
		pthread_join (uData->tidTx, &resTX);
	}
	while (resRX == PTHREAD_CANCELED || resTX == PTHREAD_CANCELED);

	close (uData->fdPipe[0]);
	close (uData->fdPipe[1]);
	freeUserArg (uData);
	freeMexPack (&packClient);
	freeMexPack (&packRoom);
	pthread_exit (&retTh);
}

/** FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI RX **/

int mkRoomSocket (mail *pack, thUserArg *uData){
	/* MAKE ROOM
	 * type = out_mkRoom_p
	 * sender = id:name (string) //non lo uso
	 * who = id (string)
	 * mex="<nameRoom>
	 */
	//todo chiedere a filippo cosa invia in who
	mail packSend;
	infoChat *info = newRoom (pack->mex, uData->id);
	if (info == 0){
		dprintf (STDERR_FILENO, "creazione della chat impossibile");
		fillPack (&packSend, failed_p, 0, 0, "Server", "Impossible Create room");
		writePack (uData->conUs.con.ds_sock, &packSend);
		return -1;
	}
	//==================================================================
	//====================== OTTENGO I DATI DELLA ROOM==================
	//==================================================================
	int idKeyChat;
	char chatDir[stringLen], nameRoom[stringLen];
	dprintf (fdDebug, "info->myPath = %s\n", info->myPath);
	sscanf (info->myPath, "./%[^/]/%d:%s", chatDir, &idKeyChat, nameRoom);

	dprintf (fdDebug, "sscanf ha identificato:\n\tchatDir= %s\n\tidKeyChat= %d\n\tnameRoom =%s\n", chatDir, idKeyChat,
	         nameRoom);
	char nameChat[64];
	sprintf (nameChat, "%d:%s", idKeyChat, nameRoom);
	//==================================================================

	//Aggiungo room alla mia tabella
	int addIndex = addEntry (uData->info->tab, nameChat, 0); //sono il primo elemento della room
	addEntry (info->tab, uData->idNameUs, addIndex);         //dico alla room dove è nella mia tabella

	//genero il th-Rm generico
	makeThRoom (idKeyChat, info->myPath, info);
	dprintf (fdDebug, "ROOM-th %d by idKeyUser=%d\n", idKeyChat, uData->id);

	//visualizzo la tabella dell'user
	sem_wait (&screewWrite);
	wtabPrint (showPannel, uData->info->tab, 0);
	sem_post (&screewWrite);

	fillPack (&packSend, success_p, 0, 0, "Server", nameChat);
	/* INVIO AL CLIENT i dati di Make room
	 * type= success_p
	 * sender=  SERVER(string) //non lo uso
	 * who= keyChat:Name(string)
	 * mex= <Null>
	 */
	writePack (uData->conUs.con.ds_sock, &packSend);
	return 0;
}

void makeThRoom (int keyChat, char *roomPath, infoChat *info){
	if (info == NULL){
		dprintf (STDERR_FILENO, "infoChat NULL, impossibile creare Tr-ROOM\n");
		return;
	}
	pthread_t roomtid;
	thRoomArg *arg = malloc (sizeof (thRoomArg));
	arg->id = keyChat;

	strncpy (arg->roomPath, roomPath, stringLen);
	arg->info = info;

	int errorRet;
	errorRet = pthread_create (&roomtid, NULL, roomTh, arg);
	if (errorRet != 0){
		printErrno ("La creazione del Thread ROOM ha dato il seguente errore", errorRet);
		exit (-1);
	}
}

int joinRoomSocket (mail *pack, thUserArg *uData){
	/* RICHIESTA JOIN A UNA ROOM DA CLIENT
	 * type = out_joinRm_p
	 * sender =  userName(string) //non lo uso
	 * who = id_Chat_to_Join(string)
	 * mex = <Null>
	 */

	mail respond;

	//ricerca room nell'avl
	int idChatJoin = atoi (pack->md.whoOrWhy);
	int pipeRm = search_BFS_avl_S (rmAvlTree_Pipe, idChatJoin);
	if (pipeRm == -1){
		dprintf (STDERR_FILENO, "Cercare la pipe ha creato un errore\nJoin Abortita");
		fillPack (&respond, failed_p, 0, 0, "Server", "Error in search room");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}
	else if (pipeRm == -2){
		dprintf (STDERR_FILENO, "Room id not found\nJoin Abortita");
		fillPack (&respond, failed_p, 0, 0, "Server", "Room id not found");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}

	//check room just join
	if (searchFirstOccurenceKey (uData->info->tab, idChatJoin) != -1){
		//sono già joinato nella room
		dprintf (STDERR_FILENO, "Just Join inRoom %d\n", idChatJoin);
		fillPack (&respond, failed_p, 0, 0, "Server", "Room id just Join");
		writePack (uData->conUs.con.ds_sock, &respond);
	}

	/*===========================================================================================*/
	/*============================== Comunicazione con Th-Room ==================================*/
	/*===========================================================================================*/

	mail roomPack;
	char firstIndex[16];
	sprintf (firstIndex, "%d", uData->info->tab->head.nf_id);
	dprintf (fdDebug, "Join th-User send in_join_p, at %d pipe\n", pipeRm);
	int *fdUsWritePipe = malloc (sizeof (int));       //i messaggi devono essere pacchetti malloccati
	*fdUsWritePipe = uData->fdPipe[writeEndPipe];

	fillPack (&roomPack, in_join_p, sizeof (int), fdUsWritePipe, uData->idNameUs, firstIndex);
	/* RICHIESTA JOIN A ROOM DA TH-USER
	 * type = in_join_p
	 * sender =  idkeyUs:name (string)
	 * who = firstFree_EntryIndex(string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex = <myPipe> (int)
	 */
	writePack_inside (pipeRm, &roomPack);
	//freeMexPack (&roomPack);
	/*====================================== Attendo risposta dalla Room ======================================*/

RITENTA:
	readPack_inside (uData->fdPipe[readEndPipe], &roomPack);
	/* RISPOSTA JOIN DA TH-ROOM (PARTICOLARE)
	 * type = in_entryIndex_p
	 * sender = idkeyRoom:name (string)
	 * who = addPos(string)
	 * mex = <Null>
	 */
	switch (roomPack.md.type){
		case in_entryIndex_p:
			//aggiungo alla mia tabella la nuova room sotto
			break;
		case failed_p:
			dprintf (STDERR_FILENO, "Room take error\nJoin Abortita");
			fillPack (&respond, failed_p, 0, 0, "Server", "Room th take error");
			writePack (uData->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto indesiderato
			writePack_inside (uData->fdPipe[writeEndPipe], &roomPack);
			goto RITENTA;
			break;
	}

	/*===========================================================================================*/

	//aggiungo la room alla mia tabella
	int addPos = addEntry (uData->info->tab, roomPack.md.sender, atoi (roomPack.md.whoOrWhy));
	if (addPos == -1){
		dprintf (STDERR_FILENO, "Add Entry for join take error\n");
		fillPack (&respond, failed_p, 0, 0, "ROOM", "addEntry fail");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}
	entry enToSend;
	memcpy (&enToSend, &uData->info->tab->data[addPos], sizeof (entry));
	fillPack (&respond, out_dataRm_p, sizeof (entry), &enToSend, "Server", 0);
	writePack (uData->conUs.con.ds_sock, &respond);
	//freeMexPack (&respond);
	return 0;
}

int delRoomSocket (mail *pack, thUserArg *uData){
	/* DELETE ROOM DAL CLIENT
	 * type = out_delRm_p
	 * sender =  userName(string) ID:NAME //non lo uso
	 * who = idRoom:MyEntryIndexOfChat(string)
	 * mex = <Null>
	 */
	mail respond, sendRoom;
	int idRoom, indexEntryRoom;
	char myPipe[24];
	sscanf (pack->md.whoOrWhy, "%d:%d", &idRoom, &indexEntryRoom);
	sprintf (myPipe, "%d", uData->fdPipe[writeEndPipe]);

	fillPack (&sendRoom, in_delRm_p, 0, 0, uData->idNameUs, myPipe);
	/* RICHIESTA DELETE-ROOM DA TH-USER
	 * type = in_delRm_p
	 * sender =  userName(string) ID:NAME
	 * who = pipeRespond(string)
	 * mex = <Null>
	 */
	int pipeRm = search_BFS_avl_S (rmAvlTree_Pipe, idRoom);
	if (pipeRm == -1){
		dprintf (STDERR_FILENO, "Cercare la pipe ha creato un errore\nDel Abortita");
		fillPack (&respond, failed_p, 0, 0, "Server", "Error in search room");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}
	else if (pipeRm == -2){
		//se arrivo qua allora SICURAMENTE è già scomparso dalla mia tabella, comunico al client di eliminarlo anche dalla sua
		dprintf (STDERR_FILENO, "Room id not found\nDel Abortita");
		fillPack (&respond, out_delRm_p, 0, 0, "Server", "Room id delete");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}
	writePack_inside (pipeRm, &sendRoom);

	/*====================================== Attendo risposta dalla Room ======================================*/

	mail reciveRoom;
RITENTA:
	readPack_inside (uData->fdPipe[readEndPipe], &reciveRoom);
	switch (reciveRoom.md.type){
		case success_p:
			//posso eliminare la room anche dalla mia tabella
			delEntry (uData->info->tab, indexEntryRoom);
			dprintf (fdDebug, "destroy room succes\n");
			fillPack (&respond, success_p, 0, 0, "Server", "destroy room succes");
			writePack (uData->conUs.con.ds_sock, &respond);
			break;
		case failed_p:
			dprintf (STDERR_FILENO, "destroy room fail\nDel Abortita");
			fillPack (&respond, failed_p, 0, 0, "Server", reciveRoom.md.whoOrWhy);
			writePack (uData->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto indesiderato
			writePack_inside (uData->fdPipe[writeEndPipe], &reciveRoom);
			goto RITENTA;
			break;
	}
	return 0;
}

int leaveRoomSocket (mail *pack, thUserArg *uData){
	/* RICHIESTA DI LEAVE DA CLIENT
	 * type = out_leaveRm_p
	 * sender =  userName(string) //non lo uso
	 * who = idKeyRoom:IndexOfMyTable(string)
	 * mex = <Null>
	 */
	mail respondUser;
	int idRoom, indexTabUs;
	sscanf (pack->md.whoOrWhy, "%d:%d", &idRoom, &indexTabUs);
	int pipeRm = search_BFS_avl_S (rmAvlTree_Pipe, idRoom);
	if (pipeRm == -1){
		dprintf (STDERR_FILENO, "Cercare la pipe ha creato un errore\nDel Abortita");
		fillPack (&respondUser, failed_p, 0, 0, "Server", "Error in search room");
		writePack (uData->conUs.con.ds_sock, &respondUser);
		return -1;
	}
	else if (pipeRm == -2){
		//se arrivo qua allora SICURAMENTE è già scomparso dalla mia tabella, comunico al client di eliminarlo anche dalla sua
		dprintf (STDERR_FILENO, "Room id not found\nDel Abortita");
		fillPack (&respondUser, out_delRm_p, 0, 0, "Server", "Room id delete");
		writePack (uData->conUs.con.ds_sock, &respondUser);
		return -1;
	}

	//Invio al thRoom i dati necessari

	char buf[24];
	sprintf (buf, "%d:%d", uData->info->tab->data[indexTabUs].point, uData->fdPipe[writeEndPipe]);
	mail roomPack;
	fillPack (&roomPack, in_leave_p, 0, 0, uData->idNameUs, buf);
	/* RICHIESTA LEAVE-ROOM ALLA ROMM
	 * type = in_leave_p
	 * sender =  idNameUs(string)
	 * who = IndexOfTable:myPipe(string)
	 * mex = <Null>
	 */
	writePack_inside (pipeRm, &roomPack);

	/*====================================== Attendo risposta dalla Room ======================================*/

READ_ROOM_LEAVE:
	readPack_inside (uData->fdPipe[readEndPipe], &roomPack);
	switch (roomPack.md.type){
		case success_p:
			delEntry (uData->info->tab, indexTabUs);
			//warning del può fallire se ci sono problemi nel file sistem
			dprintf (fdDebug, "leaveRoomSocket send success at user\n");
			fillPack (&respondUser, success_p, 0, 0, uData->idNameUs, "Leave compleate");
			writePack (uData->conUs.con.ds_sock, &respondUser);
			return 0;
			break;
		case failed_p:
			dprintf (fdDebug, "leaveRoomSocket send fail at user\n");
			fillPack (&respondUser, failed_p, 0, 0, uData->idNameUs, "Leave fail in room-Th");
			writePack (uData->conUs.con.ds_sock, &respondUser);
			return -1;
			break;
		default:
			//pacchetto indesiderato
			writePack_inside (uData->fdPipe[writeEndPipe], &roomPack);
			goto READ_ROOM_LEAVE;
			break;
	}
	return 0;
}

int openRoomSocket (mail *pack, thUserArg *uData){
	/* RICHIESTA OPEN A UNA ROOM DA CLIENT
	 * type = out_openRm_p
	 * sender =  id:name
	 * who = idKeyRoom:IndexOfMyTable(string)
	 * mex = <Null>
	 */
	mail respond;
	int idKeyRm, indexTab;
	sscanf (pack->md.whoOrWhy, "%d:%d", &idKeyRm, &indexTab);

	//test indexTab sia nella mia room
	if (atoi (uData->info->tab->data[indexTab].name) != idKeyRm){
		fillPack (&respond, failed_p, 0, 0, "Server", "Not in my table");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}

	//test presenza in un altra mail-list
	if (uData->keyIdRmFF != -1){
		exitRoomSocket (pack, uData);
	}

	int pipeRm = search_BFS_avl_S (rmAvlTree_Pipe, idKeyRm);
	if (pipeRm == -1){
		dprintf (STDERR_FILENO, "Cercare la pipe ha creato un errore\n");
		fillPack (&respond, failed_p, 0, 0, "Server", "Error in search room");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}
	else if (pipeRm == -2){
		//se arrivo qua allora SICURAMENTE è già scomparso dalla mia tabella, comunico al client di eliminarlo anche dalla sua
		dprintf (STDERR_FILENO, "Room id not found\nOpen Abortita");
		fillPack (&respond, out_delRm_p, 0, 0, "Server", "Room id deleted");
		writePack (uData->conUs.con.ds_sock, &respond);
		return -1;
	}

	mail roomPack;
	char buff[16];
	sprintf (buff, "%d", uData->fdPipe[writeEndPipe]);
	fillPack (&roomPack, in_openRm_p, 0, 0, uData->idNameUs, buff);
	/* RICHIESTA OPEN A ROOM DA TH-USER
	 * type = in_openRm_p
	 * sender =  idkeyUs:name (string)
	 * who = my write End pipe (string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex = <Nul>
	 */
	writePack_inside (pipeRm, &roomPack);

	/*====================================== Attendo risposta dalla Room ======================================*/

	convRam *cpRam;
	size_t pt;
	char *buf;
READ_ROOM_OPEN:
	readPack_inside (uData->fdPipe[readEndPipe], &roomPack);
	dprintf (fdDebug, "[openRoomSocket]Recive from TH-ROOM type: %d[%s]\n", roomPack.md.type,
	         typeToText (roomPack.md.type));
	/* RISPOSTA OPEN DA TH-ROOM (PARTICOLARE)
	 * type = in_kConv_p
	 * sender = idkeyRoom:name (string)
	 * who = idkeyRoom:name (string)
	 * dim = dimensione del file srotolato (sul file sistem)
	 * mex = <conversazione copiata fino a quel momento> (convRam *)
	 */
	switch (roomPack.md.type){
		case in_kConv_p:
			cpRam = roomPack.mex;
			buf = malloc (roomPack.md.dim);
			memcpy (buf, cpRam, sizeof (convInfo));
			pt = sizeof (convInfo);
			for (int i = 0; i < cpRam->head.nMex; ++i){
				memcpy (buf + pt, &cpRam->mexList[i]->info, sizeof (mexInfo));
				pt += sizeof (mexInfo);
				strcpy (buf + pt, cpRam->mexList[i]->text);
				pt += strlen (cpRam->mexList[i]->text);
			}

			fillPack (&respond, out_kConv_p, roomPack.md.dim, buf, "Server", roomPack.md.whoOrWhy);
			writePack (uData->conUs.con.ds_sock, &respond);
			//dprintf (fdDebug, "[openRoomSocket]Write k_conv doing\n");
			uData->pipeRmFF = pipeRm;
			uData->keyIdRmFF = idKeyRm;
			free (buf);
			buf = NULL;
			return 0;
			break;
		case failed_p:
			fillPack (&respond, failed_p, 0, 0, "Server", roomPack.md.whoOrWhy);
			writePack (uData->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto indesiderato
			writePack_inside (uData->fdPipe[writeEndPipe], &roomPack);
			goto READ_ROOM_OPEN;
			break;
	}
	return -1;
}

int exitRoomSocket (mail *pack, thUserArg *data){
	/* RICHIESTA EXIT DALLA ROOM DA CLIENT
	 * type = out_exitRm_p
	 * sender =  id:name
	 * who = id_Chat_to_Exit(string)
	 * mex = <Null>
	 */
	mail packRoom;
	if (atoi (pack->md.whoOrWhy) == data->keyIdRmFF){
		fillPack (&packRoom, in_exit_p, 0, 0, data->idNameUs, pack->md.whoOrWhy);
		/* RICHIESTA EXIT A ROOM DA TH-USER
		 * type = in_exit_p
		 * sender =  idkeyUs:name (string)
		 * who = id_Chat_to_Exit(string)
		 * mex = <Null>
		 */
		writePack_inside (data->pipeRmFF, &packRoom);
		//a questo punto mi dimentico della room
		dprintf (fdOut, "Us %d exit from room %d\n", data->id, data->keyIdRmFF);
		data->pipeRmFF = -1;
		data->keyIdRmFF = -1;
		return 0;
	}
	else{
		dprintf (STDERR_FILENO, "Us %d exit from room %d\n!! Impossible notify at room\n", data->id, data->keyIdRmFF);
		data->pipeRmFF = -1;
		data->keyIdRmFF = -1;
		return -1;
	}
}

int mexReciveSocket (mail *pack, thUserArg *data){
	/* MESSAGGIO IN ARRIVO DA CLIENT
	 * type = out_mess_p
	 * sender = usId:name
	 * who = idNumber_ di controllo, da mantenere intatto
	 * mex = testoMessaggio
	 * dim = dime messaggio (\0 comreso)
	 */
	mail roomPack, respond;
	char buf[sendDim];
	sprintf (buf, "%d:%s", data->fdPipe[writeEndPipe], data->idNameUs);
	fillPack (&roomPack, in_mess_p, pack->md.dim, pack->mex, buf, pack->md.whoOrWhy);
	/* INVIO MESSAGGIO DA TH-USER
	 * type = in_mess_p
	 * sender = usPipe:IDKey:NAME [%d:%d:%s]
	 * who = idNumber_ di controllo, da mantenere intatto
	 * mex = testoMessaggio
	 * dim = dim messaggio (\0 comreso)
	 */
	writePack_inside (data->pipeRmFF, &roomPack);
	/*====================================== Attendo risposta dalla Room ======================================*/
READ_MEX_RECIVE:
	readPack_inside (data->fdPipe[readEndPipe], &roomPack);
	switch (roomPack.md.type){
		case out_messSuccess_p:
			fillPack (&respond, out_messSuccess_p, 0, 0, roomPack.md.sender, roomPack.md.whoOrWhy);
			writePack (data->conUs.con.ds_sock, &respond);
			return 0;
			break;
		case failed_p:
			dprintf (STDERR_FILENO, "[mexReciveSocket_respond room] fail from room, cause %s\n", roomPack.md.whoOrWhy);
			fillPack (&respond, failed_p, 0, 0, roomPack.md.sender, roomPack.md.whoOrWhy);
			writePack (data->conUs.con.ds_sock, &respond);
			return -1;
			break;
		default:
			//pacchetto indesiderato
			writePack_inside (data->fdPipe[writeEndPipe], &roomPack);
			goto READ_MEX_RECIVE;
			break;
	}
}

/** #### TH-USER SUL SERVER CON RUOLO DI TX **/
void *thUs_ServTX (thUserArg *uData){
	mail packPipeRead;
	while (1){
		dprintf (fdOut, "[Us-tx-(%s)]wait message from [%d] pipe\n", uData->idNameUs, uData->fdPipe[readEndPipe]);
		if (readPack_inside (uData->fdPipe[readEndPipe], &packPipeRead)){
			switch (errno){
				case EBADF:
					dprintf (STDERR_FILENO, "readEndPipe=%d now is close\nClose Th\n", uData->fdPipe[readEndPipe]);
					pthread_exit (NULL);  //uccide il th (il pthread_close ha fallito)
					break;
				default:
					break;
			}
		}

		switch (packPipeRead.md.type){
			case in_delRm_p:
				if (delRoomForwarding (&packPipeRead, uData))
				{
					dprintf (STDERR_FILENO, "[Us-tx-(%s)]Forwarding delRoom notify take error\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-tx-(%s)]Forwarding delRoom notify Success\n", uData->idNameUs);
				}
				break;
			case in_mess_p:
				if (mexForwarding (&packPipeRead, uData)){
					dprintf (STDERR_FILENO, "[Us-tx-(%s)]Forwarding message take error\n", uData->idNameUs);
				}
				else{
					dprintf (fdOut, "[Us-tx-(%s)]Forwarding message Success\n", uData->idNameUs);
				}
				break;
			default:
				//pacchetto indesiderato
				dprintf (fdDebug, "[Us-tx-(%s)]Unexpected pack type = %d[%s]\n", uData->idNameUs, packPipeRead.md.type,
				         typeToText (packPipeRead.md.type));
				writePack_inside (uData->fdPipe[writeEndPipe], &packPipeRead);
				break;
		}
		//freeMexPack (&packPipeRead);
		//freeMexPack (&sendClient);
	}//#end while
}

/* FUNZIONI DI SUPPORTO PER TH-USER SUL SERVER CON RUOLO DI TX */

int delRoomForwarding (mail *pack, thUserArg *data){
	/* RICEVUTO AL TH-ROOM
	 * type = in_delRm_p
	 * sender =  roomName(string) ID:NAME //non lo uso
	 * who = index of entry in Tab(string)
	 * mex = <Null>
	 */
	int entryToDel = atoi (pack->md.whoOrWhy);
	delEntry (data->info->tab, entryToDel);
	dprintf (fdDebug, "[Us-tx-(%s)]delete entry %d caused closing room\n", data->idNameUs, entryToDel);
	//todo possibile problema di sincronia con rx !!!! da meditare
	if (data->keyIdRmFF == atoi (pack->md.sender)) //se ero nella sua lista di inoltro mi dimentico
	{
		data->keyIdRmFF = -1;
		data->pipeRmFF = -1;
	}
	/* INVIO AL CLIENT
	 * type = out_exitRm_p
	 * sender =  SERVER(string) //non lo uso
	 * who = index of entry in Tab(string)
	 * mex = <Null>
	 */
	mail sendClient;
	fillPack (&sendClient, out_exitRm_p, 0, 0, "SERVER", pack->md.whoOrWhy);
	writePack (data->conUs.con.ds_sock, &sendClient);
	return 0;
}

int mexForwarding (mail *pack, thUserArg *data){
	/* RICEVUTO AL TH-room-tx
	 * type = in_mess_p
	 * sender =  roomName(string) ID:NAME //non lo uso
	 * who = frase messaggio in arrivo(string)
	 * mex = puntatore a mex, e dimesione del pacchetto da inviare nella write_pack DEVO CREARE IL BUFFER CONTINUO
	 */
	mex *m;
	m = pack->mex;
	char buf[pack->md.dim];
	memcpy (buf, pack->mex, sizeof (mexInfo));
	memcpy ((buf + sizeof (mexInfo)), m->text, pack->md.dim - sizeof (mexInfo));

	fillPack (&pack, out_mess_p, pack->md.dim, buf, pack->md.sender, pack->md.whoOrWhy);
	//inoltro al client cambiando tipo
	writePack (data->conUs.con.ds_sock, &pack);
	return 0;
}

/*==============================================================*/
/*==============================================================*/
/*==============================================================*/

/** [][][] TH-ROOM GENERICO, prima di specializzarsi, HA IL COMPITO DI creare le strutture **/
void *roomTh (thRoomArg *info){
	// dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
	int errorRet = pipe2 (info->fdPipe, O_DIRECT);
	if (errorRet != 0){
		printErrno ("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
		exit (-1);
	}
	insert_avl_node_S (rmAvlTree_Pipe, info->id, info->fdPipe[writeEndPipe]);

	// room path  ./"chatDir"/idKey:"name"  (inizio = ./ )
	int id;
	char inizio[stringLen], chatDir[stringLen], name[stringLen];
	sscanf (info->roomPath, "%[^/]/%[^/]/%d:%s", inizio, chatDir, &id, name);
	strncpy (info->roomName, name, stringLen);
	sprintf (info->idNameRm, "%d:%s", id, name);

	init_listHead_S (&info->mailList, fdOut);  //all'avvio della room la coda di inoltro è nulla

	dprintf (fdOut, "Th-ROOM %d create\n\tmyPath = %s\n\tfdPipe[writeEndPipe] = %d\n", info->id, info->roomPath,
	         info->fdPipe[writeEndPipe]);

	//i thRoom condividono lo stesso oggetto thRoomArg info, quindi stare attenti
	pthread_create (&info->tidRx, NULL, thRoomRX, info);
	pthread_create (&info->tidTx, NULL, thRoomTX, info);

	pthread_exit (NULL); //ho generato le componenti posso terminare
	return NULL;
}

/** #### TH-ROOM CON RUOLO DI RX **/
void *thRoomRX (thRoomArg *rData){
	mail packRecive, packSend;
	bool exit = true;
	int retTh = 0;
	while (exit){
		dprintf (fdOut, "[Rm-rx(%s)]wait message from [%d]pipe\n", rData->idNameRm, rData->fdPipe[readEndPipe]);

		if (readPack_inside (rData->fdPipe[readEndPipe], &packRecive) == -1){
			dprintf (STDERR_FILENO, "Read error, broken pipe\n");
			dprintf (STDERR_FILENO, "th-RoomRx %s in chiusura\n", rData->idNameRm);
			retTh = -1;
			break;
		}

		switch (packRecive.md.type){
			case in_join_p:
				if (joinRoom_inside (&packRecive, rData)){
					dprintf (STDERR_FILENO, "[Rm-rx(%s)]Impossible Join for Th-room\n", rData->idNameRm);
				}
				else{
					dprintf (fdOut, "[Rm-rx(%s)]New user [%s] Join in Room success\n", rData->idNameRm,
					         packRecive.md.sender);
				}
				break;
			case in_delRm_p:
				if (delRoom_inside (&packRecive, rData) == 0){
					dprintf (fdOut, "[Rm-rx(%s)]Destroy Th-room success\n", rData->idNameRm);
					//devo chiudere tutti i th-collegari a questo
					pthread_cancel (rData->tidTx);    //per ora state e type di default, meditare se modificare il type
					recursive_delete (rData->roomPath);
					exit = false;
				}
				else{
					dprintf (STDERR_FILENO, "[Rm-rx(%s)]Impossible Destroy this Th-room\n", rData->idNameRm);
				}
				break;
			case in_leave_p:
				if (leaveRoom_inside (&packRecive, rData, &exit)){
					dprintf (STDERR_FILENO, "[Rm-rx(%s)]Impossible leave user %s of this Th-room\n", rData->idNameRm,
					         packRecive.md.sender);
				}
				else{
					dprintf (fdOut, "[Rm-rx(%s)]Leave [%s]  from Room success\n", packRecive.md.sender,
					         rData->idNameRm);
				}
				break;
			case in_openRm_p:
				if (openRoom_inside (&packRecive, rData)){
					dprintf (STDERR_FILENO, "[Rm-rx(%s)]Impossible add user %s at mail-List of Th-room\n",
					         rData->idNameRm,
					         packRecive.md.sender);
				}
				else{
					dprintf (fdOut, "[Rm-rx(%s)]Add Open Room of [%s] success\n", rData->idNameRm,
					         packRecive.md.sender);
				}
				break;
			case in_exit_p:
				if (exitRoom_inside (&packRecive, rData)){
					dprintf (STDERR_FILENO, "[Rm-rx(%s)]Impossible remove node from mail-List of Th-room%s\n",
					         rData->idNameRm,
					         packRecive.md.sender);
				}
				else{
					dprintf (fdOut, "[Rm-rx(%s)]Remove mail-list node Room success\n", rData->idNameRm);
				}
				break;
			default:
				dprintf (fdDebug, "[Rm-rx-(%s)]Unexpected pack type = %d[%s]\n", rData->idNameRm, packRecive.md.type,
				         typeToText (packRecive.md.type));
				printPack (&packRecive, fdDebug);
				writePack_inside (rData->fdPipe[writeEndPipe], &packRecive);
				usleep (5000);   //dorme 5ms per permettere al rx o chi per lui di prendere il pacchetto
				break;
		}
		//freeMexPack (&packRecive);
		//freeMexPack (&packSend);
	}//#end while

	delete_avl_node_S (usAvlTree_Pipe, rData->id);   //mi levo dall'avl e inizio la pulizia
	sleep (1);
	pthread_cancel (rData->tidTx);
	close (rData->fdPipe[0]);
	close (rData->fdPipe[1]);
	freeMexPack (&packRecive);
	freeMexPack (&packSend);
	freeRoomArg (rData);
	pthread_exit (&retTh);
}

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI RX **/
int joinRoom_inside (mail *pack, thRoomArg *data){
	/* RICHIESTA JOIN A ROOM DA TH-USER
	 * type = in_join_p
	 * sender =  idkeyUs:name (string)
	 * who = firstFree_EntryIndex(string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex = <myPipe> (int)
	 */
	mail respond;
	int pipeUser = *((int *)pack->mex);
	free (pack->mex);

	dprintf (fdDebug, "Join th-Room recive in_join_p, and pipe user is %d pipe\n", pipeUser);
	//aggiungo alla mia tabella il nuovo user
	int addPos = addEntry (data->info->tab, pack->md.sender, atoi (pack->md.whoOrWhy));
	if (addPos == -1){
		dprintf (STDERR_FILENO, "Add Entry for join side room take error\n");
		fillPack (&respond, failed_p, 0, 0, "ROOM", "addEntry fail");
		writePack_inside (pipeUser, &respond);
		return -1;
	}

	char addPosBuf[16];
	sprintf (addPosBuf, "%d", addPos);
	fillPack (&respond, in_entryIndex_p, 0, 0, data->idNameRm, addPosBuf);
	/* RISPOSTA JOIN DA TH-ROOM (PARTICOLARE)
	 * type = in_entryIndex_p
	 * sender = idkeyRoom:name (string)
	 * who = addPos(string)
	 * mex = <Null>
	 */
	writePack_inside (pipeUser, &respond);
	return 0;
}

int delRoom_inside (mail *pack, thRoomArg *data){
	///eseguibile solo se sono amministratore della room
	/* RICHIESTA DELETE-ROOM DA TH-USER
	 * type = in_delRm_p
	 * sender =  userName(string) ID:NAME
	 * who = pipeRespond(string)
	 * mex = <Null>
	 */
	mail respondThUs;
	int pipeUserCallMe = atoi (pack->md.whoOrWhy);
	int chatAdmin = atoi (data->info->tab->head.name);

	//verifichiamo solo gli id per test amministratore
	if (chatAdmin != atoi (pack->md.sender)){        //non ero amministratore
		dprintf (STDERR_FILENO, "Utente %d non amministratore, è AMMINISTRATORE :%d\n", data->id, chatAdmin);
		fillPack (&respondThUs, failed_p, 0, 0, data->idNameRm, "Non hai il diritto");
		writePack_inside (pipeUserCallMe, &respondThUs);
		return -1;
	}

	delete_avl_node_S (rmAvlTree_Pipe, data->id); //mi tolgo dalle room ragiungibili
	sleep (1);   //aspetto per dare tempo di parlare a chi ancora avesse il mio recapito

	//Comincio a cancellare tutti gli utenti della chat, cancellando la room nella loro tabella.
	//se sono collegati gli invio un messaggio via pipe, altrimenti la cancello a mano
	mail respond;
	entry *en = data->info->tab->data;
	char pointStr[32];//string di en->point
	int idUserScr, pipeUserScr;
	for (int i = 0; i < data->info->tab->head.len; ++i){
		idUserScr = atoi (en[i].name);
		pipeUserScr = search_BFS_avl_S (usAvlTree_Pipe, idUserScr);
		if (pipeUserScr == pipeUserCallMe) //quando arrivo a me medesimo
		{
			///salta, quando rispondo success alla fine l'user chiamante la eliminerà
			continue;
		}
		switch (pipeUserScr){
			case -1:    //errore
				dprintf (STDERR_FILENO, "Search avl create error, abort\n");
				fillPack (&respond, failed_p, 0, 0, "ROOM", "search user take error");
				writePack_inside (pipeUserCallMe, &respond);
				return -1;
				break;
			case -2:    //non trovato, utente offline
				///non faccio nulla, al suo successivo login lo elimina da solo
				break;
			default:    //utente presente, gli invio il segnale
				sprintf (pointStr, "%d", en->point);
				fillPack (&respond, in_delRm_p, 0, 0, data->idNameRm, pointStr);
				/* INVIO AL TH-USER-TX
				 * type = in_delRm_p
				 * sender =  roomName(string) ID:NAME //non lo uso
				 * who = index of entry in Tab(string)
				 * mex = <Null>
				 */
				writePack_inside (pipeUserScr, &respond);
				break;
		}
	}   //#end for, inscritti eliminati

	//elimino la room sul fileSistem
	fillPack (&respond, success_p, 0, 0, data->idNameRm, "Deleting Room Success");
	writePack_inside (pipeUserCallMe, &respond);

	destroy_dlist_S (&data->mailList);
	return 0;    //da fuori chiudo l'altro thread room
}

int leaveRoom_inside (mail *pack, thRoomArg *data, int *exit){
	/* RICHIESTA LEAVE-ROOM ALLA ROMM
	 * type = in_leave_p
	 * sender =  idNameUs(string)
	 * who = IndexOfTable:myPipe(string)
	 * mex = <Null>
	 */
	mail respond;
	int indexTabRoom, pipeUs;
	sscanf (pack->md.whoOrWhy, "%d:%d", &indexTabRoom, &pipeUs);
	int idNameUs = atoi (pack->md.sender);

	//check utente da leavare è quello che lo sta chiedendo
	if (idNameUs == atoi (data->info->tab->data[indexTabRoom].name)){
		if (delEntry (data->info->tab, indexTabRoom)){
			dprintf (STDERR_FILENO, "(rm-TH) Impossible remove entry\n");
			fillPack (&respond, failed_p, 0, 0, data->idNameRm, "Impossible remove");
			writePack_inside (pipeUs, &respond);
			return -1;
		}
		dprintf (fdDebug, "(rm-TH) leave user Do\n");

		//check presenza in lista di inoltro
		dlist_p usNode = nodeSearchKey (&data->mailList, idNameUs);
		if (usNode) //se non nullo esiste e lo devo eliminare
		{
			deleteNodeByList (&data->mailList, usNode);
		}
		fillPack (&respond, success_p, 0, 0, data->idNameRm, "Entry remove");
		writePack_inside (pipeUs, &respond);

	}
	else{        //utente errato
		dprintf (STDERR_FILENO, "(rm-TH) user not the same (id):\n%s != %s\n", pack->md.sender,
		         data->info->tab->data[indexTabRoom].name);
		fillPack (&respond, failed_p, 0, 0, data->idNameRm, "You aren't user");
		writePack_inside (pipeUs, &respond);
		return -1;
	}

	//check se l'uscito era l'amministratore, devo rinominare  un nuovo l'amministratore
	if (strcmp (data->info->tab->head.name, pack->md.sender) == 0){
		firstFree *fr = &data->info->tab->head;
		entry *en;
		for (int i = 0; i < fr->len; ++i){
			en = &data->info->tab->data[i];
			if (!isEmptyEntry (en) && !isLastEntry (en)) //il primo non vuoto nuovo e non lastFree è il nuovo admin
			{
				if (renameFirstEntry (data->info->tab, en->name)){
					dprintf (STDERR_FILENO, "Admin in room '%s' can't be Change to %s\n", data->idNameRm, fr->name);
					perror ("cause of fail is:");
					return -1;
				}
				dprintf (fdOut, "Admin in room '%s' is now Change to %s\n", data->idNameRm, fr->name);
				return 0;
				break;
			}
		}

		//se termina il for la tabella non ha più utenti, room da eliminare
		dprintf (STDERR_FILENO, "New Admin not Found for room %s\nROOM DELETING", data->idNameRm);
		delete_avl_node_S (rmAvlTree_Pipe, data->id); //mi tolgo dalle room ragiungibili
		sleep (1);   //aspetto per dare tempo di parlare a chi ancora avesse il mio recapito
		pthread_cancel (data->tidTx);    //per ora state e type di defoult, meditare se modificare il type
		recursive_delete (data->roomPath);
		*exit = false;
		return 0;
	}
	return 0;
}

int openRoom_inside (mail *pack, thRoomArg *data){
	/* RICHIESTA OPEN A ROOM DA TH-USER
	 * type = in_openRm_p
	 * sender =  idkeyUs:name (string)
	 * who = my write End pipe (string) //la posizione in cui viene aggiunto nella mia tabella la room
	 * mex = <Nul>
	 */
	mail respond;
	int pipeUsRespond = atoi (pack->md.whoOrWhy);
	dlist_p node = makeNode (atoi (pack->md.sender), pipeUsRespond);
	if (!node){
		fillPack (&respond, failed_p, 0, 0, data->idNameRm, "Errore in create node");
		writePack_inside (pipeUsRespond, &respond);
		return -1;
	}
	add_head_dlist_S (&data->mailList, node);    //se entrambi i parametri sono non nulli non può fallire

	convRam *cpRam = copyConv (data->info->conv);
	if (!cpRam){
		fillPack (&respond, failed_p, 0, 0, data->idNameRm, "Errore in copy conv");
		writePack_inside (pipeUsRespond, &respond);
		return -1;
	}
	else{
		struct stat streamInfo;
		fstat (fileno (data->info->conv->stream), &streamInfo);

		fillPack (&respond, in_kConv_p, streamInfo.st_size, cpRam, data->idNameRm, data->idNameRm);
		/* RISPOSTA OPEN DA TH-ROOM (PARTICOLARE)
		 * type = in_kConv_p
		 * sender = idkeyRoom:name (string)
		 * who = idkeyRoom:name (string)
		 * dim = dimensione del file srotolato (sul file sistem)
		 * mex = <conversazione copiata fino a quel momento> (convRam *)
		 */
		writePack_inside (pipeUsRespond, &respond);
		//freeMexPack (&respond);
		return 0;
	}
}

int exitRoom_inside (mail *pack, thRoomArg *data){
	/* RICHIESTA EXIT A ROOM DA TH-USER
	 * type = in_exit_p
	 * sender =  idkeyUs:name (string)
	 * who = id_Chat_to_Exit(string)
	 * mex = <Null>
	 */
	dlist_p node = nodeSearchKey (&data->mailList, atoi (pack->md.sender));
	if (deleteNodeByList (&data->mailList, node)){
		return -1;
	}
	return 0;
}

/** #### TH-ROOM CON RUOLO DI TX **/
void *thRoomTX (thRoomArg *rData){
	mail packRead_in, sendClient;
	while (1){
		dprintf (fdOut, "[Rm-tx-(%s)] wait message from [%d] pipe\n", rData->idNameRm, rData->fdPipe[readEndPipe]);
		if (readPack_inside (rData->fdPipe[readEndPipe], &packRead_in)){
			switch (errno){
				case EBADF:
					dprintf (STDERR_FILENO, "readEndPipe=%d now is close\nClose Th\n", rData->fdPipe[readEndPipe]);
					freeMexPack (&packRead_in);
					freeMexPack (&sendClient);
					pthread_exit (NULL);  //uccide il th (il pthread_close ha fallito)
					break;
				default:
					break;
			}
		}

		switch (packRead_in.md.type){
			case in_mess_p:
				dprintf (fdOut, "[Rm-tx(%s)]Mex incoming in room, TEXT: %s\n", rData->idNameRm, packRead_in.mex);
				if (mexRecive_inside (&packRead_in, rData)){
					dprintf (STDERR_FILENO, "[Rm-tx(%s)]PROBLEM to Forwarding mex from %s\n", rData->idNameRm,
					         packRead_in.md.sender);
				}
				else{
					dprintf (fdOut, "[Rm-tx(%s)]Forwarding mex by Room success\n", rData->idNameRm);
				}
				break;

			default:
				//pacchetto non gestito, quindi non indirizzato a noi, lo ri infilo nella pipe
				dprintf (fdDebug, "[Rm-tx-(%s)]Unexpected pack type = %d[%s]\n", rData->idNameRm, packRead_in.md.type,
				         typeToText (packRead_in.md.type));
				writePack_inside (rData->fdPipe[writeEndPipe], &packRead_in);
				usleep (5000);   //dorme 5ms per permettere al rx o chi per lui di prendere il pacchetto
				break;
		}

		//freeMexPack (&packRead_in);
		//freeMexPack (&sendClient);
	}//#end while
}

/** FUNZIONI DI SUPPORTO PER TH-ROOM CON RUOLO DI TX **/

int mexRecive_inside (mail *pack, thRoomArg *data){
	/* RICEZIONE MESSAGGIO DA TH-USER
	 * type = in_mess_p
	 * sender = usPipe:IDKey:NAME [%d:%d:%s]
	 * who = idNumber_ di controllo, da mantenere intatto
	 * mex = testoMessaggio
	 * dim = dim messaggio (\0 comreso)
	 */
	mail usRespond;
	int usPipe, idkeyUs;
	sscanf (pack->md.sender, "%d:%d", &usPipe, &idkeyUs);

	//segno il nodo inviante
	dlist_p senderMexNode = nodeSearchKey (&data->mailList, idkeyUs);
	if (!senderMexNode){
		fillPack (&usRespond, failed_p, 0, 0, data->idNameRm, "Not in Room forwarding List");
		writePack_inside (usPipe, &usRespond);
		return -1;
	}

	mex *newMex = makeMex (pack->mex, idkeyUs);
	if (!newMex){
		fillPack (&usRespond, failed_p, 0, 0, data->idNameRm, "Error in make Message type");
		writePack_inside (usPipe, &usRespond);
		return -1;
	}
	//printMex (newMex, STDERR_FILENO);

	if (addMex (data->info->conv, newMex)){
		dprintf (STDERR_FILENO, "[mexRecive_inside]Add mex at conv FAIL\n");
		fillPack (&usRespond, failed_p, 0, 0, data->idNameRm, "Error in adding mex on conv (maybe saved)");
		writePack_inside (usPipe, &usRespond);
		return -1;
	}
	//dprintf (fdDebug, "[mexRecive_inside] Add mex on conv Success\n");
	fillPack (&usRespond, out_messSuccess_p, 0, 0, data->idNameRm, pack->md.whoOrWhy);
	writePack_inside (usPipe, &usRespond);

	/*#################################### Inoltro nella mail-list ####################################*/
	dprintf (fdDebug, "[mex_inside_forwarding]\ndata->mailList.head=%p, *data->mailList.head=%p\n", data->mailList.head,
	         *data->mailList.head);
	if (!data->mailList.head || !*data->mailList.head){
		dprintf (STDERR_FILENO, "[mex_inside_forwarding]head or first node is NULL!\n");
		return -1;
	}
	dlist_p tmp;
	tmp = *data->mailList.head;
	listData_p dt;

	//tmp = *head->head;
	int count = 0;
	dprintf (fdDebug, "===[head of LIST]===\n");
	dprintf (fdDebug, "HEAD[0]=%p\n", tmp);
	do{
		dt = (listData_p)tmp->data;
		dprintf (fdDebug, "-->node[%d]=%d:%d\n", count, dt->keyId, dt->fdPipeSend);
		count++;
		tmp = tmp->next;
	}
	while (tmp != *data->mailList.head);
	dprintf (fdDebug, "##################\n#### END LIST ####\n##################\n");
	sleep (1);

	dt = (listData_p)tmp->data;

	//invio il mexPack come un puntatore a newMex, e per dim la dimensione da spacchettato per writePack
	//int len = sizeof(newMex->info) + pack->md.dim;
	fillPack (&usRespond, in_mess_p, sizeof (newMex->info) + pack->md.dim, newMex, data->idNameRm, "New mex incoming");
	tmp = *data->mailList.head;
	dprintf (fdDebug, "inizio inoltro\nsenderNode=%p [%d]\n", senderMexNode, ((listData_p)senderMexNode->data)->keyId);
	//todo da far funzionare
	/*do{
		if (tmp != senderMexNode){
			dprintf (fdDebug, "senderNode=%p [%d], tmpNode=%p [%d]\n", senderMexNode,
			         ((listData_p)senderMexNode->data)->keyId, tmp,
			         ((listData_p)tmp->data)->keyId);    //*tmp->data casca sul 1° intero
			//sleep (1);
			if (writePack_inside (dt->fdPipeSend, &usRespond)){
				switch (errno){
					case EPIPE:
						//pipe non più raggiungibile, tolgo dalla lista di inoltro
						dprintf (STDERR_FILENO, "In Forwarding mex user=%d impossible to reach\nDeleting from list\n",
						         dt->keyId);
						deleteNodeByList (&data->mailList, tmp);
						break;
					default:
						dprintf (STDERR_FILENO, "In Forwarding mex user=%d unknown error :%s", dt->keyId,
						         strerror (errno));
						break;
				}
			}
		}
		tmp = tmp->next;
	}
	while (tmp != *data->mailList.head);
	*/
	//freeMexPack (&usRespond);
	return 0;

}

/** SEND PACK_inside e WRITE PACK_inside**/
int readPack_inside (int fdPipe, mail *pack){
	int iterContr = 0; // vediamo se la read fallisce
	ssize_t bRead = 0;
	ssize_t ret = 0;
	//dprintf (fdDebug, "readPack_inside Funx\n");
	sigset_t newSet, oltSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oltSet);

	int iterazione = 0;
	do{
		iterazione++;
		ret = read (fdPipe, pack + bRead, sizeof (mail) - bRead);
		if (ret == -1){
			dprintf (STDERR_FILENO, "[reedPack_inside]Error @:\niterazione=[%d], read pipe=[%d]\n", iterazione, fdPipe);
			perror ("cause:");
			return -1;
		}
		if (ret == 0){
			iterContr++;
			if (iterContr > 4){
				dprintf (STDERR_FILENO, "[reedPack_inside]Seems Read can't go further; test connection... [%d]\n",
				         iterContr);
				if (testConnection_inside (fdPipe) == -1){
					dprintf (STDERR_FILENO, "[reedPack_inside]Test Fail\n");
					return -1;
				}
				else{
					iterContr = 0;
				}
			}
		}
		else{
			iterContr = 0;
		}
		bRead += ret;
	}
	while (sizeof (mail) - bRead != 0);

	pthread_sigmask (SIG_SETMASK, &oltSet, &newSet);   //restora tutto
	return 0;
}

int writePack_inside (int fdPipe, mail *pack) //dentro il thArg deve essere puntato un mail
{
	//rispetto alla versione normale invio il puntatore di mex, per cui prima lo si mallocca ,e il ricevente lo freeza
	/// la funzione si aspetta che il buffer non sia modificato durante l'invio
	ssize_t bWrite = 0;
	ssize_t ret = 0;
	int iterazione = 0;
	sigset_t newSet, oltSet;
	sigfillset (&newSet);
	pthread_sigmask (SIG_SETMASK, &newSet, &oltSet);
	do{
		iterazione++;
		ret = write (fdPipe, pack + bWrite, sizeof (mail) - bWrite); //original
		if (ret == -1){
			switch (errno){
				case EPIPE:
					dprintf (STDERR_FILENO, "[writePack_inside]Pipe break @:\niterazione=[%d], write pipe=[%d]\n",
					         iterazione, fdPipe);
					return -1;
					//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
				default:
					perror ("[writePack_inside]Error:\n");
					return -1;
					break;
			}
		}
		bWrite += ret;
	}
	while (sizeof (mail) - bWrite != 0);
	pthread_sigmask (SIG_SETMASK, &oltSet, &newSet);   //restora tutto
	return 0;
}

int testConnection_inside (int fdPipe){
	mail packTest;
	fillPack (&packTest, out_test_p, 0, 0, "SERVER", "testing_code");

	if (writePack_inside (fdPipe, &packTest) == -1){
		return -1;
	}
	return 0;
}

void freeUserArg (thUserArg *p){
	freeInfoUser (p->info);
	free (p);
}

void freeRoomArg (thRoomArg *p){
	freeInfoChat (p->info);
	free (p);
}

/** List utility**/

dlist_p makeNode (int keyId, int fdPSend){
	dlist_p node = calloc (1, sizeof (dlist_t));
	if (!node){
		return NULL;
	}
	listData_p data = calloc (1, sizeof (listData_p));
	if (!data){
		free (node);
		return NULL;
	}
	data->keyId = keyId;
	data->fdPipeSend = fdPSend;
	node->data = (void *)data;
	return node;
}

dlist_p nodeSearchKey (listHead_S_p head, int key){
	//NULL non trovato, o per lista vuota o perchè non c'è
	//pointer al nodo con la key corripondente

	listData_p dataUs;

	lockReadSem (head->semId);

	if (!head->head || !*head->head){
		dprintf (STDERR_FILENO, "[dslib Search]head or first node is NULL!\n");
		unlockReadSem (head->semId);
		return NULL;
	}

	dlist_p tmp;
	int count = 0;
	/*
	tmp = *head->head;
	dprintf (fdDebug, "===[head of LIST]===\n");
	dprintf (fdDebug, "&HEAD[0]=%p\n",tmp);
	do {
		dataUs = (listData_p)tmp->data;
		dprintf (fdDebug, "-->node[%d]=%d:%d\n",count, dataUs->keyId,dataUs->fdPipeSend);
		count++;
		tmp = tmp->next;
	} while (tmp != *head->head);
	dprintf (fdDebug, "##################\n#### END LIST ####\n##################\n");
	sleep (1);
	*/
	count = 0;
	tmp = *head->head;
	do{
		dataUs = (listData_p)tmp->data;
		if (dataUs->keyId == key){
			unlockReadSem (head->semId);
			dprintf (fdDebug, "[nodeSearchKey]node[%d]have same key(%d)\n", count, key);
			return tmp;
		}
		tmp = tmp->next;
		count++;
	}
	while (tmp != *head->head);

	unlockReadSem (head->semId);
	dprintf (fdDebug, "[nodeSearchKey]No node have same key(%d)\n", key);
	return NULL;
}

int deleteNodeByList (listHead_S_p head, dlist_p nodeDel){
	//-1 error
	//0 deleted

	if (!nodeDel){
		dprintf (STDERR_FILENO, "INVALID NODE\n");
		return -1;
	}

	lockWriteSem (head->semId);

	*head->head = nodeDel;   //è un azzardo ma l'istruzione successiva sposta il puntatore al valore opportuno
	delete_head_dlist (head->head);   //non può dare errore così passati i parametri
	//ora head.head è posizionato su prossimo della coda NELL'heap e non nello stack

	unlockWriteSem (head->semId);
	return 0;
}

char *typeToText (int type){

	switch (type){
		case success_p:
			return "success_p";
		case failed_p:
			return "failed_p";
		case out_mess_p:
			return "mess_p";
		case out_messSuccess_p:
			return "messSuccess_p";
		case out_test_p:
			return "test_p";
		case out_login_p:
			return "login_p";
		case out_logout_p:
			return "logout_p";
		case out_delUs_p:
			return "delUs_p";
		case out_mkUser_p:
			return "mkUser_p";
		case out_dataUs_p:
			return "dataUs_p";
		case out_kConv_p:
			return "kConv_p";
		case out_mkRoom_p:
			return "mkRoom_p";
		case out_joinRm_p:
			return "joinRm_p";
		case out_openRm_p:
			return "openRm_p";
		case out_dataRm_p:
			return "dataRm_p";
		case out_leaveRm_p:
			return "leaveRm_p";
		case out_delRm_p:
			return "delRm_p";
		case out_exitRm_p:
			return "exitRm_p";
		case in_join_p:
			return "in_join_p";
		case in_delRm_p:
			return "in_delRm_p";
		case in_leave_p:
			return "in_leave_p";
		case in_entryIndex_p:
			return "in_entryIndex_p";
		case in_openRm_p:
			return "in_openRm_p";
		case in_exit_p:
			return "in_exit_p";
		case in_kConv_p:
			return "in_kConv_p";
		case in_mess_p:
			return "in_mess_p";
		default:
			return "Not expected type";
	}
}

void printPack (mail *pack, int fd){
	dprintf (fd, "######[][]I METADATI SONO[][]######\n");
	dprintf (fd, "Dim pack = %ld\n", pack->md.dim);
	dprintf (fd, "Type = %d[%s]\n", pack->md.type, typeToText (pack->md.type));
	dprintf (fd, "Sender = %s\n", pack->md.sender);
	dprintf (fd, "whoOrWhy = %s\n", pack->md.whoOrWhy);
	dprintf (fd, "------[][]IL MESSAGGIO[][]------\n");
	dprintf (fd, "MEX = %p\n", pack->mex);
}