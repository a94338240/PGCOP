#ifndef PG_COP_MODULES_H
#define PG_COP_MODULES_H

#include <pthread.h>
#include "pg_cop_util.h"

#define MAXLEN_MODULE_FILE_EXT (20)
#define MAXLEN_LOAD_MODULE_DEBUG_INFO (200)
#define MAXLEN_MODULE_PATH (200)
#define DIRE_TYPE_REGULAR_FILE (8)

typedef enum {
  PG_COP_MODULE_TYPE_NONE,
  PG_COP_MODULE_TYPE_COM
} PG_COP_MODULE_TYPE_t;

typedef enum {
  PG_COP_RPC_TYPE_INT,
  PG_COP_RPC_TYPE_FLOAT,
  PG_COP_RPC_TYPE_DOUBLE,
  PG_COP_RPC_TYPE_CHAR,
  PG_COP_RPC_TYPE_STRING,
  PG_COP_RPC_TYPE_BINARY,
} PG_COP_RPC_TYPE_t;

typedef struct _pg_cop_module_t pg_cop_module_t;

typedef struct {
  const unsigned int magic;
  const PG_COP_MODULE_TYPE_t type;
  const char *name;
  char *private_data;
} pg_cop_module_info_t;

typedef struct {
  PG_COP_RPC_TYPE_t type;
  void* return_val;
} pg_cop_rpc_state_t;

typedef int (*pg_cop_rpc_func_callback_t)(void *);

typedef struct {
  int (*init)(int argc, char *argv[]);
  int (*bind)();
  int (*accept)();
  int (*send)(int id, const void *buf, unsigned int len,
              unsigned int flags);
  int (*recv)(int id, void *buf, unsigned int len,
              unsigned int flags);
} pg_cop_module_com_hooks_t;

typedef struct {
  pg_cop_rpc_state_t* (*call_function_sync)(const char *func, ...);
  pg_cop_rpc_state_t* (*call_function_async)(const char *func, 
                                             pg_cop_rpc_func_callback_t callback, 
                                             ...);
} pg_cop_module_rpc_hooks_t;

typedef union {
  pg_cop_module_com_hooks_t *com;
  pg_cop_module_rpc_hooks_t *rpc;
} pg_cop_module_hooks_t;

int pg_cop_hook_com_init(pg_cop_module_t *module, int argc, char *argv[]);
int pg_cop_hook_com_bind(pg_cop_module_t *module);
int pg_cop_hook_com_accept(pg_cop_module_t *module);
int pg_cop_hook_com_send(pg_cop_module_t *module, int id, 
                         const void *buf, unsigned int len,
                         unsigned int flags);
int pg_cop_hook_com_recv(pg_cop_module_t *module, int id, 
                         void *buf, unsigned int len,
                         unsigned int flags);

typedef struct _pg_cop_module_t {
  void *dl_handle;
  pthread_t thread;
  pthread_attr_t thread_attr;
  pg_cop_module_info_t *info;
  pg_cop_module_hooks_t hooks;
  pg_cop_list_t list_head;
} pg_cop_module_t;

extern pg_cop_module_t *pg_cop_modules_list_for_com;

#define PG_COP_HOOK_CHECK_FAILURE                   \
  (!module || !module->info ||                      \
   module->info->type != PG_COP_MODULE_TYPE_COM)          

void pg_cop_init_modules_table();
void pg_cop_load_modules();
  
#endif /* PG_COP_MODULES_H */
