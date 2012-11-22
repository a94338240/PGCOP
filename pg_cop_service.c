#include <pthread.h>
#include <stdlib.h>
#include "pg_cop_service.h"
#include "pg_cop_modules.h"
#include "pg_cop_debug.h"
#include "pg_cop_rodata_strings.h"

void *pg_cop_service_routine(void *module)
{
  int fd;

  pg_cop_hook_com_init(module, 0, NULL);
  pg_cop_hook_com_bind(module);

  for (;;) {
    fd = pg_cop_hook_com_accept(module);
    /* TODO: Receive and send. */
  }
  return NULL;
}

void pg_cop_service_start()
{
  pg_cop_list_t *pos;
  pg_cop_module_t *module;
  int s;

  PG_COP_LIST_FOREACH(pos, 
                      &pg_cop_modules_list_for_com->list_head) {
    /* TODO create thread. */
    module = (pg_cop_module_t *)
      PG_COP_LIST_GET(pos, pg_cop_module_t);

    module->thread_attr = 
      (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
    /* FIXME Stack size. */
    if (!module->thread_attr) {
      DEBUG_ERROR(rodata_str_pthread_mem_failed);
      continue;
    }

    s = pthread_attr_init(module->thread_attr);
    if (s != 0) {
      DEBUG_ERROR(rodata_str_cannot_create_thread);
      free(module->thread_attr);
      module->thread_attr = NULL;
      continue;
    }

    pthread_create(module->thread, module->thread_attr,
                   pg_cop_service_routine, module);
  }
  /* TODO Start service. */
}
