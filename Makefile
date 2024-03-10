CC = gcc
CFLAGS = -Wall -Iinclude -g -Wextra -Werror -Wpedantic
LDFLAGS =

.PHONY: all folders server client install doc valgrind clean

all: folders server client

server: bin/orchestrator

client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: obj/orchestrator.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: obj/client.o
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
	rm -f obj/* tmp/* bin/*
