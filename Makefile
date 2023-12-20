CC = gcc
CFLAGS = -g -Wall -pthread

all: sensor_node connmgr

run: runserver

clean:
	rm sensor_node connmgr

sensor_node: sensor_node.c lib/tcpsock.c
	$(CC) $(CFLAGS) $^ -o $@

connmgr: connmgr.c lib/tcpsock.c
	$(CC) $(CFLAGS) $^ -o $@

runserver: connmgr
	./connmgr 5678 3

runclient1: sensor_node
	./sensor_node 1 2 127.0.0.1 5678

runclient2: sensor_node
	./sensor_node 2 5 127.0.0.1 5678

zip:
	zip lastMilestone.zip *.c *.h

