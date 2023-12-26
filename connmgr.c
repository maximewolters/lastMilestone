#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "lib/tcpsock.h"
#include "pthread.h"
#include "config.h"
#include "sbuffer.h"


int bytes, result;
sensor_data_t data;
pthread_mutex_t mutex;
int i = 0;
int conn_counter = 0;
int disconnected_clients = 0;
sbuffer_t *shared_buffer;
int first_insertion = 1;


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
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,data.ts);
            if(first_insertion == 1)
            {
                printf("inserting first set of data into shared buffer\n");
                if(sbuffer_insert(shared_buffer, &data) == SBUFFER_SUCCESS)
                {
                    printf("insertion of data into buffer successful\n");
                    first_insertion = 0;
                    pthread_cond_signal(&condition_buffer);
                }
            }
            else{
                sbuffer_insert(shared_buffer, &data);
                pthread_cond_signal(&condition_buffer);
            }
        }
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED){
        printf("Peer has closed connection\n");
        disconnected_clients++;}
    else
        printf("Error occurred on connection to peer\n");
    tcp_close(&client);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

int connmgrMain(int port, int max_conn, sbuffer_t *sbuffer) {
    tcpsock_t *server, *client;
    int MAX_CONN = max_conn;
    int PORT = port;
    pthread_t thread_ids[MAX_CONN];
    shared_buffer = sbuffer;
    pthread_mutex_init(&mutex, NULL);

    printf("Server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) {
        printf("error while opening server\n");
        exit(EXIT_FAILURE);
    }
    do {
        printf("waiting for client to connect\n");
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

    while (disconnected_clients < MAX_CONN) {
        pthread_join(thread_ids[i], NULL);
        i++;
        if (i >= MAX_CONN) {
            i = 0;
        }
    }

    //pthread_mutex_lock(&mutex);
    if (disconnected_clients >= MAX_CONN) {
        if (tcp_close(&server) != TCP_NO_ERROR)
            exit(EXIT_FAILURE);
        printf("server is shutting down\n");

        //pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        return 0;
    }
    return 0;
}

