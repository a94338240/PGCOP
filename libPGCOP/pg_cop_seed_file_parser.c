/*
    PGCOP - PocoGraph Component Oriented Platform.
    Copyright (C) 2013  David Wu <david@pocograph.com>
                        Steve Ma <steve@pocograph.com>

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

#include "pg_cop_seed_file_parser.h"
#include "pg_cop_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>

struct _inner_build_info {
	char*                 seed_buf;
	unsigned int          need_alloc_size;
	char*                 module_name;
	int                    tracker_cnt;
	int                    func_cnt;
	struct pg_cop_seed_file_func_info_list*        func_list;
	struct pg_cop_seed_file_tracker_info_list*     tracker_list;

};

/*
  seed file format

                     ┌─────────────────────────────────┐
                  /  │seed_file_size                   │
                  │  │check_sum                        │
                  │  │version                          │
           header─┤  │module_name_leghth               │
                  │  │tracker_cnt                      │
                  │  │tracker_list_header_offset       │  ──────────────────────────────────────┐
                  │  │function_cnt                     │                                        │
                  \  │func_list_header_offset          │  ──────────────────────────────────────┼───────────┐
                     ├─────────────────────────────────┤                                        │           │
                     │info hash[32]                    │                                        │           │
                     │'\0'                             │                                        │           │
                     │module_name_character            │  ─> amount as module_name_leghth       │           │
                     │'\0'                             │                                        │           │
                     ├─────────────────────────────────┤                                        │           │
                  /  │┌─────────────────┐              │                                        │           │
                  │  ││type             │              │ <──────────────────────────────────────┘           │
                  │  │├─────────────────┤              │                                                    │
          tracker─┤  ││address          │              │  ─> amount as tracker_cnt                          │
                  │  │├─────────────────┤              │                                                    │
                  │  ││port             │              │                                                    │
                  \  │`─────────────────┘              │                                                    │
                     │...                              │                                                    │
                     ├─────────────────────────────────┤                                                    │
                  /  │┌───────────────────┐            │ <──────────────────────────────────────────────────┘
                  │  ││func_name_offset   │            │ ──────────────────────────────┐
                  │  │├───────────────────┤            │                               │
                  │  ││func_name_len      │            │ │                             │
   func list info─┤  │`───────────────────┘            │ │─> amount as function_cnt    │
                  │  │┌───────────────────┐            │ │                             │
                  │  ││func_name_offset   │            │ ──────────────────────────────┼────┐
                  │  │├───────────────────┤            │                               │    │
                  │  ││func_name_len      │            │                               │    │
                  │  │`───────────────────┘            │                               │    │
                  \  │...                              │                               │    │
                     ├─────────────────────────────────┤                               │    │
                  /  │func_name_1                      │ <─────────────────────────────┘    │
                  │  │─────────────────────────────────│                                    │
                  │  │'\0'                             │                                    │
                  │  ├─────────────────────────────────┤                                    │
   func name data─┤  │func_name_2                      │ <──────────────────────────────────┘
                  │  ├─────────────────────────────────┤
                  │  │'\0'                             │
                  │  ├─────────────────────────────────┤
                  \  │...                              │
                     ┴─────────────────────────────────┘
 */

static int _calc_module_name_size(struct _inner_build_info* ibf)
{
	int len = 0;

	if (NULL == ibf->module_name) {
		DEBUG_ERROR("[mod_seed_file] module name Err");
		return -1;
	}
	len = strlen(ibf->module_name);

	ibf->need_alloc_size = sizeof(struct pg_cop_seed_file_header);
	ibf->need_alloc_size += 33;
	ibf->need_alloc_size += (len + 1);
	return 0;
}

static int _calc_tracker_size(struct _inner_build_info* ibf)
{
	struct pg_cop_seed_file_tracker_info_list* it = 0;
	int cnt = 0;

	list_for_each_entry(it, &ibf->tracker_list->list_head, list_head) {
		++cnt;
	}

	ibf->need_alloc_size += (sizeof(struct pg_cop_seed_file_tracker_info) * cnt);
	ibf->tracker_cnt = cnt;
	return 0;
}

