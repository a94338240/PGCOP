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

pg_cop_module_t *pg_cop_modules_list = NULL;
static int vstack_id = 0;

const char *pg_cop_modules_path = "/usr/local/pgcop/share/pgcop/modules";

int pg_cop_init_modules_table()
{
  pg_cop_modules_list = 
    (pg_cop_module_t *)malloc(sizeof(pg_cop_module_t)); // INFO No free needed
  if (!pg_cop_modules_list)
    return -1;
  INIT_LIST_HEAD(&pg_cop_modules_list->list_head);
  return 0;
}

int pg_cop_load_modules(int argc, char *argv[])
{
  DIR *module_dir;
  struct dirent *module_dir_entry;
  char file_ext[20] = {};
  char module_path[255] = {};
  pg_cop_module_t *module;
  void *dl_handle;
  void *module_hooks;
  pg_cop_module_info_t *module_info;
  pg_cop_module_t *list;
  pg_cop_vstack_t *vstack;

  if (!pg_cop_modules_list)
    return -1;

  module_dir = opendir(pg_cop_modules_path);

  if (!module_dir)
    DEBUG_CRITICAL("Cannot found module directory.");

  while ((module_dir_entry = readdir(module_dir))) {
    if (!module_dir_entry->d_name)
      continue;
    if (pg_cop_get_file_extension(module_dir_entry->d_name, 
                                  file_ext, sizeof(file_ext)) != 0)
      continue;
    if (strncmp(file_ext, ".so", 3))
      continue;

    strncpy(module_path, pg_cop_modules_path, sizeof(module_path) - 1);
    strncat(module_path, "/", sizeof(module_path) - 1);
    strncat(module_path, module_dir_entry->d_name, sizeof(module_path) - 1);
    dl_handle = dlopen(module_path, RTLD_LAZY);

    if (!dl_handle) {
      DEBUG_ERROR("Module %s cannot be loaded.", 
                  module_dir_entry->d_name);
      continue;
    }

    module_info = dlsym(dl_handle, "pg_cop_module_info");
    if (!module_info) {
      DEBUG_ERROR("No info symbol found in %s.",
                  module_dir_entry->d_name);
      continue;
    }

    module_hooks = dlsym(dl_handle, "pg_cop_module_hooks");
    if (!module_hooks) {
      DEBUG_ERROR("No hooks symbol found in %s.",
                  module_dir_entry->d_name);
      continue;
    }

    module = (pg_cop_module_t *)malloc(sizeof(pg_cop_module_t)); // FIXME Not freed
    module->dl_handle = dl_handle;
    module->info = module_info;
    module->thread = 0;
    module->hooks = module_hooks;
    list_add_tail(&module->list_head, &pg_cop_modules_list->list_head);

    DEBUG_INFO("Module %s be loaded.",
               module_dir_entry->d_name);
  }

  closedir(module_dir);
  return 0;
}

int pg_cop_module_init(pg_cop_module_t *module, 
                       int argc, char *argv[])
{
  if (module->hooks == NULL ||
      module->hooks->init == NULL) {
    return -1;
  }
  return module->hooks->init(argc, argv);
}

int pg_cop_module_start(pg_cop_module_t *module) 
{
  int retval = 0;

  if (module->hooks == NULL ||
      module->hooks->start == NULL)
    return -1;

  retval = pthread_attr_init(&module->thread_attr);
  if (retval != 0) {
    DEBUG_ERROR("Cannot create thread attributes.");
    return -1;
  }
  
  if (pthread_create(&module->thread, &module->thread_attr,
                     (void *(*)(void *))module->hooks->start, module)) {
    DEBUG_ERROR("Cannot create thread.");
    return -1;
  }

  return 0;
}

