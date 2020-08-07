prefix = /usr/local
bindir = $(prefix)/bin

all:
	$(CC) adhocify.c -g -std=c99 -Wall -Wextra -pedantic -o adhocify

install: adhocify
	install -D adhocify $(DESTDIR)$(bindir)/adhocify

.PHONY: install
