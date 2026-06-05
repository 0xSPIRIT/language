main:
	gcc src/main.c -g -rdynamic -fsanitize=address -std=c2x -Wall -o bin/main
