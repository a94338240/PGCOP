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

#ifndef PG_COP_SEEDS_H
#define PG_COP_SEEDS_H

#include "list.h"
#include "pg_cop_interface.h"

typedef struct {
	pg_cop_module_interface_t *intf;
	char *host;
	int port;
	struct list_head list_head;
} pg_cop_tracker_info_t;

typedef struct {
	pg_cop_module_interface_t *intf;
	pg_cop_module_interface_t *ann_intf;
	char *infohash;
	char *mod_name;
	pg_cop_tracker_info_t *tracker_info_list;
	struct list_head list_head;
} pg_cop_seed_t;

extern pg_cop_seed_t *pg_cop_seeds_list;
extern char *pg_cop_seeds_path;

int pg_cop_init_seeds_table();
int pg_cop_load_seeds(int, char **);
pg_cop_seed_t *pg_cop_seed_new(char *,
                               char *,
                               pg_cop_tracker_info_t *);
int pg_cop_seed_destroy(pg_cop_seed_t *);
int pg_cop_seed_announce(pg_cop_seed_t *);
pg_cop_tracker_info_t *pg_cop_tracker_info_new(char *,
        int);
int pg_cop_tracker_info_destroy(pg_cop_tracker_info_t *);
int pg_cop_seed_connect_tracker(pg_cop_seed_t *);
int pg_cop_tracker_info_list_destroy(pg_cop_tracker_info_t *);
int pg_cop_seed_announce(pg_cop_seed_t *seed);
int pg_cop_seed_get_announced_peers(pg_cop_seed_t *seed);
int pg_cop_seed_disconnect_tracker(pg_cop_seed_t *seed);
int pg_cop_seed_revoke(pg_cop_seed_t *seed);

#endif /* PG_COP_SEEDS_H */
