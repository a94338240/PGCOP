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

#ifndef PG_COP_INTERFACE_H
#define PG_COP_INTERFACE_H

#include "pg_cop_modules.h"
#include "pg_cop_vstack.h"

#define MAGIC_START_INVOKE (0xE7381721)
#define MAGIC_START_RETURN (0xE7383322)

typedef enum {
	MODULE_INTERFACE_TYPE_THREAD,
	MODULE_INTERFACE_TYPE_SOCKET_TCP,
} pg_cop_module_interface_type_t;

typedef struct _pg_cop_module_interface_t {
	pg_cop_module_interface_type_t type;
	pg_cop_vstack_t *vstack;
	int connection_id;
	char *host;
	int port;
	char *mod_name;
	sem_t recv_sem;
	struct _pg_cop_module_interface_t *peer;
} pg_cop_module_interface_t;

typedef struct {
	pg_cop_module_interface_t *intf;
	struct list_head list_head;
} pg_cop_module_interface_announcement_t;

pg_cop_module_interface_t *pg_cop_module_interface_announce(const char *,
        pg_cop_module_interface_type_t, ...);
pg_cop_module_interface_t *pg_cop_module_interface_connect(const char *);
int pg_cop_module_interface_disconnect(pg_cop_module_interface_t *);
int pg_cop_module_interface_invoke(pg_cop_module_interface_t *, const char *,
                                   int, ...);
int pg_cop_module_interface_return(pg_cop_module_interface_t *,
                                   int, ...);
int pg_cop_module_interface_wait(pg_cop_module_interface_t *, char **);
#define pg_cop_module_interface_pop(intf, type, ...)  \
  pg_cop_vstack_pop(intf->vstack, type, __VA_ARGS__)
#define pg_cop_module_interface_push(intf, type, ...) \
  pg_cop_vstack_push(intf->vstack, type, __VA_ARGS__)
#define pg_cop_module_interface_has_more(intf) \
  pg_cop_vstack_has_more(intf->vstack)
#define pg_cop_module_interface_clear(intf) \
  pg_cop_vstack_clear(intf->vstack)
int pg_cop_module_interface_daemon_init();
int pg_cop_module_interface_daemon_start();

pg_cop_module_interface_t *pg_cop_module_interface_new(const char *,
        pg_cop_module_interface_type_t, ...);
int pg_cop_module_interface_destroy(pg_cop_module_interface_t *);
int pg_cop_module_interface_revoke(pg_cop_module_interface_t *);

#endif /* PG_COP_INTERFACE_H */
