#   PGCOP - PocoGraph Component Oriented Platform.
#   Copyright (C) 2013  David Wu <david@pocograph.com>

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

noinst_PROGRAMS = seed_tool
seed_tool_SOURCES = pg_cop_seed_tool.c \
					pg_cop_seed_tool_opts_ag.c

seed_tool_CFLAGS = -I ../libPGCOP
seed_tool_LDADD = ../libPGCOP/libPGCOP.la 
seed_tool_LDFLAGS = -lopts
