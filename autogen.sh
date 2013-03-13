#!/bin/sh

aclocal && \
autoheader && \
libtoolize -c && \
automake -a -c && \
autoconf

chdir ./hypervisor
autogen pg_cop_hypervisor_opts_ag.def
