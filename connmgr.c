/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "lib/tcpsock.h"
#include "pthread.h"
#include "config.h"

struct ThreadArgs {
    pthread_mutex_t *mutex;
    tcpsock_t *client;
};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleConnection(void *argument) {
    struct ThreadArgs *args = (struct ThreadArgs *)argument;
    tcpsock_t *client = args->client;
    int bytes, result;
    sensor_data_t data;

    pthread_mutex_lock(args->mutex);

    do {
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *)&data.id, &bytes);

        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *)&data.value, &bytes);

        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *)&data.ts, &bytes);

        if ((result == TCP_NO_ERROR) && bytes) {
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                    (long int)data.ts);
        }
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occurred on connection to peer\n");

    tcp_close(&client);
    pthread_mutex_unlock(args->mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    sensor_data_t data;
    int bytes, result;
    int conn_counter = 0;

    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    if (argc < 3) {
        printf("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }
    printf("Server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR)
        exit(EXIT_FAILURE);
    while (conn_counter < MAX_CONN)
    {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");
        conn_counter++;
        pthread_t thread_id;
        struct ThreadArgs args;
        args.mutex = &mutex;
        args.client = client;
        result = pthread_create(&thread_id, NULL, handleConnection, (void *)&args);
        if (result != 0) {
            fprintf(stderr, "Error while making stream!");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);

    }

    if (tcp_close(&server) != TCP_NO_ERROR)
        exit(EXIT_FAILURE);

    printf("Test server is shutting down\n");
    pthread_mutex_destroy(&mutex);
    return 0;
}