#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([PGCOP], [0.0.1], [pgcop.pocograph.com])
AM_INIT_AUTOMAKE
LT_INIT

AC_CONFIG_SRCDIR([hypervisor/pg_cop_hypervisor.c])
AC_CONFIG_SRCDIR([libPGCOP/pg_cop_modules.c])
AC_CONFIG_SRCDIR([modules/pg_cop_mod_socket.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

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
                 hypervisor/Makefile
                 libPGCOP/Makefile
                 modules/Makefile])

AC_OUTPUT

