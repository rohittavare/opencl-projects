CC = gcc
FLAGS = -g -Wall -Wextra -l OpenCL

default: clean build

clean:
	rm -f matrix_mult

build:
	$(CC) host.c -o matrix_mult $(FLAGS)
