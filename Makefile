CC = gcc
OPTS = -Wextra -Wall -pedantic

HEADERS_PATH = ./include
SRC_PATH = ./src

.PHONY: all
all: cassini

.PHONY: distclean
distclean:
	rm -rf run/ build/ cassini saturnd

cassini: src/cassini.c
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/cassini.c -o cassini
