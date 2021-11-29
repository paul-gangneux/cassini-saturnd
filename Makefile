CC = gcc
OPTS = -Wextra -Wall -pedantic

HEADERS_PATH = ./include
SRC_PATH = ./src

headers = $(HEADERS_PATH)/custom-string.h $(HEADERS_PATH)/client-request.h

.PHONY: all
all: cassini

.PHONY: distclean
distclean:
	rm -rf run/ build/ cassini saturnd -v

cassini: src/cassini.c $(headers) $(SRC_PATH)/custom-string.c
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/custom-string.c $(SRC_PATH)/cassini.c -o cassini

.PHONY: test
test: cassini
	./run-cassini-tests.sh

.PHONY: killtests
killtests:
	killall -v run-cassini-tests.sh*

.PHONY: test2
test2: cassini
	 ./cassini -p ./run/pipes -c echo hello

