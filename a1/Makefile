all: build/left build/right treePipe

debug: build/left build/right build/treePipe.debug

build/treePipe: treePipe.c
	gcc -o build/treePipe treePipe.c -Wall

build/treePipe.debug: treePipe.c
	gcc -o build/treePipe.debug treePipe.c -Wall -g

build/left: pl.c
	gcc -o build/left pl.c -Wall
build/right: pr.c
	gcc -o build/right pr.c -Wall