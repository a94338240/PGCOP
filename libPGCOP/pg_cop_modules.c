#include "pg_cop_modules.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>

pg_cop_module_t *pg_cop_modules_list_for_com;
pg_cop_module_t *pg_cop_modules_list_for_trans;

void pg_cop_init_modules_table()
{
  pg_cop_modules_list_for_com = 
    (pg_cop_module_t *)malloc(sizeof(pg_cop_module_t));
  PG_COP_LIST_HEAD(pg_cop_modules_list_for_com);

  pg_cop_modules_list_for_trans = 
    (pg_cop_module_t *)malloc(sizeof(pg_cop_module_t));
  PG_COP_LIST_HEAD(pg_cop_modules_list_for_trans);
}

void pg_cop_load_modules()
{
  DIR *module_dir;
  struct dirent *module_dir_entry;
  char file_ext[MAXLEN_MODULE_FILE_EXT] = {};
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO] = {};
  char module_path[MAXLEN_MODULE_PATH] = {};
  pg_cop_module_t *module;
  void *dl_handle;
  void *module_hooks;
  pg_cop_module_info_t *module_info;
  pg_cop_module_t *list;

  module_dir = opendir(rodata_path_modules);
  if (!module_dir)
    DEBUG_CRITICAL(rodata_str_cannot_find_open_dir);
  while ((module_dir_entry = readdir(module_dir))) {
    if (!module_dir_entry->d_name)
      continue;
    if (pg_cop_get_file_extension(module_dir_entry->d_name, 
                                  file_ext, sizeof(file_ext)) != 0)
      continue;
    if (module_dir_entry->d_type != DIRE_TYPE_REGULAR_FILE ||
        strncmp(file_ext, rodata_path_modules_ext, 2))
      continue;

    strncpy(module_path, rodata_path_modules, sizeof(module_path));
    strncat(module_path, "/", sizeof(module_path));
    strncat(module_path, module_dir_entry->d_name, sizeof(module_path));
    dl_handle = dlopen(module_path, RTLD_LAZY);

    if (!dl_handle) {
      sprintf(debug_info, rodata_str_module_cannot_be_load, 
              module_dir_entry->d_name);
      DEBUG_ERROR(debug_info);
      continue;
    }

    module_info = dlsym(dl_handle, "pg_cop_module_info");
    if (!module_info) {
      sprintf(debug_info, rodata_str_module_nosym_module_info,
              module_dir_entry->d_name);
      DEBUG_ERROR(debug_info);
      continue;
    }

    module_hooks = dlsym(dl_handle, "pg_cop_module_hooks");
    if (!module_hooks) {
      sprintf(debug_info, rodata_str_module_nosym_module_hooks,
              module_dir_entry->d_name);
      DEBUG_ERROR(debug_info);
      continue;
    }

    module = (pg_cop_module_t *)malloc(sizeof(pg_cop_module_t));
    module->dl_handle = dl_handle;
    module->hooks = module_hooks;
    module->info = module_info;

    switch (module->info->type) {
    case PG_COP_MODULE_TYPE_COM:
      list = pg_cop_modules_list_for_com;
      break;
    case PG_COP_MODULE_TYPE_TRANSCEIVER:
      list = pg_cop_modules_list_for_trans;
      break;
    default:
      sprintf(debug_info, rodata_str_cannot_detect_type_of_module_format, 
              module_dir_entry->d_name);
      DEBUG_ERROR(debug_info);
      continue;
    }

    PG_COP_LIST_ADD_TAIL(list, module);

    sprintf(debug_info, rodata_str_module_loaded_format,
            module_dir_entry->d_name);
    DEBUG_INFO(debug_info);
  }

  closedir(module_dir);
}
