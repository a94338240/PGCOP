#include "pg_cop_service.h"
#include "pg_cop_modules.h"
#include "pg_cop_debug.h"
#include "pg_cop_rodata_strings.h"

#include <pthread.h>
#include <stdlib.h>

void *pg_cop_service_routine(void *module)
{
  int fd;

  if (pg_cop_hook_com_init(module, 0, NULL) != 0) /* FIXME args */
    return NULL;
  if (pg_cop_hook_com_bind(module) != 0)
    return NULL;

  for (;;) {
    fd = pg_cop_hook_com_accept(module);
    if (fd < 0)
      DEBUG_ERROR(rodata_str_accept_error);
    /* TODO: Receive and send. */
  }
  return NULL;
}

void pg_cop_service_start()
{
  pg_cop_list_t *pos, *head;
  pg_cop_module_t *module;
  void *res;
  int s;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  int count = 0;

  head = &pg_cop_modules_list_for_com->list_head;
  PG_COP_LIST_FOREACH(pos, head) {
    module = (pg_cop_module_t *)
      PG_COP_LIST_GET(pos, pg_cop_module_t);

    /* FIXME Stack size. */
    s = pthread_attr_init(&module->thread_attr);
    if (s != 0) {
      DEBUG_ERROR(rodata_str_cannot_create_thread);
      continue;
    }

    pthread_create(&module->thread, &module->thread_attr,
                   pg_cop_service_routine, module);

    count++;
  }

  sprintf(debug_info, rodata_str_com_module_enabled, count);
  DEBUG_INFO(debug_info);
  if (count)
    pthread_join(module->thread, &res);
  else
    DEBUG_CRITICAL(rodata_str_no_com_module);
}
