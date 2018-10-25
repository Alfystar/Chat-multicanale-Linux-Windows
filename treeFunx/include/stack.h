/*
 * Stack abstraction
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

#include "dlist.h"

#pragma once

/* Stack ADT using a circular doubly linked list */
typedef struct {
	dlist_pp head;
} d_stack_t, *d_stack_p;

/* Create a new Stack */
d_stack_p get_stack(void);

/* Push a value to Stack */
bool push(d_stack_p stack, void *val);

/* Pop a value from Stack */
void *pop(d_stack_p stack);

/* Check if a stack is empty */
bool isStackEmpty(d_stack_p stack);

/* Clean up Stack */
bool destroy_stack(d_stack_p stack);
