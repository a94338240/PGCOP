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
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

static struct option long_options[] = {
  {"help",       no_argument,       0,  'h'},
  {"conf",       required_argument, 0,  'c'},
  {0,            0,                 0,   0 }
};

int main(int argc, char *argv[])
{
  int opt = 0;
  int option_index = 0;
  void *res = NULL;
  pg_cop_module_t *module = NULL;

  while ((opt = getopt_long(argc, argv, "h",
                            long_options, &option_index)) != -1) {
    switch (opt) {
    case 'c':
      pg_cop_lua_config_file = optarg;
      break;
    default:
      fprintf(stderr, "Hypervisor\n\t--conf Config file.");
      exit(EXIT_FAILURE);
    }
  }

  pg_cop_read_config(); 
  pg_cop_module_interface_daemon_init();
  pg_cop_module_interface_daemon_start();
  pg_cop_init_modules_table();
  pg_cop_load_modules(argc, argv);

  DEBUG_INFO("Hypervisor started.");

  list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
    pg_cop_module_init(module, argc, argv);
    pg_cop_module_start(module);
  }

  list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
    pthread_join(module->thread, &res);
    if (res)
      free(res);
  }

  DEBUG_INFO("Hypervisor stopped.");
  
  return 0;
}
