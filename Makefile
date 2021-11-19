CC = gcc
OPTS = -Werror -Wextra

HEADERS_PATH = ./include
SRC_PATH = ./src

all: cassini saturnd
.PHONY: all

distclean:
	rm -rf build/ cassini saturnd

cassini:
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/cassini.c -o cassini
