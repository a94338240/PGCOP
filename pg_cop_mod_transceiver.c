#include "pg_cop_hooks.h"
#include "pg_cop_debug.h"
#include "pg_cop_rodata_strings.h"

#include <pthread.h>
#include <stdlib.h>

static int transceiver_start();

pg_cop_module_info_t pg_cop_module_info = {
  .magic = 0xF7280102,/* FIXME */
  .type = PG_COP_MODULE_TYPE_TRANSCEIVER,
  .name = "mod_transceiver"
};

pg_cop_module_trans_hooks_t pg_cop_module_hooks = {
  .start = transceiver_start
};

static void *transceiver_routine(void *module)
{
  int fd;

  if (pg_cop_hook_com_init(module, 0, NULL) != 0) { /* FIXME args */
    return NULL;
  }
  if (pg_cop_hook_com_bind(module) != 0)
    return NULL;

  for (;;) {
    fd = pg_cop_hook_com_accept(module);
    if (fd < 0)
      MOD_DEBUG_ERROR(rodata_str_accept_error);
    /* TODO: Receive and send. */
    
    pg_cop_hook_com_send(module, fd, rodata_str_service_welcome_message, 
                         rodata_size_str_service_welcome_message, 0);
  }
  return NULL;
}

static int transceiver_start()
{
  void *res;
  int s;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  int count = 0;

  PG_COP_EACH_MODULE_BEGIN(pg_cop_modules_list_for_com);
  /* FIXME Stack size. */
  s = pthread_attr_init(&_module->thread_attr);
  if (s != 0) {
    MOD_DEBUG_ERROR(rodata_str_cannot_create_thread);
    continue;
  }
  
  pthread_create(&_module->thread, &_module->thread_attr,
                 transceiver_routine, _module);
  
  count++;
  PG_COP_EACH_MODULE_END;

  sprintf(debug_info, rodata_str_com_module_enabled, count);
  MOD_DEBUG_INFO(debug_info);
  if (count) {
    PG_COP_EACH_MODULE_BEGIN(pg_cop_modules_list_for_com);
    pthread_join(_module->thread, &res);
    PG_COP_EACH_MODULE_END;
  }
  else
    MOD_DEBUG_CRITICAL(rodata_str_no_com_module);

  return 0;
}
