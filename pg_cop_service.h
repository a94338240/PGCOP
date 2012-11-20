#ifndef PG_COP_SERVICE_H
#define PG_COP_SERVICE_H

#define PG_DEFAULT_COP_SERVER_PORT (12728)
extern int pg_cop_server_port;

typedef enum {
  PG_DEFAULT_COP_SERVICE_TYPE,
  PG_COP_SERVICE_TYPE_SOCKET,
  PG_COP_SERVICE_TYPE_SHAREDMEM,
} PG_COP_SERVICE_TYPE_t;
extern PG_COP_SERVICE_TYPE_t pg_cop_service_type;

void pg_cop_service_start();

#endif /* PG_COP_SERVICE_H */
