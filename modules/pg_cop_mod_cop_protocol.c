#include "pg_cop_hooks.h"
#include "pg_cop_modules.h"
#include "pg_cop_debug.h"
#include "pg_cop_rodata_strings.h"

#include <stdlib.h>

typedef struct {
  int is_sub;
  int parent_proto_magic;
} cop_proto_private_data_t;

cop_proto_private_data_t private_data = {0, 0};

static int cop_main_proto_process(pg_cop_data_in_t in, 
                                  pg_cop_data_out_t *out,
                                  int sub_lvl);
static int cop_main_proto_sweep(pg_cop_data_out_t out);

const pg_cop_module_info_t pg_cop_module_info = {
  .magic = 0xF7280201, /* FIXME */
  .type = PG_COP_MODULE_TYPE_PROTO,
  .name = "mod_cop_main_protocol",
  .private_data = (void *)&private_data
};

const pg_cop_module_proto_hooks_t pg_cop_module_hooks = {
  .process = cop_main_proto_process,
  .sweep = cop_main_proto_sweep
};

static int cop_main_proto_process(pg_cop_data_in_t in, 
                                  pg_cop_data_out_t *out,
                                  int sub_lvl)
{
  if (sub_lvl != 0)
    return 0;

  

  PG_COP_EACH_MODULE_BEGIN(pg_cop_modules_list_for_proto);
  pg_cop_hook_proto_process(_module, in, out, 1);
  PG_COP_EACH_MODULE_END;

  return 0;
}

static int cop_main_proto_sweep(pg_cop_data_out_t out)
{
  if (out.data)
    free(out.data);

  return 0;
}
