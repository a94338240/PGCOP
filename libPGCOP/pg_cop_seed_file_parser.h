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

#ifndef PG_COP_SEED_FILE_PARSER_H
#define PG_COP_SEED_FILE_PARSER_H
#include "list.h"


struct pg_cop_seed_file_tracker_info {
	char tracker_type;
	unsigned int address;
	unsigned short port;
};

struct pg_cop_seed_file_tracker_info_list {
	struct pg_cop_seed_file_tracker_info info;
	struct list_head list_head;
};

struct pg_cop_seed_file_func_info {
	unsigned int      offset;
	unsigned int      len;
};

struct pg_cop_seed_file_func_info_list {
	char* name;
	struct list_head list_head;
};

#pragma pack(push, 1)
struct pg_cop_seed_file_header {
	unsigned int        seed_file_size;
	unsigned int        check_sum;
	unsigned short      version;
	unsigned short      module_name_leghth;
	unsigned int        tracker_cnt;
	unsigned int        tracker_list_header_offset;
	unsigned short      function_cnt;
	unsigned int        func_list_header_offset;
};
#pragma pack(pop)

int pg_cop_seed_file_create(
    char* module_name,
    char* file_name,
    struct pg_cop_seed_file_tracker_info_list* tracker,
    struct pg_cop_seed_file_func_info_list* func);

int pg_cop_seed_file_parser_all_info(char* file_name,
                                     char** module_name,
                                     char** hash,
                                     struct pg_cop_seed_file_tracker_info_list* track_h,
                                     struct pg_cop_seed_file_func_info_list* func_h);

int pg_cop_seed_file_parser_release_buf(
    char** module_name,
    char** hash,
    struct pg_cop_seed_file_tracker_info_list* track_h,
    struct pg_cop_seed_file_func_info_list* func_h);

#endif /* PG_COP_SEED_FILE_PARSER_H */
