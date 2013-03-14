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

#include "pg_cop_hypervisor.h"
#include "pg_cop_debug.h"
#include "pg_cop_config.h"
#include "pg_cop_modules.h"
#include "pg_cop_interface.h"
#include "pg_cop_seeds.h"
#include "list.h"
#include "pg_cop_hypervisor_opts_ag.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

char *ignore_modules[] = {
	NULL
};

int main(int argc, char *argv[])
{
	int optct = optionProcess(&hypervisorOptions, argc, argv);
	argc -= optct;
	argv += optct;

	if (HAVE_OPT(CONF)) {
		int filect = STACKCT_OPT(CONF);
		char** conf_opt = (char **) STACKLST_OPT(CONF);
		if (filect > 0) {
			char* config_filename = *conf_opt;
			pg_cop_lua_config_file = config_filename;
		}
	}

	pg_cop_ignore_modules = ignore_modules;

	if (pg_cop_read_config())
		goto read_config;

	if (pg_cop_module_interface_daemon_init())
		goto daemon_init;

	if (pg_cop_module_interface_daemon_start())
		goto daemon_start;

	if (pg_cop_init_modules_table())
		goto modules_init;

	if (pg_cop_load_modules(argc, argv))
		goto load_modules;

	DEBUG_INFO("Hypervisor started.");

	pg_cop_module_t *module, *module_tmp;
	list_for_each_entry_safe(module, module_tmp,
	                         &pg_cop_modules_list->list_head, list_head) {
		if (pg_cop_module_init(module, argc, argv))
			goto module_init;
		if (pg_cop_module_start(module))
			goto module_start;
		continue;

module_start:
		list_del(&module->list_head);
		pg_cop_module_destroy(module);
module_init:
		;
	}

	if (pg_cop_init_seeds_table())
		goto init_seeds;

	if (pg_cop_load_seeds(argc, argv))
		goto load_seeds;

	void *res = NULL;
	list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
		pthread_join(module->thread, &res);
		if (res)
			free(res);
	}

	DEBUG_INFO("Hypervisor stopped.");

	return 0;

load_seeds:
init_seeds:
load_modules:
modules_init:
daemon_start:
daemon_init:
read_config:
	return -1;
}
