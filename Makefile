CC=gcc
SRC_DIR=src/
OUT_DIR=bin/
LIBS=-lm
PROG=cely
CFLAGS=-fPIC -Os
CFLAGS_DEBUG=-fPIC -O0 -g -Wall

all:
	mkdir $(OUT_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c -o $(OUT_DIR)/$(PROG)

clean:
	rm -rf $(OUT_DIR)
