/*
 * Circular Doubly Linked List implementation
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

#include "../include/dlist.h"
#include "../../globalSet.h"
#include "../include/common.h"


/*
 * Add a node to the head of dlist
 */
int add_head_dlist(dlist_pp head, dlist_p node) {
	if (!node) {    //nodo non nullo
		dprintf(STDERR_FILENO, "[dlist]node is NULL!\n");
		return -1;
	}

	if (!head) {    //puntatore del puntatore al nodo testa non nullo
		dprintf(STDERR_FILENO, "[dlist]head is NULL!\n");
		return -1;
	}

	if (!*head) {   //se il puntatote ai nodi è 0 allora nodo diventa la testa e si auto referenzia
		node->next = node;
		node->prev = node;
		*head = node;

		return 0;
	}
	//nodo viene messo in testa e head slitta
	node->next = *head; /* Current head become head->next */
	node->prev = (*head)->prev;
	(*head)->prev->next = node;
	(*head)->prev = node;
	*head = node;

	return 0;
}

/*
 * Get the value in the head node of dlist
 * The node is not deleted
 */
void *get_head_dlist(dlist_pp head) {
	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]head or first node is NULL!\n");
		return NULL;
	}

	return (*head)->data;
}

/*
 * Get the value in the tail node of dlist
 * The node is not deleted
 */
void *get_tail_dlist(dlist_pp head) {
	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]head or first node is NULL!\n");
		return NULL;
	}

	return (*head)->prev->data;
}

/*
 * Delete the head node of dlist
 */
int delete_head_dlist(dlist_pp head) {
	dlist_p tmp;

	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]No nodes to delete!\n");
		return -1;
	}

	if (*head == (*head)->next) {   //elimina se stesso
		free(*head);
		*head = NULL;
		return 0;
	}

	tmp = *head;
	free(tmp->data);
	(*head)->data = NULL;
	(*head)->prev->next = (*head)->next;
	(*head)->next->prev = (*head)->prev;
	*head = (*head)->next; /* head->next becomes next head */

	free(tmp);

	return 0;
}

/*
 * Delete the tail node of dlist
 */
int delete_tail_dlist(dlist_pp head) {
	dlist_p tmp;

	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]head or first node is NULL!\n");
		return -1;
	}

	if (*head == (*head)->next) {   //elimina se stesso
		free(*head);
		*head = NULL;
		return 0;
	}

	tmp = (*head)->prev;
	free(tmp->data);
	tmp->data = NULL;
	tmp->prev->next = tmp->next; /* tail->prev becomes new tail */
	tmp->next->prev = tmp->prev;

	free(tmp);

	return 0;
}

/*
 * Deallocate all memory and destroy the dlist
 * Returns the number of nodes deleted
 */
int destroy_dlist(dlist_pp head) {
	dlist_p tmp;
	int deleted = 0;

	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]No nodes to delete.\n");
		return -1;
	}

	/* Set tail->next to NULL to end deletion loop */
	(*head)->prev->next = NULL;

	while (*head) {
		tmp = *head;
		free(tmp->data);
		(*head)->data = NULL;
		(*head)->prev = NULL;
		*head = (*head)->next;

		free(tmp);
		deleted++;
	}
	//alla fine head dovrebbe tornare Null
	return deleted;
}

/*
 * Count the total number of nodes in the dlist
 */
int count_nodes_dlist(dlist_pp head) {
	dlist_p tmp;
	int count = 0;

	if (!head || !*head) {
		dprintf(STDERR_FILENO, "[dlist]head or first node is NULL!\n");
		return -1;
	}

	tmp = *head;

	do {
		count++;
		tmp = tmp->next;
	} while (tmp != *head);

	return count;
}


int add_head_dlist_S(listHead_S_p head, dlist_p node) {
	int ret;
	lockWriteSem(head->semId);
	ret = add_head_dlist(head->head, node);
	unlockWriteSem(head->semId);
	return ret;
}

/* Get the head of list */
void *get_head_dlist_S(listHead_S_p head) {
	void *data;
	lockReadSem(head->semId);
	data = get_head_dlist(head->head);
	unlockReadSem(head->semId);
	return data;
}

/* Get the tail of list */
void *get_tail_dlist_S(listHead_S_p head) {
	void *data;
	lockReadSem(head->semId);
	data = get_tail_dlist(head->head);
	unlockReadSem(head->semId);
	return data;
}

/* Delete the head of list */
int delete_head_dlist_S(listHead_S_p head) {
	int ret;
	lockWriteSem(head->semId);
	ret = delete_head_dlist(head->head);
	unlockWriteSem(head->semId);
	return ret;
}

/* Delete the tail of list */
int delete_tail_dlist_S(listHead_S_p head) {
	int ret;
	lockWriteSem(head->semId);
	ret = delete_tail_dlist(head->head);
	unlockWriteSem(head->semId);
	return ret;
}

/* Clean up list */
int destroy_dlist_S(listHead_S_p head) {
	int ret;
	lockWriteSem(head->semId);
	ret = destroy_dlist(head->head);
	free(head->head);
	unlockWriteSem(head->semId);
	return ret;
}

/* Count total nodes in list */
int count_nodes_dlist_S(listHead_S_p head) {
	int ret;
	lockReadSem(head->semId);
	ret = count_nodes_dlist(head->head);
	unlockReadSem(head->semId);
	return ret;
}


int init_listHead(listHead_S_p head, int fd) {
	// -1 error: see errno
	// -2 just inizialize

	if (head->head) {
		dprintf(STDERR_FILENO, "[dlist]List just init.\n");
		return -2;
	}

	head->head = calloc(1, sizeof(dlist_p *));


	head->semId = semget(IPC_PRIVATE, 3, IPC_CREAT | IPC_EXCL | 0666);
	if (head->semId == -1) {
		perror("Create Sem-s take error:");
		return -1;
	}

	//enum semName {wantWrite=0,readWorking=1,writeWorking=2}; number is Id of sem
	unsigned short semStartVal[3] = {0, 0, 1};

	//setup 3 semaphore in system5
	if (semctl(head->semId, 0, SETALL, semStartVal)) {
		perror("set Sem take error:");
		return -1;
	}

	//dprintf(fdDebug, "SEMAFORO Avl CREATO\n");
	semInfo(head->semId, fd);

	return 0;
}