static int _calc_func_size(struct _inner_build_info* ibf)
{
	struct pg_cop_seed_file_func_info_list* it = 0;
	int cnt = 0;
	int func_name_len_sum = 0;

	list_for_each_entry(it, &ibf->func_list->list_head, list_head) {
		++cnt;
		func_name_len_sum += (strlen(it->name) + 1);
	}
	ibf->need_alloc_size += (sizeof(struct pg_cop_seed_file_func_info) * cnt);
	ibf->need_alloc_size += func_name_len_sum;
	ibf->func_cnt = cnt;
	return 0;
}

static int _make_md5(const unsigned char* buf, int len, char** md5)
{
	unsigned char* md = MD5(buf, len, NULL);
	int i = 0;
	static char out[33];

	memset(out, 0, 33);
	for (i = 0; i < 16; ++i) {
		sprintf(&out[2 * i], "%02x", (int)md[i]);
	}
	*md5 = out;
	return 0;
}


static int _build_seed(struct _inner_build_info* ibf)
{
	//build header
	struct pg_cop_seed_file_header* ph = NULL;
	struct pg_cop_seed_file_tracker_info_list* itt = 0;
	struct pg_cop_seed_file_tracker_info* tracker_arr = 0;
	struct pg_cop_seed_file_func_info_list* itf = 0;
	struct pg_cop_seed_file_func_info* func_arr = 0;
	char* func_name_data_write_pos = 0;
	char* md5 = 0;
	int slen = 0;
	int cnt = 0;

	ibf->seed_buf = (char*)malloc(ibf->need_alloc_size);
	if (NULL == ibf->seed_buf) {
		DEBUG_ERROR("[mod_seed_file] alloc err!");
		return -1;
	}
	memset(ibf->seed_buf, 0, ibf->need_alloc_size);

	ph = (struct pg_cop_seed_file_header*)(ibf->seed_buf);
	ph->seed_file_size = ibf->need_alloc_size;
	ph->check_sum = 0;
	ph->version = 1;
	ph->module_name_leghth = strlen(ibf->module_name);
	ph->tracker_cnt = ibf->tracker_cnt;
	ph->tracker_list_header_offset = (sizeof(struct pg_cop_seed_file_header) + 33 + strlen(ibf->module_name) + 1);
	ph->function_cnt = ibf->func_cnt;
	ph->func_list_header_offset = (ph->tracker_list_header_offset +
	                               sizeof(struct pg_cop_seed_file_tracker_info) * ibf->tracker_cnt);

	//build module name
	strncpy((char*)(ibf->seed_buf + sizeof(struct pg_cop_seed_file_header) + 33), ibf->module_name, ph->module_name_leghth);

	//build tracker
	tracker_arr = (struct pg_cop_seed_file_tracker_info*)(ibf->seed_buf + ph->tracker_list_header_offset);
	list_for_each_entry(itt, &ibf->tracker_list->list_head, list_head) {
		tracker_arr[cnt].address = itt->info.address;
		tracker_arr[cnt].port = itt->info.port;
		tracker_arr[cnt].tracker_type = itt->info.tracker_type;
		++cnt;
	}
	cnt = 0;

	//build func
	func_arr = (struct pg_cop_seed_file_func_info*)(ibf->seed_buf + ph->func_list_header_offset);
	func_name_data_write_pos = (ibf->seed_buf + ph->func_list_header_offset + (ph->function_cnt * sizeof(struct pg_cop_seed_file_func_info)));
	list_for_each_entry(itf, &ibf->func_list->list_head, list_head) {
		slen = strlen(itf->name);
		func_arr[cnt].len = slen;
		func_arr[cnt].offset = (func_name_data_write_pos - ibf->seed_buf);
		strncpy(func_name_data_write_pos, itf->name, slen);
		func_name_data_write_pos += (slen + 1);
		++cnt;
	}

	//build hash
	_make_md5((unsigned char*)ibf->seed_buf, ibf->need_alloc_size, &md5);
	if (32 != strlen(md5)) {
		DEBUG_ERROR("md5 's length err!");
		return -1;
	}
	strncpy(ibf->seed_buf + sizeof(struct pg_cop_seed_file_header), md5, 32);
	return 0;
}

