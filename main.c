#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "connmgr.h"

FILE *output_file;
int port;
int max_conn;
void *start_connection_manager(void *arg);

void *reader_thread(void *arg) { //func for the reader thread
    sbuffer_t *buffer = (sbuffer_t *)arg;
    sensor_data_t data;
    while (1) {
        if (sbuffer_remove(buffer, &data) == SBUFFER_SUCCESS) { //
            if (data.id == 0 && data.value == 0 && data.ts == 0) {
                break;
            }
            fprintf(output_file, "%d,%lf,%lu\n", data.id, data.value, data.ts);
            usleep(25000); //tjis is in microsec
        }
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    pthread_t reader1, reader2, connection_manager;
    port = atoi(argv[1]);
    max_conn = atoi(argv[2]);
    //check if the right arguments are filled in
    if(argc < 3) {
        printf("please fill in port number and the maximum connections!");
        return -1;
    }
    // Initialize shared buffer
    if (sbuffer_init(&shared_buffer) != SBUFFER_SUCCESS) {
        perror("Error initializing shared buffer");
        exit(EXIT_FAILURE);
    }

    // Open output file
    output_file = fopen("sensor_data_out.csv", "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    // Create threads for readers and writer
    pthread_create(&connection_manager, NULL, start_connection_manager, NULL);
    pthread_create(&reader1, NULL, reader_thread, shared_buffer);
    pthread_create(&reader2, NULL, reader_thread, shared_buffer);


    // Wait for all threads to finish
    pthread_join(connection_manager, NULL);
    pthread_join(reader1, NULL);
    pthread_join(reader2, NULL);

    fclose(output_file);
    sbuffer_free(&shared_buffer);

    return 0;
}


void *start_connection_manager(void *arg){
    // Start the server and connection manager
    connmgrMain(port, max_conn, shared_buffer);
    pthread_exit(NULL);
}




