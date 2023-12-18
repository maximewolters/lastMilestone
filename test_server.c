/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "pthread.h"

/**
 * Implements a sequential test server (only one connection at the same time)
 */

int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    sensor_data_t data;
    int bytes, result;
    int conn_counter = 0;
    
    if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }
    
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    do {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE); //check
        printf("Incoming client connection\n"); //check
        conn_counter++; //check
        do {
            // read sensor ID
            bytes = sizeof(data.id);
            result = tcp_receive(client, (void *) &data.id, &bytes);
            // read temperature
            bytes = sizeof(data.value);
            result = tcp_receive(client, (void *) &data.value, &bytes);
            // read timestamp
            bytes = sizeof(data.ts);
            result = tcp_receive(client, (void *) &data.ts, &bytes);
            if ((result == TCP_NO_ERROR) && bytes) {
                printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                       (long int) data.ts);
            }
        } while (result == TCP_NO_ERROR);//check
        if (result == TCP_CONNECTION_CLOSED)
            printf("Peer has closed connection\n");//check
        else
            printf("Error occured on connection to peer\n");//check
        tcp_close(&client); //check
    } while (conn_counter < MAX_CONN); //check
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE); //check
    printf("Test server is shutting down\n"); //check
    return 0; //check
}




