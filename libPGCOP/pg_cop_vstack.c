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
	pg_cop_vstack_t *vstack = malloc(sizeof(pg_cop_vstack_t));
	if (!vstack) {
		DEBUG_ERROR("cannot allocate memory for Vstack");
		goto vstack_alloc;
	}

	vstack->id = id;
	vstack->size = size;
	vstack->top = 0;
	vstack->data_area = NULL;
	return vstack;

vstack_alloc:
	return NULL;
}

void pg_cop_vstack_destroy(pg_cop_vstack_t *vstack)
{
	if (!vstack->data_area)
		goto no_data;

	free(vstack->data_area);

no_data:
	free(vstack);
}

pg_cop_vstack_state_t pg_cop_vstack_push(pg_cop_vstack_t *vstack,
        pg_cop_vstack_type_t type, ...)
{
	void *data;
	int data_size;
	unsigned int tmp_u32;
	int tmp_i32;
	pg_cop_vstack_type_t *type_alias;
	int *size_alias;
	int res = VSTACK_OK;

	va_list va;
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
		data_size = strlen((char *) data) + 1;
		break;
	case VSTACK_TYPE_DATASIZE:
		data = va_arg(va, void *);
		data_size = va_arg(va, int);
		break;
	default:
		res = -VSTACK_NO_TYPE;
		goto check_type;
	}
	va_end(va);

	if (vstack->top + sizeof(int) * 2 + data_size > vstack->size) {
		res = -VSTACK_OVERFLOW;
		goto overflow;
	}

	vstack->top += sizeof(int) * 2 + data_size;
	vstack->data_area = realloc(vstack->data_area, vstack->top);
	if (vstack->top > 0 && !vstack->data_area) {
		res = -VSTACK_ALLOC;
		goto realloc;
	}
	type_alias = (pg_cop_vstack_type_t *)
	             (&vstack->data_area[vstack->top - sizeof(int)]);
	*type_alias = type;
	size_alias = (int *)(&vstack->data_area[vstack->top - sizeof(int) * 2]);
	*size_alias = data_size;
	memcpy(&vstack->data_area[vstack->top - sizeof(int) * 2 - data_size], data, data_size);
	return VSTACK_OK;

realloc:
	pg_cop_vstack_clear(vstack);
overflow:
check_type:
	if (res == -VSTACK_NO_TYPE)
		va_end(va);
	return res;
}

pg_cop_vstack_state_t pg_cop_vstack_pop(pg_cop_vstack_t *vstack,
                                        pg_cop_vstack_type_t type, ...)
{
	void *output;
	int res;

	if (vstack->top <= 0) {
		res = -VSTACK_EMPTY;
		goto check_empty;
	}

	pg_cop_vstack_type_t *type_alias = (pg_cop_vstack_type_t *)
	                                   (&vstack->data_area[vstack->top - sizeof(int)]);
	pg_cop_vstack_type_t data_type = *type_alias;
	int *size_alias = (int *)(&vstack->data_area[vstack->top - sizeof(int) * 2]);
	int data_size = *size_alias;
	void *data_tmp = &vstack->data_area[vstack->top - sizeof(int) * 2 - data_size];

	if (data_type != type) {
		DEBUG_ERROR("Wrong type in stack. required %d, but %d", type, data_type);
		res = -VSTACK_WRONG_TYPE;
		goto check_data_type;
	}

	if ((vstack->top - sizeof(int) * 2 - data_size) < 0) {
		DEBUG_ERROR("Stack broken");
		res = -VSTACK_BROKEN;
		goto check_broken;
	}

	va_list va;
	va_start(va, type);
	switch (type) {
	case VSTACK_TYPE_U8:
		* (va_arg(va, unsigned char *)) = * ((unsigned char*) data_tmp);
		break;
	case VSTACK_TYPE_I8:
		* (va_arg(va, char *)) = * ((char*) data_tmp);
		break;
	case VSTACK_TYPE_U16:
		* (va_arg(va, unsigned short *)) = * ((unsigned short*) data_tmp);
		break;
	case VSTACK_TYPE_I16:
		* (va_arg(va, short *)) = * ((short*) data_tmp);
		break;
	case VSTACK_TYPE_U32:
		* (va_arg(va, unsigned int *)) = * ((unsigned int*) data_tmp);
		break;
	case VSTACK_TYPE_I32:
		* (va_arg(va, int *)) = * ((int*) data_tmp);
		break;
	case VSTACK_TYPE_DATASIZE:
		output = malloc(data_size);
		memcpy(output, data_tmp, data_size);
		* (va_arg(va, void **)) = output;
		* (va_arg(va, int *)) = data_size;
		break;
	case VSTACK_TYPE_STRING:
		output = malloc(data_size);
		memcpy(output, data_tmp, data_size);
		* (va_arg(va, void **)) = output;
		break;
	default:
		res = -VSTACK_NO_TYPE;
		goto check_type;
	}
	va_end(va);

	vstack->top = vstack->top - sizeof(int) * 2 - data_size;
	vstack->data_area = realloc(vstack->data_area, vstack->top);
	if (vstack->top > 0 && !vstack->data_area) {
		res = -VSTACK_ALLOC;
		goto realloc;
	}

	return VSTACK_OK;

realloc:
	pg_cop_vstack_clear(vstack);
check_type:
	if (res == -VSTACK_NO_TYPE)
		va_end(va);
check_broken:
	if (res == -VSTACK_BROKEN)
		pg_cop_vstack_clear(vstack);
check_data_type:
check_empty:
	return res;
}

