CC = gcc
OPTS = -Wextra -Wall -pedantic

HEADERS_PATH = ./include
SRC_PATH = ./src

all: cassini saturnd
.PHONY: all

.PHONY: distclean
distclean:
	rm -rf build/ cassini saturnd

cassini: src/cassini.c
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/cassini.c -o cassini

saturnd: src/saturnd.c
	$(CC) $(OPTS) $(SRC_PATH)/saturnd.c -o saturnd
