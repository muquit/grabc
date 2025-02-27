bin_PROGRAMS = grabc

grabc_SOURCES = grabc.c

grabc_CFLAGS = $(X_CFLAGS)
grabc_LDADD = $(X_LIBS)

EXTRA_DIST = \
	grabc.pod

man_MANS = grabc.1

grabc.1: grabc.pod
	pod2man --date="2018-04-16" < $< > $@
