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

lib_LTLIBRARIES = libPGCOP.la
libPGCOP_la_SOURCES = pg_cop_util.c \
		                  pg_cop_util.h \
				              pg_cop_modules.c \
                      pg_cop_modules.h \
				              pg_cop_interface.c \
                      pg_cop_interface.h \
                      pg_cop_debug.h \
				              pg_cop_config.h \
	   	                pg_cop_config.c \
				              pg_cop_vstack.h \
	   	                pg_cop_vstack.c \
				              pg_cop_seeds.h \
	   	                pg_cop_seeds.c \
											list.h

libPGCOP_la_CFLAGS = $(PGCOP_DEPS_CFLAGS)
libPGCOP_la_LDFLAGS = $(PGCOP_DEPS_LIBS) -lpthread -ldl

install-data-hook:
	if [ ! -f /etc/pgcop_conf.lua ]; then \
		echo "pgcop = {}" > /etc/pgcop_conf.lua ; \
		echo "pgcop.modules_path = \"$(datadir)/pgcop/modules\"" >> /etc/pgcop_conf.lua ; \
		echo "pgcop.seeds_path = \"$(datadir)/pgcop/seeds\"" >> /etc/pgcop_conf.lua ; \
		echo "pgcop.incoming_port = 12728" >> /etc/pgcop_conf.lua ; \
	fi
	mkdir -p $(datadir)/pgcop/seeds
