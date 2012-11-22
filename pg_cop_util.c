#include "pg_cop_util.h"
#include <string.h>

int pg_cop_get_file_extension(const char *filename, char *ext, int length)
{
  const char *p = filename;
  const char *pe = p + strlen(filename) - 1;

  while (p != pe && *(--pe) != '.');

  if (p == pe)
    return -1;

  strncpy(ext, pe, length);
  
  return 0;
}

void _pg_cop_list_head(pg_cop_list_t *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __pg_cop_list_add(pg_cop_list_t *new,
                                    pg_cop_list_t *prev,
                                    pg_cop_list_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void _pg_cop_list_add_tail(pg_cop_list_t *new, pg_cop_list_t *head)
{
  __pg_cop_list_add(new, head->prev, head);
}
