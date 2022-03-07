CFLAGS = -Wall -g
CC = gcc $(CFLAGS)

commando : cmd.o cmdcol.o util.o commando.o
	$(CC) -o commando cmd.o cmdcol.o util.o commando.o

commando.o : commando.c commando.h
	$(CC) -c commando.c

cmd.o : cmd.c commando.h
	$(CC) -c cmd.c

cmdcol.o : cmdcol.c commando.h
	$(CC) -c cmdcol.c

util.o : util.c commando.h
	$(CC) -c util.c

clean:
	rm -f commando *.o

include test_Makefile