pg_cop_vstack_state_t pg_cop_vstack_transfer(pg_cop_vstack_t *s_vstack,
        pg_cop_vstack_t *d_vstack)
{
	int idata;
	char *sdata;
	int size;

	pg_cop_vstack_type_t type;
	while (pg_cop_vstack_pick_type(s_vstack, &type) == 0) {
		switch (type) {
		case VSTACK_TYPE_STRING:
			if (pg_cop_vstack_pop(s_vstack, type, &sdata))
				goto pop_err;
			if (pg_cop_vstack_push(d_vstack, type, sdata))
				goto push_err;
			free(sdata);
			break;
		case VSTACK_TYPE_DATASIZE:
			if (pg_cop_vstack_pop(s_vstack, type, &sdata, &size))
				goto pop_err;
			if (pg_cop_vstack_push(d_vstack, type, sdata, size))
				goto push_err;
			free(sdata);
			break;
		default:
			if (pg_cop_vstack_pop(s_vstack, type, &idata))
				goto pop_err;
			if (pg_cop_vstack_push(d_vstack, type, idata))
				goto push_err;
		}
	}
	return VSTACK_OK;

push_err:
	if (type == VSTACK_TYPE_STRING ||
	        type == VSTACK_TYPE_DATASIZE)
		free(sdata);
pop_err:
	return -VSTACK_BROKEN;
}

int pg_cop_vstack_pick_type(pg_cop_vstack_t *vstack, pg_cop_vstack_type_t *type)
{
	pg_cop_vstack_type_t *type_alias;
	int res;

	if (vstack->top <= 0) {
		res = -VSTACK_EMPTY;
		goto check_empty;
	}

	type_alias = (pg_cop_vstack_type_t *)
	             (&vstack->data_area[vstack->top - sizeof(int)]);
	*type = *type_alias;
	return 0;

check_empty:
	return res;
}

int pg_cop_vstack_used_bytes(pg_cop_vstack_t * vstack)
{
	return vstack->top;
}

void *pg_cop_vstack_dump(pg_cop_vstack_t * vstack)
{
	void *dump;

	if (vstack->top <= 0) {
		goto check_top;
	}

	dump = malloc(vstack->top);
	if (!dump) {
		goto dump_alloc;
	}

	memcpy(dump, vstack->data_area, vstack->top);
	return dump;

dump_alloc:
check_top:
	return NULL;
}

int pg_cop_vstack_has_more(pg_cop_vstack_t * vstack)
{
	return vstack->top;
}

int pg_cop_vstack_clear(pg_cop_vstack_t * vstack)
{
	if (vstack->data_area)
		free(vstack->data_area);
	vstack->data_area = NULL;
	return vstack->top = 0;
}

int pg_cop_vstack_import(pg_cop_vstack_t * vstack, void * data, int size)
{
	int res;

	vstack->top = size;
	vstack->data_area = realloc(vstack->data_area, size);
	if (vstack->top > 0 && !vstack->data_area) {
		res = -VSTACK_ALLOC;
		goto realloc_err;
	}
	memcpy(vstack->data_area, data, size);
	return 0;

realloc_err:
	pg_cop_vstack_clear(vstack);
	return res;
}
