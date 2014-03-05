all: adhocify.c
	$(CC) adhocify.c -g -std=c99 -Wall -Wextra -pedantic -o adhocify
