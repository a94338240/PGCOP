#ifndef PG_COP_UTIL_H
#define PG_COP_UTIL_H

typedef struct _pg_cop_list_t {
  struct _pg_cop_list_t *next;
  struct _pg_cop_list_t *prev;
} pg_cop_list_t;

int pg_cop_get_file_extension(const char *filename, char *ext, int length);
void _pg_cop_list_head(pg_cop_list_t *list);
void _pg_cop_list_add_tail(pg_cop_list_t *new, pg_cop_list_t *head);

#define PG_COP_LIST_HEAD(var)                   \
  _pg_cop_list_head(&var->list_head)

#define PG_COP_LIST_ADD_TAIL(var, new)          \
  _pg_cop_list_add_tail(&new->list_head, &var->list_head)

#define PG_COP_LIST_GET(list, type)              \
  ((void *)list - (void*)(&((type *)0)->list_head))
#endif /* PG_COP_UTIL_H */