int pg_cop_seed_file_create(
    char* module_name,
    char* file_name,
    struct pg_cop_seed_file_tracker_info_list* tracker,
    struct pg_cop_seed_file_func_info_list* func)
{
	FILE* f = 0;
	struct _inner_build_info* _i = (struct _inner_build_info*)malloc(sizeof(struct _inner_build_info));
	memset(_i, 0, sizeof(struct _inner_build_info));
	_i->module_name = module_name;
	_i->tracker_list = tracker;
	_i->func_list = func;

	if (0 != _calc_module_name_size(_i)) goto err;
	if (0 != _calc_tracker_size(_i)) goto err;
	if (0 != _calc_func_size(_i)) goto err;
	if (0 != _build_seed(_i)) goto err;

	//write seed file
	f = fopen(file_name, "w");
	if (NULL == f) {
		DEBUG_ERROR("[mod_seed_file] can not open file: %s", file_name);
		return -1;
	}

	if (!fwrite(_i->seed_buf, _i->need_alloc_size, 1, f)) {
		DEBUG_ERROR("[mod_seed_file] can not write file");
		return -1;
	}
	fclose(f);
	free(_i->seed_buf);
	free(_i);
	return 0;

err:
	free(_i->seed_buf);
	free(_i);
	return -1;
}


static int _get_seed_buf(const char* file_name, void** buf)
{
	FILE* f = fopen(file_name, "r");
	struct pg_cop_seed_file_header seed_h;

	if (NULL == f) {
		DEBUG_ERROR("[mod_seed_file] cannot open seed file");
		return -1;
	}

	memset(&seed_h, 0, sizeof(struct pg_cop_seed_file_header));
	if (1 != fread(&seed_h, sizeof(struct pg_cop_seed_file_header), 1, f)) {
		DEBUG_ERROR("[mod_seed_file] cannot read seed header");
		return -1;
	}
	if (0 != fseek(f, 0, SEEK_SET)) {
		DEBUG_ERROR("[mod_seed_file] seek seed file head failed!");
		return -1;
	}

	*buf = malloc(seed_h.seed_file_size);
	if (NULL == (*buf)) {
		DEBUG_ERROR("[mod_seed_file] cannot alloc for seedbuf");
		return -1;
	}

	if (1 != fread(*buf, seed_h.seed_file_size, 1, f)) {
		DEBUG_ERROR("[mod_seed_file] cannot read seed header");
		return -1;
	}
	fclose(f);
	return 0;
}

static int _get_infohash(void* seed_buf, char** hash)
{
	*hash = strdup(((char*)seed_buf + sizeof(struct pg_cop_seed_file_header)));
	if (32 != strlen(*hash)) {
		DEBUG_ERROR("[mod_seed_file] get hash info error");
		return -1;
	}
	return 0;
}

static int _get_tracker_list(void* seed_buf, struct pg_cop_seed_file_tracker_info_list* l)
{
	struct pg_cop_seed_file_header* ph = (struct pg_cop_seed_file_header*)seed_buf;
	int cnt = ph->tracker_cnt;
	int i = 0;
	struct pg_cop_seed_file_tracker_info* track_arr = (struct pg_cop_seed_file_tracker_info*)(((char*)seed_buf +
	        ph->tracker_list_header_offset));

	memset(l, 0, sizeof(struct pg_cop_seed_file_tracker_info_list));
	INIT_LIST_HEAD(&(l)->list_head);

	for (i = 0; i < cnt; ++i) {
		struct pg_cop_seed_file_tracker_info_list* new_node = (struct pg_cop_seed_file_tracker_info_list*)
		        malloc(sizeof(struct pg_cop_seed_file_tracker_info_list));
		memset(new_node, 0, sizeof(struct pg_cop_seed_file_tracker_info_list));

		new_node->info.tracker_type = track_arr[i].tracker_type;
		new_node->info.address = track_arr[i].address;
		new_node->info.port = track_arr[i].port;
		//DEBUG_INFO("[mod_seed_file] type = %d, address=%d.%d.%d.%d, port=%d", new_node->info.tracker_type, (new_node->info.address >> 24), (new_node->info.address >> 16) & 0xFF, (new_node->info.address >> 8) & 0xFF, new_node->info.address & 0xFF, new_node->info.port);
		list_add_tail(&new_node->list_head, &l->list_head);
	}
	return 0;
}

