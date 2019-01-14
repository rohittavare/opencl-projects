CC = gcc
FLAGS = -g -Wall -Wextra -l OpenCL

default: clean build

clean: 
	rm -f vector_add

build: 
	$(CC) host.c -o vector_add $(FLAGS)
