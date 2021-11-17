CC = gcc
OPTS = -Wall -Werror -Wextra

HEADERS_PATH = ./include
SRC_PATH = ./src

all: cassini saturnd
.PHONY: all

distclean:
	rm -rf build/ cassini saturnd

cassini:
	$(CC) $(OPTS) $(HEADERS_PATH)/cassini.h $(SRC_PATH)/cassini.c -o cassini
