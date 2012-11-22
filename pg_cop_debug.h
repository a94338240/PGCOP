#ifndef PG_COP_DEBUG_H
#define PG_COP_DEBUG_H
#include <stdio.h>

#define DEBUG_ERROR(a)                          \
  do {                                          \
    fprintf(stderr, "ERROR: %s\n", a);          \
  } while (0)

#define DEBUG_CRITICAL(a)                                             \
  do {                                                                \
    fprintf(stderr, "CRITICAL: %s\n\n", a); \
    exit(EXIT_FAILURE);                                               \
  } while (0)

#define DEBUG_INFO(a)                                     \
  do {                                                    \
    fprintf(stdout, "INFO: %s\n", a);                     \
  } while (0)

#endif /* PG_COP_DEBUG_H */
