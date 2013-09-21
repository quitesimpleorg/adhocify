all: adhocify.c
	gcc adhocify.c -g -std=c99 -Wall -Wextra -pedantic -o adhocify
