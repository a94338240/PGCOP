#include "pg_cop_config.h"
#include "pg_cop_modules.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_debug.h"

#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void pg_cop_read_config(const char *file)
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];

  if (!filename)
    filename = rodata_path_lua_config_file;

  luaL_openlibs(L);
  if (luaL_loadfilex(L, filename, NULL) || 
      lua_pcall(L, 0, 0, 0)) {
    sprintf(debug_info, rodata_str_lua_error, 
            lua_tostring(L, -1));
    DEBUG_ERROR(debug_info);
    return;
  }
 
  lua_getglobal(L, "pgcop");
  if (lua_istable(L, -1)) {
    lua_getfield(L, -1, "modules_path");
    if (lua_isstring(L, -1))
      pg_cop_modules_path = strdup(lua_tostring(L, -1));
  }

  lua_close(L);
}