int _get_module_name(void* seed_buf, char** name)
{
	*name = strdup(((char*)seed_buf + sizeof(struct pg_cop_seed_file_header) + 33));
	if (((struct pg_cop_seed_file_header*)seed_buf)->module_name_leghth != strlen(*name)) {
		DEBUG_ERROR("[mod_seed_file] get module name error:length assert err");
		return -1;
	}
	return 0;
}

int _get_func_list(void* seed_buf, struct pg_cop_seed_file_func_info_list* listh)
{
	struct pg_cop_seed_file_header* ph = (struct pg_cop_seed_file_header*)seed_buf;
	int cnt = ph->function_cnt;
	int i = 0;
	struct pg_cop_seed_file_func_info* func_arr = (struct pg_cop_seed_file_func_info*)(((char*)seed_buf +
	        ph->func_list_header_offset));

	memset(listh, 0, sizeof(struct pg_cop_seed_file_func_info_list));
	INIT_LIST_HEAD(&listh->list_head);

	for (i = 0; i < cnt; ++i) {
		struct pg_cop_seed_file_func_info_list* new_node = (struct pg_cop_seed_file_func_info_list*)
		        malloc(sizeof(struct pg_cop_seed_file_func_info_list));
		memset(new_node, 0, sizeof(struct pg_cop_seed_file_func_info_list));

		new_node->name = strdup((char*)seed_buf + func_arr[i].offset);
		if (func_arr[i].len != strlen(new_node->name)) {
			DEBUG_ERROR("func name length check error");
			return -1;
		}
		//DEBUG_INFO("func name=%s", new_node->name);
		list_add_tail(&new_node->list_head, &listh->list_head);
	}
	return 0;
}


int pg_cop_seed_file_parser_all_info(const char* file_name,
                                     char** module_name,
                                     char** hash,
                                     struct pg_cop_seed_file_tracker_info_list* track_h,
                                     struct pg_cop_seed_file_func_info_list* func_h)
{
	void* buf = 0;

	if (0 != _get_seed_buf(file_name, &buf)) goto e_3;
	if (0 != _get_module_name(buf, module_name)) goto e_3;
	if (0 != _get_infohash(buf, hash)) goto e_3;
	if (0 != _get_tracker_list(buf, track_h)) goto e_3;
	if (0 != _get_func_list(buf, func_h)) goto e_3;

	free(buf);
	return 0;

e_3:
	free(buf);
	return -1;
}


int pg_cop_seed_file_parser_release_buf(
    char** module_name,
    char** hash,
    struct pg_cop_seed_file_tracker_info_list* track_h,
    struct pg_cop_seed_file_func_info_list* func_h)
{
	struct list_head* dumb = track_h->list_head.next;
	free(*module_name);
	*module_name = NULL;

	free(*hash);
	*hash = NULL;

	while (dumb != &track_h->list_head) {
		struct pg_cop_seed_file_tracker_info_list* itt = list_entry(dumb, typeof(*itt), list_head);
		dumb = dumb->next;
		free(itt);
		itt = 0;
	}

	dumb = func_h->list_head.next;
	while (dumb != &func_h->list_head) {
		struct pg_cop_seed_file_func_info_list* itf = list_entry(dumb, typeof(*itf), list_head);
		dumb = dumb->next;
		free(itf->name);
		free(itf);
		itf = 0;
	}

	return 0;
}

