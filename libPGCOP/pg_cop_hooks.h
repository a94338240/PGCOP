#ifndef PG_COP_HOOKS_H
#define PG_COP_HOOKS_H

#include "pg_cop_modules.h"

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
  void *data;
  int size;
} pg_cop_data_in_t;

typedef struct {
  void *data;
  int size;
} pg_cop_data_out_t;

typedef enum {
  PG_COP_RPC_TYPE_INT,
  PG_COP_RPC_TYPE_FLOAT,
  PG_COP_RPC_TYPE_DOUBLE,
  PG_COP_RPC_TYPE_CHAR,
  PG_COP_RPC_TYPE_STRING,
  PG_COP_RPC_TYPE_BINARY,
} PG_COP_RPC_TYPE_t;
typedef struct {
  PG_COP_RPC_TYPE_t type;
  void* return_val;
} pg_cop_rpc_state_t;
typedef int (*pg_cop_rpc_func_callback_t)(void *);
typedef struct {
  pg_cop_rpc_state_t* (*call_function_sync)(const char *func, ...);
  pg_cop_rpc_state_t* (*call_function_async)(const char *func, 
                                             pg_cop_rpc_func_callback_t callback, 
                                             ...);
} pg_cop_module_rpc_hooks_t;

typedef struct {
  int (*init)(int argc, char *argv[]);
  int (*process)(pg_cop_data_in_t in, pg_cop_data_out_t *out,
                 int sub_lvl);
  int (*sweep)(pg_cop_data_out_t out);
} pg_cop_module_proto_hooks_t;

typedef struct {
  int (*init)(int argc, char *argv[]);
  int (*start)();
} pg_cop_module_trans_hooks_t;

#define PG_COP_COM_HOOKS(hooks) \
  ((pg_cop_module_com_hooks_t *)(hooks))

#define PG_COP_RPC_HOOKS(hooks) \
  ((pg_cop_module_rpc_hooks_t *)(hooks))

#define PG_COP_TRANS_HOOKS(hooks) \
  ((pg_cop_module_trans_hooks_t *)(hooks))

#define PG_COP_PROTO_HOOKS(hooks) \
  ((pg_cop_module_proto_hooks_t *)(hooks))

#define PG_COP_HOOK_CHECK_FAILURE_COM               \
  (!module || !module->info ||                      \
   module->info->type != PG_COP_MODULE_TYPE_COM)    

#define PG_COP_HOOK_CHECK_FAILURE_TRANS             \
  (!module || !module->info ||                      \
   module->info->type != PG_COP_MODULE_TYPE_TRANSCEIVER)    

#define PG_COP_HOOK_CHECK_FAILURE_PROTO             \
  (!module || !module->info ||                      \
   module->info->type != PG_COP_MODULE_TYPE_PROTO)    

int pg_cop_hook_com_init(pg_cop_module_t *module, int argc, char *argv[]);
int pg_cop_hook_com_bind(pg_cop_module_t *module);
int pg_cop_hook_com_accept(pg_cop_module_t *module);
int pg_cop_hook_com_send(pg_cop_module_t *module, int id, 
                         const void *buf, unsigned int len,
                         unsigned int flags);
int pg_cop_hook_com_recv(pg_cop_module_t *module, int id, 
                         void *buf, unsigned int len,
                         unsigned int flags);

int pg_cop_hook_trans_init(pg_cop_module_t *module, int argc, char *argv[]);
int pg_cop_hook_trans_start(pg_cop_module_t *module);

int pg_cop_hook_proto_init(pg_cop_module_t *module, int argc, char *argv[]);
int pg_cop_hook_proto_process(pg_cop_module_t *module, pg_cop_data_in_t in,
                              pg_cop_data_out_t *out, int sub_lvl);
int pg_cop_hook_proto_sweep(pg_cop_module_t *module, pg_cop_data_out_t out);

#endif /* PG_COP_HOOKS_H */
