.PHONY:all clean cleano

WORKDIR=.
VPATH=./src

CC=gcc
CFLAGS=-Wall -I$(WORKDIR)/inc/
#LIBS

BIN=upload

all:$(BIN)

upload:upload.o make_log.o
	$(CC) $^ -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(BIN)
cleano:
	-rm -f *.o
