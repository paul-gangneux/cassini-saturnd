CC = gcc
OPTS = -Wextra -Wall -pedantic

HEADERS_PATH = ./include
SRC_PATH = ./src

cassini_headers = $(HEADERS_PATH)/commandline.h $(HEADERS_PATH)/custom-string.h $(HEADERS_PATH)/client-request.h $(HEADERS_PATH)/cassini.h
cassini_source = $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/custom-string.c $(SRC_PATH)/commandline.c $(SRC_PATH)/cassini.c

saturnd_headers = $(HEADERS_PATH)/commandline.h $(HEADERS_PATH)/custom-string.h $(HEADERS_PATH)/timing.h $(HEADERS_PATH)/client-request.h $(HEADERS_PATH)/tasklist.h $(HEADERS_PATH)/saturnd.h
saturnd_source = $(SRC_PATH)/timing-text-io.c $(SRC_PATH)/custom-string.c $(SRC_PATH)/commandline.c $(SRC_PATH)/tasklist.c $(SRC_PATH)/saturnd.c

.PHONY: all
all: cassini saturnd

.PHONY: distclean
distclean:
	rm -rf run/ build/ cassini saturnd cassini-debug -v

cassini: $(cassini_headers) $(cassini_source)
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(cassini_source) -o cassini

saturnd: $(saturnd_headers) $(saturnd_source)
	$(CC) $(OPTS) -I$(HEADERS_PATH) $(saturnd_source) -o saturnd

saturnd-debug: $(saturnd_headers) $(saturnd_source)
	$(CC) $(OPTS) -g -DDEBUG -I$(HEADERS_PATH) $(saturnd_source) -o saturnd
	gdb ./saturnd

.PHONY: test
test: cassini
	./run-cassini-tests.sh

.PHONY: killtests
killtests:
	killall -v run-cassini-tests.sh*

.PHONY: killsaturnd
killsaturnd:
	killall -v saturnd

.PHONY: test2
test2: cassini
	 ./cassini -p ./run/pipes -c echo hello

cassini-debug: $(cassini_headers) $(cassini_source)
	$(CC) -g $(OPTS) -I$(HEADERS_PATH) $(cassini_source) -o cassini-debug

debug: cassini-debug
	gdb ./cassini-debug -p ./run/pipes -c echo hello
