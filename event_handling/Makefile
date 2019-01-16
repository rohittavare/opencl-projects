CC = gcc
FLAGS = -g -Wall -Wextra -l OpenCL

default: clean build

build:
	$(CC) host.c -o event_handling $(FLAGS)

clean:
	rm -f event_handling
