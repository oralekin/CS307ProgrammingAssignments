#! /bin/sh

# TODO: time runs
find src/ | entr -cns "make run -s input=samples/$1.in | diff samples/$1.out -"