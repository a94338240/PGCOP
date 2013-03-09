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

#include "pg_cop_vstack.h"
#include "pg_cop_debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

pg_cop_vstack_t *pg_cop_vstack_new(int id, int size)
{
  pg_cop_vstack_t *vstack = NULL;
  vstack = malloc(sizeof(pg_cop_vstack_t) + size);
  if (vstack == NULL) {
    DEBUG_ERROR("cannot allocate memory for Vstack");
    return NULL;
  }
  vstack->id = id;
  vstack->size = size;
  vstack->top = 0;
  return vstack;
}

void pg_cop_vstack_destroy(pg_cop_vstack_t *vstack)
{
  if (vstack)
    free(vstack);
}

pg_cop_vstack_state_t pg_cop_vstack_push(pg_cop_vstack_t *vstack, 
                                         pg_cop_vstack_type_t type, ...)
{
  va_list va;
  int data_size;
  unsigned int tmp_u32;
  int tmp_i32;
  pg_cop_vstack_type_t *type_alias;
  int *size_alias;

  void *data;
  
  va_start(va, type);
  switch (type) {
  case VSTACK_TYPE_U8:
  case VSTACK_TYPE_I8:
  case VSTACK_TYPE_U16:
  case VSTACK_TYPE_I16:
  case VSTACK_TYPE_I32:
    tmp_i32 = va_arg(va, int);
    data = &tmp_i32;
    data_size = 4;
    break;
  case VSTACK_TYPE_U32:
    tmp_u32 = va_arg(va, unsigned int);
    data = &tmp_u32;
    data_size = 4;
    break;
  case VSTACK_TYPE_STRING:
    data = va_arg(va, void *);
    data_size = strlen((char *)data) + 1;
    break;
  case VSTACK_TYPE_DATASIZE:
    data = va_arg(va, void *);
    data_size = va_arg(va, int);
    break;
  default:
      return -VSTACK_NO_TYPE;
  }
  va_end(va);
  
  if (vstack->top + sizeof(int) * 2 + data_size > vstack->size)
    return -VSTACK_OVERFLOW;

  vstack->top += sizeof(int) * 2 + data_size;
  type_alias = (pg_cop_vstack_type_t *)
    (&vstack->data_area[vstack->top - sizeof(int)]);
  *type_alias = type;
  size_alias = (int *)(&vstack->data_area[vstack->top - sizeof(int) * 2]);
  *size_alias = data_size;
  memcpy(&vstack->data_area[vstack->top - sizeof(int) * 2 - data_size], data, data_size);
  return VSTACK_OK;
}

pg_cop_vstack_state_t pg_cop_vstack_pop(pg_cop_vstack_t *vstack, 
                                        pg_cop_vstack_type_t type, ...)
{
  int data_size;
  pg_cop_vstack_type_t data_type;
  void *data_tmp;
  void *output;
  va_list va;
  pg_cop_vstack_type_t *type_alias;
  int *size_alias;

  if (vstack->top <= 0)
    return -VSTACK_EMPTY;

  type_alias = (pg_cop_vstack_type_t *)
    (&vstack->data_area[vstack->top - sizeof(int)]);
  data_type = *type_alias;
  size_alias = (int *)(&vstack->data_area[vstack->top - sizeof(int) * 2]);
  data_size = *size_alias;
  data_tmp = &vstack->data_area[vstack->top - sizeof(int) * 2 - data_size];

  if (data_type != type) {
    DEBUG_ERROR("Wrong type in stack. required %d, but %d", type, data_type);
    return -VSTACK_WRONG_TYPE;
  }

  if ((vstack->top - sizeof(int) * 2 - data_size) < 0) {
    DEBUG_ERROR("Stack broken");
    return -VSTACK_BROKEN;
  }

  vstack->top = vstack->top - sizeof(int) * 2 - data_size;

  va_start(va, type);
  switch (type) {
  case VSTACK_TYPE_U8:
    *(va_arg(va, unsigned char *)) = *((unsigned char*)data_tmp);
    break;
  case VSTACK_TYPE_I8:
    *(va_arg(va, char *)) = *((char*)data_tmp);
    break;
  case VSTACK_TYPE_U16:
    *(va_arg(va, unsigned short *)) = *((unsigned short*)data_tmp);
    break;
  case VSTACK_TYPE_I16:
    *(va_arg(va, short *)) = *((short*)data_tmp);
    break;
  case VSTACK_TYPE_U32:
    *(va_arg(va, unsigned int *)) = *((unsigned int*)data_tmp);
    break;
  case VSTACK_TYPE_I32:
    *(va_arg(va, int *)) = *((int*)data_tmp);
    break;
  case VSTACK_TYPE_DATASIZE:
    output = malloc(data_size);
    memcpy(output, data_tmp, data_size);
    *(va_arg(va, void **)) = output;
    *(va_arg(va, int *)) = data_size;
    break;
  case VSTACK_TYPE_STRING:
    output = malloc(data_size);
    memcpy(output, data_tmp, data_size);
    *(va_arg(va, void **)) = output;
    break;
  default:
      return -VSTACK_NO_TYPE;
  }
  va_end(va);

  return VSTACK_OK;
}

pg_cop_vstack_state_t pg_cop_vstack_transfer(pg_cop_vstack_t *s_vstack, 
                                             pg_cop_vstack_t *d_vstack)
{
  pg_cop_vstack_type_t type;
  int idata;
  char *sdata;
  int size;
  while (pg_cop_vstack_pick_type(s_vstack, &type) == 0) {
    switch (type) {
    case VSTACK_TYPE_STRING:
      pg_cop_vstack_pop(s_vstack, type, &sdata);
      pg_cop_vstack_push(d_vstack, type, sdata);
      free(sdata);
      break;
    case VSTACK_TYPE_DATASIZE:
      pg_cop_vstack_pop(s_vstack, type, &sdata, &size);
      pg_cop_vstack_push(d_vstack, type, sdata, size);
      free(sdata);
      break;
    default:
      pg_cop_vstack_pop(s_vstack, type, &idata);
      pg_cop_vstack_push(d_vstack, type, idata);
    }
  }
  return VSTACK_OK;
}

int pg_cop_vstack_pick_type(pg_cop_vstack_t *vstack, pg_cop_vstack_type_t *type)
{
  pg_cop_vstack_type_t *type_alias;

  if (vstack->top <= 0)
    return -VSTACK_EMPTY;
  
  type_alias = (pg_cop_vstack_type_t *)
    (&vstack->data_area[vstack->top - sizeof(int)]);
  *type = *type_alias;
  return 0;
}

int pg_cop_vstack_used_bytes(pg_cop_vstack_t *vstack)
{
  return vstack->top;
}

void *pg_cop_vstack_dump(pg_cop_vstack_t *vstack)
{
  void *dump;
  if (vstack->top == 0)
    return NULL;
  dump = malloc(vstack->top);
  memcpy(dump, vstack->data_area, vstack->top);
  return dump;
}

int pg_cop_vstack_has_more(pg_cop_vstack_t *vstack)
{
  return vstack->top;
}

int pg_cop_vstack_clear(pg_cop_vstack_t *vstack)
{
  return vstack->top = 0;
}

int pg_cop_vstack_import(pg_cop_vstack_t *vstack, void *data, int size)
{
  vstack->top = size;
  memcpy(vstack->data_area, data, size);
  return 0;
}
