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

AC_PREREQ([2.68])
AC_INIT([PGCOP], [0.0.1], [pgcop.pocograph.com])
AM_INIT_AUTOMAKE
LT_INIT

AC_CONFIG_SRCDIR([hypervisor/pg_cop_hypervisor.c])
AC_CONFIG_SRCDIR([libPGCOP/pg_cop_modules.c])
AC_CONFIG_SRCDIR([modules/pg_cop_mod_socket.c])
AC_CONFIG_SRCDIR([tests/pg_cop_tester.c])
AC_CONFIG_SRCDIR([tools/pg_cop_seed_tool.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
CFLAGS+=" -Werror -Wall -Wno-format-security -DFOR_UNIT_TESTS"

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h strings.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero socket])

PKG_CHECK_MODULES(PGCOP_DEPS, [lua5.2])

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug],[enable debugging [default: no]]),
    [case "${enableval}" in
       yes) debug=true;;
        no) debug=false;;
         *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]);;
     esac], [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_CONFIG_FILES([Makefile
                 libPGCOP/Makefile
                 modules/Makefile
                 hypervisor/Makefile
                 tests/Makefile
				 tools/Makefile])

AC_OUTPUT

