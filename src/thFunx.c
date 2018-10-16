//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"



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

	///Codice funzionante per la creazione di un thread user passandogli correttamente il parametro info
	/*
	infoUser *info;
	long idKey = atoi(info->pathName);       //essendo myname xx:TEXT, la funzione termina ai : e ottengo la key
	char userDir[128];
	sprintf(userDir, "./%s/%ld:%s", userDirName, idKey, argv[1]);
	setUpThUser(idKey, userDir, info);
	dprintf(fdOut, "USER th creato, idKey=%d\n", idKey);
	*/
	return NULL;
}

void *userTh(thConnArg *info) {
	thUserArg *arg = info->arg; //da impostare in base al login, l'accept lascia campi "vuoti"
	arg->conUs.con = info->con; //copio i dati di info
	free(info); //il tipo thConnArg non serve più tutto è stato copiato

	dprintf(fdOut, "TH-User creato, In Attesa primo pack\n");
	//pause();
	mail *pack = malloc(sizeof(mail));

	readPack(arg->conUs.con.ds_sock, pack);        //ottengo il primo pack per capire il da fare.
	int idKey;
	///in base al tipo di connessione si riepe arg con i giusti dati
	switch (pack->md.type) {
		case login_p:
			if (loginServerSide(pack, arg)) {
				perror("login fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
				pthread_exit(NULL);
			}
			break;
		case mkUser_p:
			if (mkUserServerSide(pack, arg)) {
				perror("make New User fase fail :");
				dprintf(STDERR_FILENO, "Shutdown Th %d\n", arg->id);
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

	pthread_create(&tidRX, NULL, thrServRX, arg);
    //pthread_create(&tidTX,NULL, thrServTX,info);
	pthread_exit(-1); // todo gestione broken pipe e uscita thread

	while (1) pause();
	free(arg);  //todo da sostituire con free_thUserArg quando fatta
	pthread_exit(NULL);

	return NULL;
}

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

	for (int i = 0; i < head->len; i++) {
		if (isEmptyEntry(&cell[i]) == 1) {
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

	int sizeTab = (head->len) * sizeof(entry) + sizeof(firstFree);
	char *mex = malloc(sizeTab);

	memcpy(mex, head, sizeof(firstFree));
	memcpy(mex + sizeof(firstFree), cell, sizeTab - sizeof(firstFree));

	/// Invio risposta affermativa
	fillPack(&response, dataUs_p, sizeTab, mex, "Server", NULL);
	writePack(data->conUs.con.ds_sock, &response);
	free(mex);

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

	firstFree *head = &infoNewUs->tab->head;
	entry *cell = infoNewUs->tab->data;


	int sizeTab = (head->len) * sizeof(entry) + sizeof(firstFree);
	char *mex = malloc(sizeTab);

	memcpy(mex, head, sizeof(firstFree));
	memcpy(mex + sizeof(firstFree), cell, sizeTab - sizeof(firstFree));

	/// Invio dataSend
	char idSend[16];
	sprintf(idSend, "%d", data->id);
	fillPack(&response, dataUs_p, sizeTab, mex, "Server", idSend);
	writePack(data->conUs.con.ds_sock, &response);

	return 0;
}

void *thrServRX(thUserArg *argTh) {

    mail packRecive;
	mail packSend;

    while (1) {
        dprintf(fdOut, "thrServRx %d in attesa di messaggio da %d sock\n", argTh->id, argTh->conUs.con.ds_sock);

        if (readPack(argTh->conUs.con.ds_sock, &packRecive) == -1) {
            dprintf(STDERR_FILENO, "Read error, broken pipe\n");
	        dprintf(STDERR_FILENO, "thrServRx %d in chiusura\n", argTh->id);
	        sleep(1);
	        free(argTh); //todo creare i free per ogni tipo di dato !!!!
	        pthread_exit(-1); // todo gestione broken pipe e uscita thread
        }
	    dprintf(fdDebug, "thrServRx %d ricevuto type=%d\n", packRecive.md.type);
	    printPack(&packRecive);
	    switch (packRecive.md.type) {
		    case mkRoom_p:
			    if (mkRoomSocket(&packRecive)) {
				    dprintf(STDERR_FILENO, "mkRoom coommand from socket fail\n");
				    fillPack(&packSend, failed_p, 0, 0, "Server", "Impossible Create room");
				    writePack(argTh->conUs.con.ds_sock, &packSend);
			    } else {
				    fillPack(&packSend, success_p, 0, 0, "Server", "Room-Create");
				    writePack(argTh->conUs.con.ds_sock, &packSend);
			    }
			    break;
		    case joinRm_p:
			    break;
		    case mess_p:
			    break;

		    default:
			    dprintf(fdDebug, "thrServRx %d ricevuto type=%d, NON GESTITO\n", packRecive.md.type);
			    printPack(&packRecive);
			    break;
	    }

	    dprintf(fdDebug, "Numero byte pacchetto: %ld\n", packRecive.md.dim);
        dprintf(fdOut, "Stringa da client: %s\n\n", packRecive.mex);

	    writePack(argTh->conUs.con.ds_sock, &packRecive);

        if (strcmp(packRecive.mex, "quit") == 0) {
            break;
        }

	    if (!packRecive.mex) free(packRecive.mex);
    }
	close(argTh->conUs.con.ds_sock);
	free(argTh); //todo creare i free per ogni tipo di dato !!!!
    pthread_exit(0);
}

int mkRoomSocket(mail *pack) {
	/*
	 * type= mkRoom_p
	 * sender= user (string) //non lo uso
	 * who=id (string)
	 * mex="<nameRoom>
	 */
	nameList *user = userExist();
	//user = idKey:Name
	char nameUsAdmin[64];
	long idKey;       //essendo myname xx:TEXT, la funzione termina ai : e ottengo la key

	int want = idSearch(user, atoi(pack->md.whoOrWhy));
	dprintf(fdDebug, "user->names[want]=%s\n", user->names[want]);
	sscanf(user->names[want], "%ld:%s", &idKey, nameUsAdmin);

	infoChat *info = newRoom(pack->mex, atoi(pack->md.whoOrWhy));

	if (info == 0) {
		dprintf(STDERR_FILENO, "creazione della chat impossibile");
		nameListFree(user);
		return -1;
	}
	char roomDir[128];
	sprintf(roomDir, "./%s/%s", chatDirName, user->names[want]);
	makeThRoom(idKey, roomDir, info);
	dprintf(fdOut, "ROOM th creato, idKey=%d\n", idKey);
	nameListFree(user);
	sleep(5);
	return 0;

}

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

void *roomTh(thRoomArg *info) {
    int fdRoomPipe[2];
    int errorRet = pipe2(fdRoomPipe,
                         O_DIRECT); // dal manuale: fd[0] refers to the read end of the pipe. fd[1] refers to the write end of the pipe.
    if (errorRet != 0) {
        printErrno("La creazione della pipe per il Th-room ha dato l'errore", errorRet);
        exit(-1);
    }
	insert_avl_node_S(rmAvlTree_Pipe, info->id, fdRoomPipe[1]);

    dprintf(fdOut, "Ciao sono Un Tr-ROOM\n\tsono la %d\tmi chiamo %s\n\tmi ragiungi da %d\n", info->id, info->roomPath,
            fdRoomPipe[1]);
	pause();
	free(info);
	return NULL;

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