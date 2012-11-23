#include "pg_cop_rodata_strings.h"

const char rodata_str_usage[] = "Usage:\n\t--srvtype [socket | sharedmem]\tService type. \n\t--port\tListen port. (default: 12728)\n\n";
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
const char rodata_str_service_welcome_message[] = "PGCOP-HYPERVISOR-r1 [http://pocograph.com]\r\n\r\n";
const int rodata_size_str_service_welcome_message = sizeof(rodata_str_service_welcome_message);
const char rodata_str_cannot_detect_type_of_module_format[] = "Cannot detect the module type from %s";

const char rodata_path_modules[] = "modules";
const char rodata_path_modules_ext[] = ".so";
