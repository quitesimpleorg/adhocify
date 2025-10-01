prefix = /usr/local
bindir = $(prefix)/bin
CFLAGS = -std=c99 -Wall -Wextra -pedantic

VERSIONFALLBACK = "v1.2+"
VERSIONFLAGS = -DGIT_TAG=\"$(shell git describe --tags HEAD || echo $(VERSIONFALLBACK))\"

all:
	$(CC) adhocify.c -g $(CFLAGS) $(VERSIONFLAGS)  -o adhocify

release:
	$(CC) adhocify.c $(CFLAGS) $(VERSIONFLAGS) -o adhocify

install: release
	install -D adhocify $(DESTDIR)$(bindir)/adhocify

.PHONY: install
