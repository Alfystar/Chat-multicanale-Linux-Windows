//
// Created by alfylinux on 30/09/18.
//

#include "../include/thFunx.h"



void *acceptTh(thAcceptArg *info) {

    thUserArg *arg;

	while (1) {
        arg = malloc(sizeof(thUserArg));
        dprintf(fdOut, "acceptTh Creato, arg = %p creato.\n", arg);
		//definisco gli argomenti da passare al th user come puntati da arg di thConnArg.arg
        arg->id = -1;
		arg->conUs = info->conInfo; //copia negli argomenti del th una copia totale della connessione del server
        arg->info = NULL;
		arg->userName[0] = 0;

        if (acceptCreate(&info->conInfo.con, userTh, arg) == -1) {
	        dprintf(STDERR_FILENO, "errore in accept\n");
        }
	}
	free(info);

	///Codice funzionante per la creazione di un thread user passandogli correttamente il parametro info
	/*
	infoUser *info;
	long idKey = atoi(info->pathName);       //essendo myname xx:TEXT, la funzione termina ai : e ottengo la key
	char userDir[128];
	sprintf(userDir, "./%s/%ld:%s", userDirName, idKey, argv[1]);
	makeThUser(idKey, userDir, info);
	dprintf(fdOut, "USER th creato, idKey=%d\n", idKey);
	*/
	return NULL;
}

void *userTh(thConnArg *info) {
	thUserArg *arg = info->arg; //da impostare in base al login
	arg->conUs.con = info->con; //copio i dati di info
	free(info); //il tipo thConnArg non serve più tutto è stato copiato

	dprintf(fdOut, "TH-User creato\nId = %d\nIn Attesa primo pack\n", arg->id);
	//pause();
	mail *pack = malloc(sizeof(mail));

	readPack(arg->conUs.con.ds_sock, pack);        //ottengo il primo pack per capire il da fare.
	int idKey;
	switch (pack->md.type) {
		case login_p:
			//login
			loginServerSide(pack, arg);
			break;
		case mkUser_p:
			//mkuser funx
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
	insert_avl_node_S(usAvlPipe, arg->id, fdUserPipe[1]);

    pthread_t tidRX, tidTX;

	pthread_create(&tidRX, NULL, thrServRX, arg);
    //pthread_create(&tidTX,NULL, thrServTX,info);

	while (1) pause();
	free(arg);  //todo da sostituire con free_thUserArg quando fatta
	pthread_exit(NULL);

	return NULL;
}

int loginServerSide(mail *pack, thUserArg *data) {

	dprintf(fdDebug, "LOGIN FASE\nUtente = %s\n Type=%d", pack->md.sender, pack->md.type);
	/*
	 * Login:
	 * type=login_p
	 * Sender=xxxx
	 * whoOrWhy=idKey (string)
	 * dim=0
	 */
	if (makeThUser(atoi(pack->md.whoOrWhy), data)) {
		perror("Impossible to create new ThUser of register User :");
		return -1;
	}

	/// DEFINIRE DOVE TROVARE GLI UTENTI
	pack->md.type = success_p;
	//todo scrivere il resto della risposta
	writePack(data->conUs.con.ds_sock, pack);


	return 0;
}

int loginUserSide(int ds_sock, mail *pack) {

	char buffUser[24];
	char buffPass[28];

	printf("Inserire credenziali per login.\nUtente: ");
	scanf("%s", buffUser);
	fflush(stdin);
	printf("\nPassword:");
	scanf("%s", buffPass);
	fflush(stdin);
	printf("\n");

	if (fillPack(pack, 0, 0, NULL, buffUser, buffPass) == -1) {
		return (-1);
	}
	writePack(ds_sock, pack);

	readPack(ds_sock, pack);


	printf("Login effettuato\n");

	return 0;
}

void *thrServRX(thUserArg *argTh) {

    mail packRecive;

    while (1) {
        dprintf(fdOut, "thrServRx %d in attesa di messaggio da %d sock\n", argTh->id, argTh->conUs.con.ds_sock);
        if (readPack(argTh->conUs.con.ds_sock, &packRecive) == -1) {
            dprintf(STDERR_FILENO, "Read error, broken pipe\n");
            sleep(1);
            exit(-1); // todo gestione broken pipe e uscita thread
        }

        dprintf(fdOut, "Numero byte pacchetto: %ld\n", packRecive.md.dim);
        dprintf(fdOut, "Stringa da client: %s\n\n", packRecive.mex);

	    writePack(argTh->conUs.con.ds_sock, &packRecive);

        if (strcmp(packRecive.mex, "quit") == 0) {
            break;
        }


        //writePack(); da aggiungere il selettore chat
    }
	close(argTh->conUs.con.ds_sock);
    //free(argTh); //todo creare i free per ogni tipo di dato !!!!
    pthread_exit(0);
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
	insert_avl_node_S(rmAvlPipe, info->id, fdRoomPipe[1]);

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

///Funzione chiamata da cmd per testare i th
int makeThUser(int keyId, thUserArg *argUs) {
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