#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "lib/tcpsock.h"
#include "pthread.h"
#include "config.h"

int bytes, result;
sensor_data_t data;
pthread_mutex_t mutex;
int i = 0;
int conn_counter = 0;
int disconnected_clients = 0;

void *handleConnection(void *arg) {
    tcpsock_t *client = *((tcpsock_t **)arg);
    pthread_mutex_lock(&mutex);
    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *)&data.id, &bytes);
        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *)&data.value, &bytes);
        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *)&data.ts, &bytes);

        if ((result == TCP_NO_ERROR) && bytes) {
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                   (long int)data.ts);
        }
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED){
        printf("Peer has closed connection\n");
        disconnected_clients++;}
    else
        printf("Error occurred on connection to peer\n");
    tcp_close(&client);
    printf("client is now closed\n");
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    printf("main func start\n");
    tcpsock_t *server, *client;
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);
    pthread_t thread_ids[MAX_CONN];
    pthread_mutex_init(&mutex, NULL);

    if (argc < 3) {
        printf("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }
    printf("Server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR)
        exit(EXIT_FAILURE);
    do {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        conn_counter++;
        printf("Incoming client connection\n");

        // Allocate memory for the client socket pointer
        tcpsock_t **client_ptr = malloc(sizeof(tcpsock_t *));
        *client_ptr = client;

        result = pthread_create(&thread_ids[conn_counter - 1], NULL, handleConnection, client_ptr);
        if (result != 0) {
            fprintf(stderr, "Error while making stream!");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_ids[conn_counter - 1]);

    } while (conn_counter < MAX_CONN);

    // Separate loop to check disconnected clients and wait for threads
    while (disconnected_clients < MAX_CONN) {
        pthread_join(thread_ids[i], NULL);
        i++;
        if (i >= MAX_CONN) {
            i = 0;  // Reset i to 0 to avoid exceeding array bounds
        }
    }



    pthread_mutex_lock(&mutex);  // Lock to ensure no new connections
    if (disconnected_clients >= MAX_CONN) {
        printf("server side socket will get closed\n");
        if (tcp_close(&server) != TCP_NO_ERROR)
            exit(EXIT_FAILURE);
        printf("Test server is shutting down\n");

        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        return 0;
    }
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
    return 0;



}