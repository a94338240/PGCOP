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

#include "pg_cop_modules.h"
#include "pg_cop_debug.h"
#include "pg_cop_util.h"
#include "pg_cop_vstack.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

pg_cop_module_t *pg_cop_modules_list = NULL;
char *pg_cop_modules_path = NULL;
char **pg_cop_ignore_modules = NULL;

int pg_cop_init_modules_table()
{
	pg_cop_modules_list =
	    (pg_cop_module_t *) malloc(sizeof(pg_cop_module_t));
	if (pg_cop_modules_list == NULL)
		goto list_alloc;

	INIT_LIST_HEAD(&pg_cop_modules_list->list_head);
	return 0;

list_alloc:
	return -1;
}

int pg_cop_load_modules(int argc, char *argv[])
{
	struct dirent *module_dir_entry;

	char module_path[255] = {};
	pg_cop_module_t *module;
	void *dl_handle;
	void *module_hooks;
	pg_cop_module_info_t *module_info;

	if (!pg_cop_modules_list)
		goto check_modules_list;

	DIR *module_dir = opendir(pg_cop_modules_path);
	if (!module_dir) {
		DEBUG_CRITICAL("Cannot found module directory.");
		goto check_modules_dir;
	}

	while ((module_dir_entry = readdir(module_dir))) {
		if (!module_dir_entry->d_name)
			goto check_d_name_cont;

		char file_ext[20] = {};
		if (pg_cop_get_file_extension(module_dir_entry->d_name,
		                              file_ext, sizeof(file_ext)) != 0)
			goto get_file_extension_cont;

		if (strncmp(file_ext, ".so", 3))
			goto check_if_module_file_cont;

		for (int i = 0; pg_cop_ignore_modules &&
		        pg_cop_ignore_modules[i]; i++) {
			if (strcmp(module_dir_entry->d_name,
			           pg_cop_ignore_modules[i]) == 0) {
				DEBUG_INFO("Module %s skipped.", pg_cop_ignore_modules[i]);
				goto skip;
			}
		}

		strncpy(module_path, pg_cop_modules_path, sizeof(module_path) - 1);
		strncat(module_path, "/", sizeof(module_path) - 1);
		strncat(module_path, module_dir_entry->d_name, sizeof(module_path) - 1);
		dl_handle = dlopen(module_path, RTLD_LAZY);
		if (!dl_handle) {
			DEBUG_ERROR("Module %s cannot be loaded.",
			            module_dir_entry->d_name);
			goto dl_open_cont;
		}

		module_info = dlsym(dl_handle, "pg_cop_module_info");
		if (!module_info) {
			DEBUG_ERROR("No info symbol found in %s.",
			            module_dir_entry->d_name);
			goto dlsym_cont;
		}

		module_hooks = dlsym(dl_handle, "pg_cop_module_hooks");
		if (!module_hooks) {
			DEBUG_ERROR("No hooks symbol found in %s.",
			            module_dir_entry->d_name);
			goto dlsym_hooks_cont;
		}

		module = (pg_cop_module_t *) malloc(sizeof(pg_cop_module_t));
		if (!module)
			goto module_alloc_cont;
		module->dl_handle = dl_handle;
		module->info = module_info;
		module->thread = 0;
		module->hooks = module_hooks;
		list_add_tail(&module->list_head, &pg_cop_modules_list->list_head);

		DEBUG_INFO("Module %s be loaded.", module->info->name);
		continue;

module_alloc_cont:
		module_hooks = NULL;
dlsym_hooks_cont:
		module_info = NULL;
dlsym_cont:
		dlclose(dl_handle);
		dl_handle = NULL;
dl_open_cont:
skip:
check_if_module_file_cont:
get_file_extension_cont:
check_d_name_cont:
		DEBUG_INFO("Error occured what loading module %s", module_dir_entry->d_name);
		continue;
	}

	closedir(module_dir);
	return 0;

check_modules_dir:
check_modules_list:
	return -1;
}

int pg_cop_module_init(pg_cop_module_t *module,
                       int argc, char *argv[])
{
	if (module->hooks == NULL ||
	        module->hooks->init == NULL) {
		goto check_args;
	}
	return module->hooks->init(argc, argv);

check_args:
	return -1;
}

int pg_cop_module_destroy(pg_cop_module_t *module)
{
	if (!module)
		goto check_args;

	DEBUG_INFO("Destroying module %s", module->info->name);

	pg_cop_module_stop(module);
	dlclose(module->dl_handle);
	pthread_attr_destroy(&module->thread_attr);
	return 0;

check_args:
	return -1;
}

int pg_cop_module_stop(pg_cop_module_t *module)
{
	if (!module)
		goto check_args;

	return pthread_kill(module->thread, SIGTERM);

check_args:
	return -1;
}

int pg_cop_module_start(pg_cop_module_t *module)
{
	if (module->hooks == NULL ||
	        module->hooks->start == NULL)
		goto check_args;

	int retval = pthread_attr_init(&module->thread_attr);
	if (retval != 0) {
		DEBUG_ERROR("Cannot create thread attributes.");
		goto init_attr;
	}

	if (pthread_create(&module->thread, &module->thread_attr,
	                   (void * (*)(void *)) module->hooks->start, module)) {
		DEBUG_ERROR("Cannot create thread.");
		goto create_thread;
	}

	return 0;

create_thread:
	pthread_attr_destroy(&module->thread_attr);
init_attr:
check_args:
	return -1;
}

