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

void *handleConnection(void *argument){
    tcpsock_t *client = (tcpsock_t *)argument;
    int bytes, result;
    sensor_data_t data;
    printf("Incoming client connection\n");
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
    } while (result == TCP_NO_ERROR);
    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occurred on connection to peer\n");
    tcp_close(&client);
    return NULL;


}

int serverStartup(int argc, char *argv[]){
    tcpsock_t *server, *client;
    sensor_data_t data;
    int bytes, result;
    int conn_counter = 0;

    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    if(argc < 3) {
        printf("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }
    //server wordt opgestart en is klaar om te luisteren
    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    while(conn_counter < MAX_CONN)
    {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");
        conn_counter++; //increment counter tot max connecties bereikt wordt
        //nieuwe thread maken voor elke nieuwe connectie
        pthread_t thread_id;
        result = pthread_create(&thread_id, NULL, handleConnection((void *)client), NULL); //waarom 4 args als client arg ook in de startroutine kan meegegeven worden?
        //checke of res ok is
        if(result != 0)
        {
            fprintf(stderr, "error while making stream!");
            exit(EXIT_FAILURE);
        }
        //detach thread om resources vrij te maken nadat thread gestopt wordt
        pthread_detach(thread_id);
    }
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    printf("Test server is shutting down\n");
    return 0;


}