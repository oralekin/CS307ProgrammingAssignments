all: left right treePipe

debug: left right tpDebug

treePipe: treePipe.c
	gcc -o build/treePipe treePipe.c -Wall

tpDebug: treePipe.c
	gcc -o build/treePipe.debug treePipe.c -Wall -g

left: pl.c
	gcc -o build/left pl.c -Wall
right: pr.c
	gcc -o build/right pr.c -Wall