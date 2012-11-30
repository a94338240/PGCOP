#include "pg_cop_rodata_strings.h"

const char rodata_str_usage[] = "Usage:\n\t--help\t\t\t\tThis help.\n\t--conf <config file>\t\tLoad config file.\n\n";
const char rodata_str_service_type_invalid[] = "Service type invalid.";
const char rodata_str_port_not_in_range[] = "Port not in range.";
const char rodata_str_service_started[] = "Pocograph COP Hypervisor started!";
const char rodata_str_cannot_find_open_dir[] = "Cannot open module dir.";
const char rodata_str_module_loaded_format[] = "Module %s loaded!";
const char rodata_str_module_cannot_be_load[] = "Cannot load module %s";
const char rodata_str_module_nosym_module_info[] = "Cannot find symbol pg_cop_module_info in %s, the module is skipped.";
const char rodata_str_pthread_mem_failed[] = "Cannot allocate memory while thread creating.";
const char rodata_str_cannot_create_thread[] = "Cannot create thread.";
const char rodata_str_accept_error[] = "Cannot accept a connection.";
const char rodata_str_module_nosym_module_hooks[] = "Cannot find symbol pg_cop_module_hooks in %s, the module is skipped.";
const char rodata_str_com_module_enabled[] = "%d communication modules loaded.";
const char rodata_str_no_com_module[] = "No communication modules, hypervisor terminated.";
const char rodata_str_service_welcome_message[] = "PGCOP-HYPERVISOR-r1 [http://pocograph.com]\r\n";
const int rodata_size_str_service_welcome_message = sizeof(rodata_str_service_welcome_message);
const char rodata_str_cannot_detect_type_of_module_format[] = "Cannot detect the module type from %s";
const char rodata_str_lua_error[] = "%s";
const char rodata_str_unknown_protocol[] = "Protocol module loaded with no protocol specified.";
const char rodata_str_module_serv_to_you_format[] = "MODULE: %s serv to you.\r\n\r\n";
const char rodata_str_client_disconnected[] = "Client disconnected.";
const char rodata_str_protocol_process_skipped[] = "Protocol skipped";

const char rodata_path_lua_config_file[] = "/etc/pgcop_conf.lua";
const char rodata_path_modules[] = "/usr/share/pgcop/modules";
const char rodata_path_modules_ext[] = ".so";
