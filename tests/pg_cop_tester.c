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
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int pg_cop_vstack_test(const char **name)
{
  pg_cop_vstack_t *vstack, *vstack2;
  unsigned char u8;
  const char *str = "Test string.";
  char ext[6];
  char *outstr;

  *name = __FUNCTION__;

  vstack = pg_cop_vstack_new(0, 8*1024*1024);
  vstack2 = pg_cop_vstack_new(0, 8*1024*1024);
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
  assert(pg_cop_vstack_pop(vstack, VSTACK_TYPE_STRING, &outstr) != 0);

  assert(pg_cop_get_file_extension("test.coco", ext, sizeof(ext)) == 0);
  assert(strcmp(ext, ".coco") == 0);

  assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_U8, 0x78) == 0);
  assert(pg_cop_vstack_push(vstack, VSTACK_TYPE_STRING, str) == 0);

  assert(pg_cop_vstack_import(vstack2, vstack->data_area, vstack->top) == 0);
  assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_STRING, &outstr) == 0);
  assert(strcmp(str, outstr) == 0);
  assert(pg_cop_vstack_pop(vstack2, VSTACK_TYPE_U8, &u8) == 0);
  assert(u8 == 0x78);

  free(outstr);
  pg_cop_vstack_destroy(vstack);

  return 0;
}

static int pg_cop_vstack_transfer_test(const char **name)
{
  pg_cop_vstack_t *vstack, *vstack2;
  unsigned char u8;

  *name = __FUNCTION__;

  vstack = pg_cop_vstack_new(0, 8*1024*1024);
  assert(vstack);
  vstack2= pg_cop_vstack_new(0, 8*1024*1024);
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

static int pg_cop_modules_load_test(const char **name)
{
  *name = __FUNCTION__;
  pg_cop_module_t *module;

  pg_cop_module_interface_tracker_init();
  assert(pg_cop_load_modules(0, NULL) != 0);
  assert(pg_cop_read_config() == 0);
  assert(pg_cop_init_modules_table() == 0);
  assert(pg_cop_load_modules(0, NULL) == 0);
  
  list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
    assert(pg_cop_module_init(module, 0, NULL) == 0);
    assert(pg_cop_module_start(module) == 0);
  }
  
  return 0;
}

static int pg_cop_interface_invoke_test(const char **name) {
  *name = __FUNCTION__;

  pg_cop_module_interface_t *intf_tester;
  char *res;
  
  intf_tester = pg_cop_module_interface_new("tester", 
                                            MODULE_INTERFACE_TYPE_THREAD);
  pg_cop_module_interface_connect(intf_tester, "mod_tester");
  assert(intf_tester);
  assert(pg_cop_module_interface_invoke(intf_tester, "oops", 0) == 0);
  assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1, 
                                        VSTACK_TYPE_STRING, "TEST MESSAGE") == 0);
  assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
  assert(strcmp(res, "pong TEST MESSAGE") == 0);
  free(res);
  assert(pg_cop_module_interface_disconnect(intf_tester) == 0);
}

static int pg_cop_interface_remote_invoke_test(const char **name) {
  *name = __FUNCTION__;

  pg_cop_module_interface_t *intf_tester;
  char *res;
  
  pg_cop_module_interface_announce(pg_cop_module_interface_new("mod_tester_remote", 
                                   MODULE_INTERFACE_TYPE_SOCKET_TCP));

  intf_tester = pg_cop_module_interface_new("tester", 
                                            MODULE_INTERFACE_TYPE_THREAD);
  pg_cop_module_interface_connect(intf_tester, "mod_tester_remote");
  assert(intf_tester);
  assert(pg_cop_module_interface_invoke(intf_tester, "ping", 1, 
                                        VSTACK_TYPE_STRING, "REMOTE TEST MESSAGE") == 0);
  assert(pg_cop_module_interface_pop(intf_tester, VSTACK_TYPE_STRING, &res) == 0);
  assert(strcmp(res, "pong REMOTE TEST MESSAGE") == 0);
  free(res);
  assert(pg_cop_module_interface_disconnect(intf_tester) == 0);
}

int (*pg_cop_tests[])(const char **name) = {
  pg_cop_vstack_test,
  pg_cop_modules_load_test,
  pg_cop_vstack_transfer_test,
  pg_cop_interface_invoke_test,
  pg_cop_interface_remote_invoke_test,
  NULL
};

int main()
{
  int i = 0;
  const char *name;
  for (;; i++) {
    if (pg_cop_tests[i] == NULL) {
      printf("All tests done.\n");
      break;
    }
    if (!pg_cop_tests[i](&name)) {
      fprintf(stdout, "Testing Function: %s Result: OK\n", name);
      fflush(stdout);
    }
  }

  return 0;
}
