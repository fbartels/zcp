# -*- Makefile -*-

AM_CPPFLAGS = ${ZCPPFLAGS} ${DEBUGFLAGS}

lib_LTLIBRARIES = librosie.la
noinst_DATA = librosie.ldd

include_HEADERS = librosie.h
librosie_la_SOURCES = librosie.cpp
librosie_la_LIBADD  = -ltidy
librosie_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	-version-info 0:0:0 -Wl,--version-script=${top_builddir}/default.sym

include ../global.am
