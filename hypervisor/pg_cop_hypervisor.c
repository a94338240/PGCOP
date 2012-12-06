#include "pg_cop_hypervisor.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_debug.h"
#include "pg_cop_hooks.h"
#include "pg_cop_config.h"

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
  void *res;

  while ((opt = getopt_long(argc, argv, "h",
                            long_options, &option_index)) != -1) {
    switch (opt) {
    case 'h':
      fprintf(stderr, "%s", rodata_str_usage);
      exit(0);
      break;
    case 'c':
      pg_cop_lua_config_file = optarg;
      break;
    default:
      fprintf(stderr, "%s", rodata_str_usage);
      exit(EXIT_FAILURE);
    }
  }

  pg_cop_read_config();
  pg_cop_init_modules_table();
  pg_cop_load_modules(argc, argv);

  DEBUG_INFO(rodata_str_service_started);

  PG_COP_EACH_MODULE_BEGIN(pg_cop_modules_list_for_trans);
  pg_cop_hook_trans_init(_module, argc, argv);
  pg_cop_hook_trans_start(_module);
  PG_COP_EACH_MODULE_END;

  PG_COP_EACH_MODULE_BEGIN(pg_cop_modules_list_for_com);
  pthread_join(_module->thread, &res);
  free(res);
  PG_COP_EACH_MODULE_END;

  return 0;
}
