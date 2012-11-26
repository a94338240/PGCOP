#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([PGCOP], [0.0.1], [pgcop.pocograph.com])
AC_CONFIG_SRCDIR([hypervisor/pg_cop_hypervisor.c])
AC_CONFIG_SRCDIR([libPGCOP/pg_cop_modules.c])
AC_CONFIG_SRCDIR([modules/pg_cop_mod_socket.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [dlopen])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h strings.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero socket])

AC_CONFIG_FILES([Makefile
                 hypervisor/Makefile
                 libPGCOP/Makefile
                 modules/Makefile])
AC_OUTPUT
