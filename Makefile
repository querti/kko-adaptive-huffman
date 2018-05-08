all: main
main:
		gcc -O2 -Wextra -Wall -pedantic ahed.c main.c -o ahed
debug:
		gcc -ggdb3 ahed.c main.c -o ahed


