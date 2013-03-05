#!/bin/sh

aclocal && \
autoheader && \
libtoolize -c && \
automake -a -c && \
autoconf
