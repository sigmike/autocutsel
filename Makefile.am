
bin_PROGRAMS = autocutsel cutsel selsync
lib_LTLIBRARIES = libselsync.la

autocutsel_SOURCES = autocutsel.c common.c
cutsel_SOURCES = cutsel.c common.c
selsync_SOURCES = selsync.c
selsync_LDADD = -lselsync
libselsync_la_SOURCES = libselsync.c

noinst_HEADERS = common.h libselsync.h

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO test_autocutsel.rb selsync_test.rb
