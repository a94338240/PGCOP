#include "pg_cop_config.h"
#include "pg_cop_modules.h"
#include "pg_cop_rodata_strings.h"
#include "pg_cop_debug.h"

#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char *pg_cop_lua_config_file = NULL;


void pg_cop_read_config()
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = pg_cop_lua_config_file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];

  if (!filename)
    filename = rodata_path_lua_config_file;

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

int pg_cop_get_module_config_strdup(const char *conf_key, char **str)
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = pg_cop_lua_config_file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  char *tmp = strdup(conf_key);
  char *np = tmp;
  char *cp;
  int res = -1;

  if (!filename)
    filename = rodata_path_lua_config_file;

  if (luaL_loadfilex(L, filename, NULL) || 
      lua_pcall(L, 0, 0, 0)) {
    sprintf(debug_info, rodata_str_lua_error, 
            lua_tostring(L, -1));
    DEBUG_ERROR(debug_info);
    return -1;
  }
 
  lua_getglobal(L, "pgcop");
  if (!lua_istable(L, -1))
    goto out;
  lua_getfield(L, -1, "modules");
  if (!lua_istable(L, -1))
    goto out;
  while ((cp = strsep(&np, "."))) {
    if (np) {
      lua_getfield(L, -1, cp);
      if (!lua_istable(L, -1))
        goto out;
    } else {
      lua_getfield(L, -1, cp);
      if (!lua_isstring(L, -1))
        goto out;
      res = 0;
      *str = strdup(lua_tostring(L, -1));
      break;
    }
  }

 out:
  if (tmp)
    free(tmp);
  lua_close(L);
  return res;
}

int pg_cop_get_module_config_number(const char *conf_key, int *num)
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = pg_cop_lua_config_file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  char *tmp = strdup(conf_key);
  char *np = tmp;
  char *cp;
  int res = -1;

  if (!filename)
    filename = rodata_path_lua_config_file;

  if (luaL_loadfilex(L, filename, NULL) || 
      lua_pcall(L, 0, 0, 0)) {
    sprintf(debug_info, rodata_str_lua_error, 
            lua_tostring(L, -1));
    DEBUG_ERROR(debug_info);
    return -1;
  }
 
  lua_getglobal(L, "pgcop");
  if (!lua_istable(L, -1))
    goto out;
  lua_getfield(L, -1, "modules");
  if (!lua_istable(L, -1))
    goto out;
  while ((cp = strsep(&np, "."))) {
    if (np) {
      lua_getfield(L, -1, cp);
      if (!lua_istable(L, -1))
        goto out;
    } else {
      lua_getfield(L, -1, cp);
      if (!lua_isnumber(L, -1))
        goto out;
      res = 0;
      *num = lua_tonumber(L, -1);
      break;
    }
  }

 out:
  if (tmp)
    free(tmp);
  lua_close(L);
  return res;
}

int pg_cop_get_config_strdup(const char *conf_key, char **str)
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = pg_cop_lua_config_file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  char *tmp = strdup(conf_key);
  char *np = tmp;
  char *cp;
  int res = -1;

  if (!filename)
    filename = rodata_path_lua_config_file;

  if (luaL_loadfilex(L, filename, NULL) || 
      lua_pcall(L, 0, 0, 0)) {
    sprintf(debug_info, rodata_str_lua_error, 
            lua_tostring(L, -1));
    DEBUG_ERROR(debug_info);
    return -1;
  }
 
  lua_getglobal(L, "pgcop");
  if (!lua_istable(L, -1))
    goto out;
  while ((cp = strsep(&np, "."))) {
    if (np) {
      lua_getfield(L, -1, cp);
      if (!lua_istable(L, -1))
        goto out;
    } else {
      lua_getfield(L, -1, cp);
      if (!lua_isstring(L, -1))
        goto out;
      res = 0;
      *str = strdup(lua_tostring(L, -1));
      break;
    }
  }

 out:
  if (tmp)
    free(tmp);
  lua_close(L);
  return res;
}

int pg_cop_get_config_number(const char *conf_key, int *num)
{
  lua_State *L = (lua_State *)luaL_newstate();
  const char *filename = pg_cop_lua_config_file;
  char debug_info[MAXLEN_LOAD_MODULE_DEBUG_INFO];
  char *tmp = strdup(conf_key);
  char *np = tmp;
  char *cp;
  int res = -1;

  if (!filename)
    filename = rodata_path_lua_config_file;

  if (luaL_loadfilex(L, filename, NULL) || 
      lua_pcall(L, 0, 0, 0)) {
    sprintf(debug_info, rodata_str_lua_error, 
            lua_tostring(L, -1));
    DEBUG_ERROR(debug_info);
    return -1;
  }
 
  lua_getglobal(L, "pgcop");
  if (!lua_istable(L, -1))
    goto out;
  while ((cp = strsep(&np, "."))) {
    if (np) {
      lua_getfield(L, -1, cp);
      if (!lua_istable(L, -1))
        goto out;
    } else {
      lua_getfield(L, -1, cp);
      if (!lua_isnumber(L, -1))
        goto out;
      res = 0;
      *num = lua_tonumber(L, -1);
      break;
    }
  }

 out:
  if (tmp)
    free(tmp);
  lua_close(L);
  return res;
}
