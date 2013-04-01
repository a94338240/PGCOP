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

bin_PROGRAMS = pg-cop-hypervisor
pg_cop_hypervisor_SOURCES = pg_cop_hypervisor.c \
                            pg_cop_hypervisor.h \
                            pg_cop_hypervisor_opts_ag.h \
                            pg_cop_hypervisor_opts_ag.c
pg_cop_hypervisor_LDADD = ../libPGCOP/libPGCOP.la
pg_cop_hypervisor_LDFLAGS = -lopts
pg_cop_hypervisor_CFLAGS = -I $(srcdir)/../libPGCOP

