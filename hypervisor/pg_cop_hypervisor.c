#include "pg_cop_hypervisor.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_debug.h"
#include "pg_cop_hooks.h"
#include "pg_cop_config.h"
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

  list_for_each_entry(module, &pg_cop_modules_list_for_trans->list_head, list_head) {
    pg_cop_hook_trans_init(module, argc, argv);
    pg_cop_hook_trans_start(module);
  }

  list_for_each_entry(module, &pg_cop_modules_list_for_com->list_head, list_head) {
    res = NULL;
    pthread_join(module->thread, &res);
    if (res)
      free(res);
  }

  list_for_each_entry(module, &pg_cop_modules_list_for_trans->list_head, list_head) {
    res = NULL;
    pthread_join(module->thread, &res);
    if (res)
      free(res);
  }

  return 0;
}
