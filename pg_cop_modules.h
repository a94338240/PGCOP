#ifndef PG_COP_MODULES_H
#define PG_COP_MODULES_H

#include "pg_cop_util.h"

#define MAXLEN_MODULE_FILE_EXT (20)
#define MAXLEN_LOAD_MODULE_DEBUG_INFO (200)
#define MAXLEN_MODULE_PATH (200)
#define DIRE_TYPE_REGULAR_FILE (8)

typedef enum {
  PG_COP_MODULE_TYPE_NONE,
  PG_COP_MODULE_TYPE_COM
} PG_COP_MODULE_TYPE_t;

typedef void (*module_register_func_t)(void);

typedef struct {
  PG_COP_MODULE_TYPE_t type;
  const char *name;
  void *dl_handle;
  pg_cop_list_t list_head;
} pg_cop_module_t;

void pg_cop_load_modules();

#endif /* PG_COP_MODULES_H */
