CC = gcc
CFLAGS = -g -Wall -pthread

all: sensor_node main

run: runserver

clean:
	rm sensor_node main

sensor_node: sensor_node.c lib/tcpsock.c sbuffer.c
	$(CC) $(CFLAGS) $^ -o $@

main: main.c lib/tcpsock.c sbuffer.c connmgr.c datamgr.c  # Include datamgr.c
	$(CC) $(CFLAGS) $^ -o $@

runserver: main
	./main 5678 3

runclient1: sensor_node
	./sensor_node 1 2 127.0.0.1 5678

runclient2: sensor_node
	./sensor_node 2 2 127.0.0.1 5678

runclient3: sensor_node
	./sensor_node 3 2 127.0.0.1 5678

zip:
	zip lastMilestone.zip *.c *.h



