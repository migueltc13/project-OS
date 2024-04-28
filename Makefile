CC = gcc
CFLAGS = -Wall -Iinclude -g -Wextra -Werror -Wpedantic
LDFLAGS =

SOURCES = $(filter-out src/orchestrator.c src/client.c, $(wildcard src/*.c))
OBJECTS = $(SOURCES:src/%.c=obj/%.o)

.PHONY: all folders server client install doc valgrind clean

all: folders server client

server: bin/orchestrator

client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: $(OBJECTS) obj/orchestrator.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: $(OBJECTS) obj/client.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install:
	@echo "Installing development requirements"
	sudo apt install -y doxygen graphviz valgrind gdb

doc:
	@echo "Generating documentation"
	doxygen Doxyfile

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes bin/orchestrator tmp 4

clean:
	rm -rf obj/* tmp/* bin/*
