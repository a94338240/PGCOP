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
#include "pg_cop_modules.h"
#include "pg_cop_config.h"
#include "pg_cop_interface.h"
#include "pg_cop_seeds.h"
#include "pg_cop_seed_file_parser.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char *ignore_modules[] = {
	"mod_tester_remote.so",
	"mod_pgcop_tracker.so",
	"mod_tester_service.so",
	NULL
};

static int pg_cop_vstack_test(const char **name)
{
	pg_cop_vstack_t *vstack, *vstack2;
	unsigned char u8;
	const char *str = "Test string.";
	char ext[6];
	char *outstr;

	*name = __FUNCTION__;

	vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024);
	vstack2 = pg_cop_vstack_new(0, 8 * 1024 * 1024);
	assert(vstack);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0xF2) == 0);
	assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0xF2);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x72) == 0);
	assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0x72);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x78) == 0);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x73) == 0);
	assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0x73);
	assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0x78);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_STRING, str) == 0);
	assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_STRING, &outstr) == 0);
	assert(strcmp(str, outstr) == 0);
	free(outstr);

	assert(pg_cop_get_file_extension("test.coco", ext, sizeof(ext)) == 0);
	assert(strcmp(ext, ".coco") == 0);

	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x78) == 0);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_STRING, str) == 0);

	assert(pg_cop_vstack_import(vstack2, vstack->data_area, vstack->top) == 0);
	assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_STRING, &outstr) == 0);
	assert(strcmp(str, outstr) == 0);
	free(outstr);
	assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0x78);

	pg_cop_vstack_destroy(vstack);
	pg_cop_vstack_destroy(vstack2);

	return 0;
}

static int pg_cop_vstack_transfer_test(const char **name)
{
	pg_cop_vstack_t *vstack, *vstack2;
	unsigned char u8;

	*name = __FUNCTION__;

	vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024);
	assert(vstack);
	vstack2 = pg_cop_vstack_new(0, 8 * 1024 * 1024);
	assert(vstack2);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0xF2) == 0);
	assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x72) == 0);
	assert(pg_cop_vstack_transfer(vstack, vstack2) == 0);
	assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0xF2);
	assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_U8, &u8) == 0);
	assert(u8 == 0x72);
	pg_cop_vstack_destroy(vstack);
	pg_cop_vstack_destroy(vstack2);
	return 0;
}

static int pg_cop_seeds_load_test(const char **name)
{
	*name = __FUNCTION__;
	assert(pg_cop_init_seeds_table() == 0);
	pg_cop_seeds_path = "./";
	assert(pg_cop_load_seeds(0, NULL) == 0);

	void *res;

	pg_cop_module_interface_t *intf_tester = pg_cop_module_interface_connect("mod_tester_service");
	assert(intf_tester);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "TEST MESSAGE") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong TEST MESSAGE") == 0);
	free(res);
	assert(pg_cop_module_interface_disconnect(intf_tester) == 0);

	return 0;
}

static int pg_cop_modules_load_test(const char **name)
{
	*name = __FUNCTION__;
	pg_cop_module_t *module;

	assert(pg_cop_read_config() == 0);
	assert(pg_cop_module_interface_daemon_init() == 0);

	assert(pg_cop_init_modules_table() == 0);
	assert(pg_cop_load_modules(0, NULL) == 0);

	list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
		assert(pg_cop_module_init(module, 0, NULL) == 0);
		assert(pg_cop_module_start(module) == 0);
	}

	return 0;
}

