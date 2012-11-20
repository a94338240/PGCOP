#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include "pg_cop_hypervisor.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_service.h"
#include "pg_cop_debug.h"
#include "pg_cop_modules.h"

static struct option long_options[] = {
  {"srvtype",    required_argument, 0,  't'},
  {"port",       required_argument, 0,  'p'},
  {0,            0,                 0,   0 }
};

int main(int argc, char *argv[])
{
  int opt = 0;
  int option_index = 0;

  pg_cop_init_modules_table();
  pg_cop_load_modules();

  while ((opt = getopt_long(argc, argv, "tp",
                            long_options, &option_index)) != -1) {
    switch (opt) {
    case 't':
      if (!strcmp(optarg, "socket"))
        pg_cop_service_type = PG_COP_SERVICE_TYPE_SOCKET;
      else if (!strcmp(optarg, "sharedmem"))
        pg_cop_service_type = PG_COP_SERVICE_TYPE_SHAREDMEM;
      else
        DEBUG_CRITICAL(rodata_str_service_type_invalid);
      break;
    case 'p':
      pg_cop_server_port = atoi(optarg);
      if (pg_cop_server_port <= 0 ||
          pg_cop_server_port > 65535) {
        DEBUG_CRITICAL(rodata_str_port_not_in_range);
      }
      break;
    default:
      fprintf(stderr, "%s", rodata_str_usage);
      exit(EXIT_FAILURE);
    }
  }

  pg_cop_service_start();
  DEBUG_INFO(rodata_str_service_started);

  return 0;
}
