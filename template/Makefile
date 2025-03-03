.PHONY: all build debug

all: build
build: build/app
debug: build/debug

build/app: src/app.c
	gcc -o build/app src/app.c -Wall

build/debug: src/app.c
	gcc -o build/debug src/app.c -Wall -g
