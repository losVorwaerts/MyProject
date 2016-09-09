.PHONY:all clean cleano

WORKDIR=.
VPATH=./src

CC=gcc
CFLAGS=-Wall -I$(WORKDIR)/inc/ -I/usr/local/include/hiredis/
LIBS=-lhiredis -lfcgi -lm

#BIN=upload hiredis_test redis_test demo echo_test 
BIN=load data_cgi

all:$(BIN)

upload:upload.o make_log.o
	$(CC) $^ -o $@
hiredis_test:hiredis_test.o
	$(CC) $^ -o $@ $(LIBS)
redis_test:redis_test.o redis_op.o make_log.o 
	$(CC) $^ -o $@ $(LIBS)
demo:fcgi_demo.o
	$(CC) $^ -o $@ $(LIBS)
echo_test:echo.o
	$(CC) $^ -o $@ $(LIBS)
load:load.o make_log.o upload.o redis_op.o
	$(CC) $^ -o $@ $(LIBS)
data_cgi:data_cgi.o make_log.o redis_op.o cJSON.o
	$(CC) $^ -o $@ $(LIBS)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

clean:
	-rm -f *.o $(BIN)
cleano:
	-rm -f *.o
