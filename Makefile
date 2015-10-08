CC=gcc
CFLAGS= -Wall -g
CFLAGS2= -pthread -o

all: rw dp ds

rw: serial.c rw_serial.c rw.c
	$(CC) $(CFLAGS) $(CFLAGS2) rw serial.c rw_serial.c rw.c

dp: serial.c dp_serial.c dp.c
	$(CC) $(CFLAGS) $(CFLAGS2) dp serial.c dp_serial.c dp.c

ds: serial.c ds_serial.c ds.c
	$(CC) $(CFLAGS) $(CFLAGS2) ds serial.c ds_serial.c ds.c 

clean:
	rm *.o rw dp ds
