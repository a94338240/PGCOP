#include "pg_cop_hypervisor.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_service.h"
#include "pg_cop_debug.h"
#include "pg_cop_modules.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

static struct option long_options[] = {
  {"help",       no_argument,       0,  'h'},
  {0,            0,                 0,   0 }
};

int main(int argc, char *argv[])
{
  int opt = 0;
  int option_index = 0;

  while ((opt = getopt_long(argc, argv, "h",
                            long_options, &option_index)) != -1) {
    switch (opt) {
    case 'h':
      fprintf(stderr, "%s", rodata_str_usage);
      exit(0);
      break;
    default:
      fprintf(stderr, "%s", rodata_str_usage);
      exit(EXIT_FAILURE);
    }
  }

  pg_cop_init_modules_table();
  pg_cop_load_modules();

  DEBUG_INFO(rodata_str_service_started);
  pg_cop_service_start();

  return 0;
}
