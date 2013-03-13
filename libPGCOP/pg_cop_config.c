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

#include "pg_cop_config.h"
#include "pg_cop_modules.h"
#include "pg_cop_debug.h"
#include "pg_cop_seeds.h"

#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char *pg_cop_lua_config_file = NULL;


int pg_cop_read_config()
{
	lua_State *L = (lua_State *)luaL_newstate();
	const char *filename = pg_cop_lua_config_file;

	if (!filename)
		filename = "/etc/pgcop_conf.lua";

	if (luaL_loadfilex(L, filename, NULL) ||
	        lua_pcall(L, 0, 0, 0)) {
		DEBUG_CRITICAL("Cannot load configs from lua. error=%s", lua_tostring(L, -1));
	}

	lua_getglobal(L, "pgcop");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "modules_path");
		if (lua_isstring(L, -1))
			pg_cop_modules_path = strdup(lua_tostring(L, -1));
	}
	lua_getglobal(L, "pgcop");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "seeds_path");
		if (lua_isstring(L, -1))
			pg_cop_seeds_path = strdup(lua_tostring(L, -1));
	}
	lua_getglobal(L, "pgcop");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "incoming_port");
		if (lua_isnumber(L, -1))
			tracker_incoming_port = lua_tonumber(L, -1);
	}

	if (pg_cop_modules_path == NULL)
		pg_cop_modules_path = "/usr/local/pgcop/share/pgcop/modules";
	if (pg_cop_seeds_path == NULL)
		pg_cop_seeds_path = "/usr/local/pgcop/share/pgcop/seeds";

	lua_close(L);
	return 0;
}

int pg_cop_get_module_config_strdup(const char *conf_key, char **str)
{
	lua_State *L = (lua_State *)luaL_newstate();
	const char *filename = pg_cop_lua_config_file;
	char *tmp = strdup(conf_key);
	char *np = tmp;
	char *cp;
	int res = -1;

	if (!filename)
		filename = "/etc/pgcop_conf.lua";

	if (luaL_loadfilex(L, filename, NULL) ||
	        lua_pcall(L, 0, 0, 0)) {
		DEBUG_ERROR("Cannot load configs from lua. error=%s", lua_tostring(L, -1));
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
	if (res)
		DEBUG_ERROR("Config key %s not be found.", conf_key);
	if (tmp)
		free(tmp);
	lua_close(L);
	return res;
}

int pg_cop_get_module_config_number(const char *conf_key, int *num)
{
	lua_State *L = (lua_State *)luaL_newstate();
	const char *filename = pg_cop_lua_config_file;
	char *tmp = strdup(conf_key);
	char *np = tmp;
	char *cp;
	int res = -1;

	if (!filename)
		filename = "/etc/pgcop_conf.lua";

	if (luaL_loadfilex(L, filename, NULL) ||
	        lua_pcall(L, 0, 0, 0)) {
		DEBUG_ERROR("Cannot load configs from lua. error=%s", lua_tostring(L, -1));
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
	if (res)
		DEBUG_ERROR("Config key %s not be found.", conf_key);
	if (tmp)
		free(tmp);
	lua_close(L);
	return res;
}

int pg_cop_get_config_strdup(const char *conf_key, char **str)
{
	lua_State *L = (lua_State *)luaL_newstate();
	const char *filename = pg_cop_lua_config_file;
	char *tmp = strdup(conf_key);
	char *np = tmp;
	char *cp;
	int res = -1;

	if (!filename)
		filename = "/etc/pgcop_conf.lua";

	if (luaL_loadfilex(L, filename, NULL) ||
	        lua_pcall(L, 0, 0, 0)) {
		DEBUG_ERROR("Cannot load configs from lua. error=%s", lua_tostring(L, -1));
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
	if (res)
		DEBUG_ERROR("Config key %s not be found.", conf_key);
	if (tmp)
		free(tmp);
	lua_close(L);
	return res;
}

int pg_cop_get_config_number(const char *conf_key, int *num)
{
	lua_State *L = (lua_State *)luaL_newstate();
	const char *filename = pg_cop_lua_config_file;
	char *tmp = strdup(conf_key);
	char *np = tmp;
	char *cp;
	int res = -1;

	if (!filename)
		filename = "/etc/pgcop_conf.lua";

	if (luaL_loadfilex(L, filename, NULL) ||
	        lua_pcall(L, 0, 0, 0)) {
		DEBUG_ERROR("Cannot load configs from lua. error=%s", lua_tostring(L, -1));
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
	if (res)
		DEBUG_ERROR("Config key %s not be found.", conf_key);
	if (tmp)
		free(tmp);
	lua_close(L);
	return res;
}
