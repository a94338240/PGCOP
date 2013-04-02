/*
    PGCOP - PocoGraph Component Oriented Platform.
    Copyright (C) 2013  David Wu <david@pocograph.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PG_COP_DEBUG_H
#define PG_COP_DEBUG_H
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_ERROR(...)              \
  do {                                   \
    fprintf(stderr, "ERROR: ");          \
    fprintf(stderr, __VA_ARGS__);     \
    fprintf(stderr, "\n");               \
    fflush(stderr);                      \
  } while (0)

#define DEBUG_CRITICAL(...)            \
  do {                                    \
    fprintf(stderr, "CRITICAL: ");        \
    fprintf(stderr, __VA_ARGS__);      \
    fprintf(stderr, "\n");                \
    fflush(stderr);                       \
    exit(EXIT_FAILURE);                   \
  } while (0)

#define DEBUG_INFO(...)                                \
  do {                                                    \
    fprintf(stdout, "INFO: ");                            \
    fprintf(stdout, __VA_ARGS__);                      \
    fprintf(stdout, "\n");                                \
    fflush(stdout);                                       \
  } while (0)

#define DEBUG_DEBUG(...)                               \
  do {                                                    \
    fprintf(stdout, "DEBUG: ");                           \
    fprintf(stdout, __VA_ARGS__);                      \
    fprintf(stdout, "\n");                                \
    fflush(stdout);                                       \
  } while (0)

#define MOD_DEBUG_ERROR(...)                                     \
  do {                                                              \
    fprintf(stderr, "ERROR: [%s] ", pg_cop_module_info.name);       \
    fprintf(stderr, __VA_ARGS__);                                \
    fprintf(stderr, "\n");                                          \
    fflush(stderr);                                                 \
  } while (0)

#define MOD_DEBUG_CRITICAL(...)                                    \
  do {                                                              \
    fprintf(stderr, "CRITICAL: [%s] ", pg_cop_module_info.name);    \
    fprintf(stderr, __VA_ARGS__);                                \
    fprintf(stderr, "\n");                                       \
    fflush(stderr);                                              \
    exit(EXIT_FAILURE);                                          \
  } while (0)

#define MOD_DEBUG_INFO(...)                                      \
  do {                                                              \
    fprintf(stdout, "INFO: [%s] ", pg_cop_module_info.name);        \
    fprintf(stdout, __VA_ARGS__);                                \
    fprintf(stdout, "\n");                                          \
    fflush(stdout);                                                 \
  } while (0)

#define MOD_DEBUG_DEBUG(...)                                     \
  do {                                                              \
    fprintf(stdout, "DEBUG: [%s] ", pg_cop_module_info.name);       \
    fprintf(stdout, __VA_ARGS__);                                \
    fprintf(stdout, "\n");                                          \
    fflush(stdout);                                                 \
  } while (0)

#define DEBUG_LOG_BY_LEVEL(l, ...) \
  do { \
    extern int g_pg_cop_debug_level; \
    if (g_pg_cop_debug_level >= l) { \
      printf(__VA_ARGS__); \
      fflush(stdout); \
    } \
  } while(0)

#endif /* PG_COP_DEBUG_H */
