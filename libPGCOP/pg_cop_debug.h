#ifndef PG_COP_DEBUG_H
#define PG_COP_DEBUG_H
#include <stdio.h>

#define DEBUG_ERROR(...)              \
  do {                                   \
    fprintf(stderr, "ERROR: ");          \
    fprintf(stderr, __VA_ARGS__);     \
    fprintf(stderr, "\n");               \
  } while (0)

#define DEBUG_CRITICAL(...)            \
  do {                                    \
    fprintf(stderr, "CRITICAL: ");        \
    fprintf(stderr, __VA_ARGS__);      \
    fprintf(stderr, "\n");                \
    exit(EXIT_FAILURE);                   \
  } while (0)

#define DEBUG_INFO(...)                                \
  do {                                                    \
    fprintf(stdout, "INFO: ");                            \
    fprintf(stdout, __VA_ARGS__);                      \
    fprintf(stdout, "\n");                                \
  } while (0)

#define DEBUG_DEBUG(...)                               \
  do {                                                    \
    fprintf(stdout, "DEBUG: ");                           \
    fprintf(stdout, __VA_ARGS__);                      \
    fprintf(stdout, "\n");                                \
  } while (0)

#define MOD_DEBUG_ERROR(...)                                     \
  do {                                                              \
    fprintf(stderr, "ERROR: [%s] ", pg_cop_module_info.name);       \
    fprintf(stderr, __VA_ARGS__);                                \
    fprintf(stderr, "\n");                                          \
  } while (0)

#define MOD_DEBUG_CRITICAL(...)                                    \
  do {                                                              \
    fprintf(stderr, "CRITICAL: [%s] ", pg_cop_module_info.name);    \
    fprintf(stderr, __VA_ARGS__);                                \
    fprintf(stderr, "\n");                                       \
    exit(EXIT_FAILURE);                                          \
  } while (0)

#define MOD_DEBUG_INFO(...)                                      \
  do {                                                              \
    fprintf(stdout, "INFO: [%s] ", pg_cop_module_info.name);        \
    fprintf(stdout, __VA_ARGS__);                                \
    fprintf(stdout, "\n");                                          \
  } while (0)

#define MOD_DEBUG_DEBUG(...)                                     \
  do {                                                              \
    fprintf(stdout, "DEBUG: [%s] ", pg_cop_module_info.name);       \
    fprintf(stdout, __VA_ARGS__);                                \
    fprintf(stdout, "\n");                                          \
  } while (0)

#endif /* PG_COP_DEBUG_H */
