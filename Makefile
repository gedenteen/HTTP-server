COMP = gcc
FLAGS = -Wall -g -o
OBJECTS = src/main.c src/server.c

# TODO .PHONY

all:
	$(COMP) $(OBJECTS) $(FLAGS) main.bin

# TODO: clean
