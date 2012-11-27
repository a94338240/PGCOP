noinst_LTLIBRARIES = mod_socket.la mod_transceiver.la

mod_socket_la_SOURCES = pg_cop_mod_socket.c
mod_socket_la_LIBADD = ../libPGCOP/libPGCOP.la
mod_socket_la_LDFLAGS = -avoid-version -module -rpath /nowhere
mod_socket_la_CFLAGS = -I ../libPGCOP

mod_transceiver_la_SOURCES = pg_cop_mod_transceiver.c
mod_transceiver_la_LIBADD = ../libPGCOP/libPGCOP.la
mod_transceiver_la_LDFLAGS = -avoid-version -module -rpath /nowhere
mod_transceiver_la_CFLAGS = -I ../libPGCOP

install-data-hook:
	mkdir -p $(datadir)/pgcop/modules
	cp -L .libs/*.so $(datadir)/pgcop/modules
	if [ ! -f /etc/pgcop_conf.lua ]; then \
		echo "pgcop = {}" > /etc/pgcop_conf.lua ; \
		echo "pgcop.modules_path = \"$(datadir)/pgcop/modules\"" >> /etc/pgcop_conf.lua ; \
	fi

