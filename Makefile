CC = gcc
OPTS = -Wextra -Wall -pedantic

HEADERS_PATH = ./include
SRC_PATH = ./src

headers = $(HEADERS_PATH)/custom-string.h $(HEADERS_PATH)/client-request.h

.PHONY: all
all: cassini saturnd

.PHONY: distclean
distclean:
	rm -rf run/ build/ cassini saturnd cassini-debug -v

cassini: src/cassini.c $(headers) $(SRC_PATH)/custom-string.c
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/custom-string.c $(SRC_PATH)/cassini.c -o cassini

saturnd: distclean
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/saturnd.c $(SRC_PATH)/tasklist.c -o saturnd

.PHONY: test
test: cassini
	./run-cassini-tests.sh

.PHONY: killtests
killtests:
	killall -v run-cassini-tests.sh*

.PHONY: test2
test2: cassini
	 ./cassini -p ./run/pipes -c echo hello

cassini-debug: src/cassini.c $(headers) $(SRC_PATH)/custom-string.c
	$(CC) -g $(OPTS) -I$(HEADERS_PATH) $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/custom-string.c $(SRC_PATH)/cassini.c -o cassini-debug

debug: cassini-debug
	gdb ./cassini-debug -p ./run/pipes -c echo hello
