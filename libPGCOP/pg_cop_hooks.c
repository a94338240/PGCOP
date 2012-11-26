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
