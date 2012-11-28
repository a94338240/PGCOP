lib_LTLIBRARIES = libPGCOP.la
libPGCOP_la_SOURCES = pg_cop_util.c \
		                  pg_cop_util.h \
				              pg_cop_modules.c \
                      pg_cop_modules.h \
                      pg_cop_debug.h \
	                    pg_cop_hooks.c \
                      pg_cop_hooks.h \
                      pg_cop_rodata_strings.c \
                      pg_cop_rodata_strings.h \
				              pg_cop_config.h \
                      pg_cop_config.c

libPGCOP_la_CFLAGS = $(PGCOP_DEPS_CFLAGS)
libPGCOP_la_LDFLAGS = $(PGCOP_DEPS_LIBS) -lpthread -ldl

install-data-hook:
	if [ ! -f /etc/pgcop_conf.lua ]; then \
		echo "pgcop = {}" > /etc/pgcop_conf.lua ; \
		echo "pgcop.modules_path = \"$(datadir)/pgcop/modules\"" >> /etc/pgcop_conf.lua ; \
	fi
