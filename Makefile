all: adhocify.c
	gcc adhocify.c -std=c99 -Wall -Wextra -pedantic -o adhocify
