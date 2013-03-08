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

#include "pg_cop_util.h"
#include "pg_cop_debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
