prefix = /usr/local
bindir = $(prefix)/bin
CFLAGS = -std=c99 -Wall -Wextra -pedantic
all:
	$(CC) adhocify.c -g $(CFLAGS) -o adhocify

release:
	$(CC) adhocify.c $(CFLAGS) -o adhocify

install: release
	install -D adhocify $(DESTDIR)$(bindir)/adhocify

.PHONY: install
