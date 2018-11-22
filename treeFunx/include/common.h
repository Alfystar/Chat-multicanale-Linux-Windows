/*
 * Common header file for dslib
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
 * Copyright (C) 2015 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dslib.  If not, see <http://www.gnu.org/licenses/>.
 */

///NON USATA AL SUO POSTO LA LIBRERIA STANDARD #include <stdbool.h>

#ifndef COMMON_H
#define COMMON_H

#include "log.h"


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

//#include "../../globalSet.h"
extern int fdDebug;
extern int fdOut;

#define LEFT  0
#define RIGHT 1

#define TRUE true
#define FALSE false

enum semName {
	wantWrite = 0, readWorking = 1, writeWorking = 2
};

void semInfo(int semId, int fd);

int lockWriteSem(int semId);

int unlockWriteSem(int semId);

int lockReadSem(int semId);

int unlockReadSem(int semId);

#endif