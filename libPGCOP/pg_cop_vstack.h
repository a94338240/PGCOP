/*
    PGCOP - PocoGraph Component Oriented Platform.
    Copyright (C) 2013  David Wu <david@pocograph.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PG_COP_VSTACK_H
#define PG_COP_VSTACK_H

#include "list.h"
#include <semaphore.h>

typedef enum {
  VSTACK_TYPE_U8,
  VSTACK_TYPE_I8,
  VSTACK_TYPE_U16,
  VSTACK_TYPE_I16,
  VSTACK_TYPE_U32,
  VSTACK_TYPE_I32,
  VSTACK_TYPE_STRING,
  VSTACK_TYPE_DATASIZE
} pg_cop_vstack_type_t;

typedef enum {
  VSTACK_OK,
  VSTACK_NO_TYPE,
  VSTACK_WRONG_TYPE,
  VSTACK_OVERFLOW,
  VSTACK_EMPTY,
  VSTACK_BROKEN
} pg_cop_vstack_state_t;

typedef enum {
  VSTACK_SCOPE_THREAD,
  // VSTACK_SCOPE_PROCESS,
  // VSTACK_SCOPE_NETWORK
} pg_cop_vstack_scope_t;

typedef struct {
  int id;
  int size;
  int top;
  pg_cop_vstack_scope_t scope;
  char data_area[];
} pg_cop_vstack_t;

pg_cop_vstack_t *pg_cop_vstack_new(int id, int size, pg_cop_vstack_scope_t);
void pg_cop_vstack_destroy(pg_cop_vstack_t *);
pg_cop_vstack_state_t pg_cop_vstack_push(pg_cop_vstack_t *vstack, pg_cop_vstack_type_t type, ...);
pg_cop_vstack_state_t pg_cop_vstack_pop(pg_cop_vstack_t *vstack, pg_cop_vstack_type_t type, ...);
int pg_cop_vstack_pick_type(pg_cop_vstack_t *vstack, pg_cop_vstack_type_t*);
pg_cop_vstack_state_t pg_cop_vstack_transfer(pg_cop_vstack_t *s_vstack, 
                                             pg_cop_vstack_t *d_vstack);

#endif /* PG_COP_VSTACK_H */
