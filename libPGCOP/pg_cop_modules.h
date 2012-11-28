#ifndef PG_COP_MODULES_H
#define PG_COP_MODULES_H

#include <pthread.h>
#include "pg_cop_util.h"

#define MAXLEN_MODULE_FILE_EXT (32)
#define MAXLEN_LOAD_MODULE_DEBUG_INFO (255)
#define MAXLEN_MODULE_PATH (255)
#define DIRE_TYPE_REGULAR_FILE (8)

typedef enum {
  PG_COP_MODULE_TYPE_NONE,
  PG_COP_MODULE_TYPE_COM,
  PG_COP_MODULE_TYPE_TRANSCEIVER,
  PG_COP_MODULE_TYPE_PROTO,
} PG_COP_MODULE_TYPE_t;

typedef struct _pg_cop_module_t pg_cop_module_t;

typedef struct {
  const unsigned int magic;
  const PG_COP_MODULE_TYPE_t type;
  const char *name;
  char *private_data;
} pg_cop_module_info_t;

typedef struct _pg_cop_module_t {
  void *dl_handle;
  pthread_t thread;
  pthread_attr_t thread_attr;
  pg_cop_module_info_t *info;
  void *hooks;
  pg_cop_list_t list_head;
} pg_cop_module_t;

extern pg_cop_module_t *pg_cop_modules_list_for_com;
extern pg_cop_module_t *pg_cop_modules_list_for_trans;
extern pg_cop_module_t *pg_cop_modules_list_for_proto;
extern const char *pg_cop_modules_path;

#define PG_COP_EACH_MODULE_BEGIN(module_head) do {             \
  pg_cop_list_t *_tmp_pos, *_tmp_head;                         \
  pg_cop_module_t *_module;                                    \
  _tmp_head = &module_head->list_head;                         \
  PG_COP_LIST_FOREACH_BEGIN(_tmp_pos, _tmp_head);              \
  _module = (pg_cop_module_t *)                                \
    PG_COP_LIST_GET(_tmp_pos, pg_cop_module_t);

#define PG_COP_EACH_MODULE_END \
  PG_COP_LIST_FOREACH_END;     \
  } while (0)

void pg_cop_init_modules_table();
void pg_cop_load_modules(int argc, char *argv[]);
  
#endif /* PG_COP_MODULES_H */