static int pg_cop_interface_invoke_test(const char **name)
{
	*name = __FUNCTION__;

	pg_cop_module_interface_t *intf_tester;
	char *res;

	intf_tester = pg_cop_module_interface_connect("mod_tester");
	assert(intf_tester);
	assert(pg_cop_module_interface_invoke(intf_tester, "oops", 0) == 0);
	assert(pg_cop_module_interface_invoke(intf_tester, "oops", 0) == 0);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "TEST MESSAGE") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong TEST MESSAGE") == 0);
	free(res);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "TEST MESSAGE 2") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong TEST MESSAGE 2") == 0);
	free(res);
	assert(pg_cop_module_interface_disconnect(intf_tester) == 0);

	intf_tester = pg_cop_module_interface_connect("mod_tester");
	assert(intf_tester);
	assert(pg_cop_module_interface_invoke(intf_tester, "oops", 0) == 0);
	assert(pg_cop_module_interface_invoke(intf_tester, "oops", 0) == 0);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "TEST MESSAGE") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong TEST MESSAGE") == 0);
	free(res);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "TEST MESSAGE 2") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong TEST MESSAGE 2") == 0);
	free(res);
	assert(pg_cop_module_interface_disconnect(intf_tester) == 0);

	return 0;
}

static int pg_cop_interface_remote_invoke_test(const char **name)
{
	*name = __FUNCTION__;

	pg_cop_module_interface_t *intf_tester;
	pg_cop_module_interface_t *intf_remote;
	char *res;

	intf_remote = pg_cop_module_interface_announce("mod_tester_remote",
	              MODULE_INTERFACE_TYPE_SOCKET_TCP,
	              "127.0.0.1", 12728);
	// Call twice
	intf_tester = pg_cop_module_interface_connect("mod_tester_remote");
	assert(intf_tester);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "REMOTE TEST MESSAGE") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong REMOTE TEST MESSAGE") == 0);
	free(res);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "REMOTE TEST MESSAGE 2") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong REMOTE TEST MESSAGE 2") == 0);
	free(res);
	assert(pg_cop_module_interface_disconnect(intf_tester) == 0);

	// Disconnect twice
	intf_tester = pg_cop_module_interface_connect("mod_tester_remote");
	assert(intf_tester);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "REMOTE TEST MESSAGE") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong REMOTE TEST MESSAGE") == 0);
	free(res);
	assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1,
	                                      VSTACK_TYPE_STRING, "REMOTE TEST MESSAGE 2") == 0);
	assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
	assert(strcmp(res, "pong REMOTE TEST MESSAGE 2") == 0);
	free(res);
	assert(pg_cop_module_interface_disconnect(intf_tester) == 0);
	DEBUG_INFO("TYPE=%d get", intf_remote->type);
	assert(pg_cop_module_interface_revoke(intf_remote) == 0);
	return 0;
}


struct pg_cop_seed_file_tracker_info seed_file_test_arr_t[3] = {
	{1, (127 << 24) | (0 << 16) | (0 << 8) | (1), 12728},
	{2, (10 << 24) | (1 << 16) | (2 << 8) | (130), 122},
	{3, (10 << 24) | (1 << 16) | (3 << 8) | (117), 123},
};

char* seed_file_test_arr_f[4] = {
	"fucnabcd",
	"xxx123",
	"___func_x",
	"Fn_abort"
};

static int pg_cop_seeds_file_generate_test(char* seed_file_name)
{
	struct pg_cop_seed_file_tracker_info_list t0, t1, t2, t3;
	struct pg_cop_seed_file_func_info_list f0, f1, f2, f3, f4;
	int idx = 0;

	INIT_LIST_HEAD(&t0.list_head);
	INIT_LIST_HEAD(&f0.list_head);

	t1.info.tracker_type = seed_file_test_arr_t[idx].tracker_type;
	t1.info.address = seed_file_test_arr_t[idx].address;
	t1.info.port = seed_file_test_arr_t[idx].port;
	list_add_tail(&t1.list_head, &t0.list_head);
	++idx;


	t2.info.tracker_type = seed_file_test_arr_t[idx].tracker_type;
	t2.info.address = seed_file_test_arr_t[idx].address;
	t2.info.port = seed_file_test_arr_t[idx].port;
	list_add_tail(&t2.list_head, &t0.list_head);
	++idx;

	t3.info.tracker_type = seed_file_test_arr_t[idx].tracker_type;
	t3.info.address = seed_file_test_arr_t[idx].address; //167838581
	t3.info.port = seed_file_test_arr_t[idx].port;
	list_add_tail(&t3.list_head, &t0.list_head);
	++idx;


	idx = 0;
	f1.name = seed_file_test_arr_f[idx];
	list_add_tail(&f1.list_head, &f0.list_head);
	++idx;


	f2.name = seed_file_test_arr_f[idx];
	list_add_tail(&f2.list_head, &f0.list_head);
	++idx;


	f3.name = seed_file_test_arr_f[idx];
	list_add_tail(&f3.list_head, &f0.list_head);
	++idx;

	f4.name = seed_file_test_arr_f[idx];
	list_add_tail(&f4.list_head, &f0.list_head);
	++idx;

	assert(!pg_cop_seed_file_create("mod_a000", seed_file_name, &t0, &f0));
	return 0;
}

