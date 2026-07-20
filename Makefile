main:
	clang src/main.c -g -rdynamic -fsanitize=address -std=c23 -Wall -o bin/main
