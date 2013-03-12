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

#ifndef PG_COP_CONFIG_H
#define PG_COP_CONFIG_H

extern const char *pg_cop_lua_config_file;
extern int tracker_incoming_port;

int pg_cop_read_config();
int pg_cop_get_module_config_number(const char *conf_key, int *num);
int pg_cop_get_module_config_strdup(const char *conf_key, char **str);
int pg_cop_get_config_number(const char *conf_key, int *num);
int pg_cop_get_config_strdup(const char *conf_key, char **str);

#endif /* PG_COP_CONFIG_H */
