CFLAGS = -Wall -g
CC     = gcc $(CFLAGS)

PROGRAMS = bl_server bl_client
OBJECTS = util.o server_funcs.o simpio.o

all : $(PROGRAMS)

%.o : %.c blather.h
	$(CC) -c $<


bl_server : $(OBJECTS) bl_server.o
	$(CC) -o $@ $^ -lpthread

bl_client : $(OBJECTS) bl_client.o
	$(CC) -o $@ $^ -lpthread

clean :
	rm -f *.o *.fifo $(PROGRAMS)

include test_Makefile