static int pg_cop_seeds_file_parser_test(char* file_name)
{
	char* hash;
	struct pg_cop_seed_file_tracker_info_list track_h;
	struct pg_cop_seed_file_func_info_list func_h;
	char* module_name = 0;

	struct pg_cop_seed_file_tracker_info_list* it = 0;
	struct pg_cop_seed_file_func_info_list* itl = 0;
	int cnt = 0;

	assert(0 == pg_cop_seed_file_parser_all_info(file_name, &module_name, &hash, &track_h, &func_h));
	assert(32 == strlen(hash));
	assert(0 == strcmp(module_name, "mod_a000"));

	DEBUG_INFO("get module name=%s", module_name);
	DEBUG_INFO("get hash=%s", hash);
	list_for_each_entry(it, &track_h.list_head, list_head) {
		assert(seed_file_test_arr_t[cnt].tracker_type == it->info.tracker_type);
		assert(seed_file_test_arr_t[cnt].address == it->info.address);
		assert(seed_file_test_arr_t[cnt].port == it->info.port);
		DEBUG_INFO("[mod_seed_file] type = %d, address=%d.%d.%d.%d, port=%d", it->info.tracker_type, (it->info.address >> 24), (it->info.address >> 16) & 0xFF, (it->info.address >> 8) & 0xFF, it->info.address & 0xFF, it->info.port);
		++cnt;
	}
	assert((sizeof(seed_file_test_arr_t) / sizeof(struct pg_cop_seed_file_tracker_info)) == cnt);
	DEBUG_INFO("seed tracker cnt = %d", cnt);


	cnt = 0;
	list_for_each_entry(itl, &func_h.list_head, list_head) {
		assert(0 == strcmp(itl->name, seed_file_test_arr_f[cnt]));
		DEBUG_INFO("func name=%s", itl->name);
		++cnt;
	}
	assert(sizeof(seed_file_test_arr_f) / sizeof(char*) == cnt);
	DEBUG_INFO("func cnt = %d", cnt);


	if (0 != pg_cop_seed_file_parser_release_buf(&module_name, &hash, &track_h, &func_h)) return -1;

	return 0;
}

static int pg_cop_seeds_scenario_test(const char **name)
{
	*name = __FUNCTION__;
	char* seed_file_name = "x123.seed";

	if (0 != pg_cop_seeds_file_generate_test(seed_file_name)) return -1;

	if (0 != pg_cop_seeds_file_parser_test(seed_file_name)) return -1;
	return 0;
}





typedef struct {
	int (*method)(const char **name);
	int num;
} test_case_t;

test_case_t test_cases[] = {
	{pg_cop_seeds_scenario_test, 1},
	{pg_cop_vstack_test, 1},
	{pg_cop_modules_load_test, 1},
	{pg_cop_vstack_transfer_test, 1},
	{pg_cop_interface_invoke_test, 1},
	{pg_cop_interface_remote_invoke_test, 1},
	{pg_cop_seeds_load_test, 1},
	{NULL, 0}
};

int main()
{
	int i = 0;
	int j = 0;
	const char *name;

	pg_cop_ignore_modules = ignore_modules;

	for (i = 0;; i++) {
		if (test_cases[i].num == 0) {
			printf("All tests done.\n");
			goto out;
		}
		for (j = 0; j < test_cases[i].num; j++) {
			if (!test_cases[i].method(&name)) {
				fprintf(stdout, "Testing Function: %s, %d times, Result: OK\n", name, j + 1);
				fflush(stdout);
			}
		}
	}
out:

	return 0;
}
