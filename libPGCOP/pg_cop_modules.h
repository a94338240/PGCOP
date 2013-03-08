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

#ifndef PG_COP_MODULES_H
#define PG_COP_MODULES_H

#include "list.h"
#include "pg_cop_debug.h"
#include "pg_cop_util.h"
#include <pthread.h>

typedef struct _pg_cop_module_t pg_cop_module_t;

typedef struct {
  const char *name;
} pg_cop_module_info_t;

typedef struct {
  int (*init)(int, char **);
  void *(*start)(struct _pg_cop_module_t *);
} pg_cop_module_hooks_t;

typedef struct _pg_cop_module_t {
  void *dl_handle;
  pthread_t thread;
  pthread_attr_t thread_attr;
  pg_cop_module_info_t *info;
  pg_cop_module_hooks_t *hooks;
  struct list_head list_head;
} pg_cop_module_t;

extern pg_cop_module_t *pg_cop_modules_list;
extern const char *pg_cop_modules_path;

int pg_cop_init_modules_table();
int pg_cop_load_modules(int, char **);

int pg_cop_module_init(pg_cop_module_t *, 
                        int , char **);
int pg_cop_module_start(pg_cop_module_t *);
#endif /* PG_COP_MODULES_H */
