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

