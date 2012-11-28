#include "pg_cop_hooks.h"

int pg_cop_hook_com_init(pg_cop_module_t *module, int argc, char *argv[])
{
  if (PG_COP_HOOK_CHECK_FAILURE_COM || 
      !PG_COP_COM_HOOKS(module->hooks)->init)
    return -1;
  return PG_COP_COM_HOOKS(module->hooks)->init(argc, argv);
}

int pg_cop_hook_com_bind(pg_cop_module_t *module)
{
  if (PG_COP_HOOK_CHECK_FAILURE_COM || 
      !PG_COP_COM_HOOKS(module->hooks)->bind)
    return -1;
  return PG_COP_COM_HOOKS(module->hooks)->bind();
}

int pg_cop_hook_com_accept(pg_cop_module_t *module)
{
  if (PG_COP_HOOK_CHECK_FAILURE_COM || 
      !PG_COP_COM_HOOKS(module->hooks)->accept)
    return -1;
  return PG_COP_COM_HOOKS(module->hooks)->accept();
}

int pg_cop_hook_com_send(pg_cop_module_t *module, int id, 
                         const void *buf, unsigned int len,
                         unsigned int flags)
{
  if (PG_COP_HOOK_CHECK_FAILURE_COM || 
      !PG_COP_COM_HOOKS(module->hooks)->send)
    return -1;
  return PG_COP_COM_HOOKS(module->hooks)->send(id, buf, len, flags);
}

int pg_cop_hook_com_recv(pg_cop_module_t *module, int id, 
                         void *buf, unsigned int len,
                         unsigned int flags)
{
  if (PG_COP_HOOK_CHECK_FAILURE_COM || 
      !PG_COP_COM_HOOKS(module->hooks)->recv)
    return -1;
  return PG_COP_COM_HOOKS(module->hooks)->recv(id, buf, len, flags);
}

int pg_cop_hook_trans_init(pg_cop_module_t *module, int argc, char *argv[])
{
  if (PG_COP_HOOK_CHECK_FAILURE_TRANS || 
      !PG_COP_TRANS_HOOKS(module->hooks)->init)
    return -1;
  return PG_COP_TRANS_HOOKS(module->hooks)->init(argc, argv);
}

int pg_cop_hook_trans_start(pg_cop_module_t *module)
{
  if (PG_COP_HOOK_CHECK_FAILURE_TRANS || 
      !PG_COP_TRANS_HOOKS(module->hooks)->start)
    return -1;
  return PG_COP_TRANS_HOOKS(module->hooks)->start();
}

int pg_cop_hook_proto_init(pg_cop_module_t *module, int argc, char *argv[])
{
  if (PG_COP_HOOK_CHECK_FAILURE_PROTO || 
      !PG_COP_PROTO_HOOKS(module->hooks)->init)
    return -1;
  return PG_COP_PROTO_HOOKS(module->hooks)->init(argc, argv);
}

int pg_cop_hook_proto_process(pg_cop_module_t *module, pg_cop_data_in_t in, 
                              pg_cop_data_out_t *out, int sub_lvl)
{
  if (PG_COP_HOOK_CHECK_FAILURE_PROTO || 
      !PG_COP_PROTO_HOOKS(module->hooks)->process)
    return -1;
  return PG_COP_PROTO_HOOKS(module->hooks)->process(in, out, sub_lvl);
}

int pg_cop_hook_proto_sweep(pg_cop_module_t *module, pg_cop_data_out_t out)
{
  if (PG_COP_HOOK_CHECK_FAILURE_PROTO || 
      !PG_COP_PROTO_HOOKS(module->hooks)->sweep)
    return -1;
  return PG_COP_PROTO_HOOKS(module->hooks)->sweep(out);
}
