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
pthread_cond_t readers_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readers_mutex = PTHREAD_MUTEX_INITIALIZER;
int exit_readers = 0;



void *storage_manager(void *arg) { //func for the reader thread
    printf("storage manager startup\n");
    sbuffer_t *buffer = (sbuffer_t *)arg;
    sensor_data_t data;

    while (1) {
        //printf("entering while loop for write\n");
        pthread_mutex_lock(&readers_mutex);
        while (!exit_readers && sbuffer_remove(buffer, &data) != SBUFFER_SUCCESS) {
            pthread_cond_wait(&readers_cond, &readers_mutex);
        }
        pthread_mutex_unlock(&readers_mutex);
        if (exit_readers) {
            printf("exit\n");
            break;
        }
        if (data.id == 0 && data.value == 0 && data.ts == 0) {
            printf("no data\n");
            break;
        }
        if(fprintf(output_file, "%d,%lf,%lu\n", data.id, data.value, data.ts)<0){
            printf("error while writing data to file\n");
        }
        //printf("Data written to CSV: id=%d, value=%lf, timestamp=%lu\n", data.id, data.value, data.ts);

        usleep(25000);
    }

    return NULL;
}


int main(int argc, char* argv[]) {
    pthread_t reader1, connection_manager;
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
    pthread_create(&reader1, NULL, storage_manager, shared_buffer);

    //Set the exit_readers flag
    /*the problem is in this block of code, because here the exit_readers gets
     * set to 1 when the reader is only just running
     * this makes it so that the reader immediately exits.*/
    pthread_mutex_lock(&readers_mutex);
    exit_readers = 1;
    pthread_cond_signal(&readers_cond);
    pthread_mutex_unlock(&readers_mutex);

    // Wait for all threads to finish
    pthread_join(connection_manager, NULL);
    pthread_join(reader1, NULL);


    printf("Final buffer size: %d\n", sbuffer_size(shared_buffer));


    FILE *csv_check = fopen("sensor_data_out.csv", "r");
    if (csv_check != NULL) {
        fseek(csv_check, 0, SEEK_END);
        long size = ftell(csv_check);
        fclose(csv_check);

        if (size > 0) {
            printf("CSV file is not empty. Size: %ld bytes.\n", size);
        } else {
            printf("CSV file is empty.\n");
        }
    } else {
        printf("Error opening CSV file for checking.\n");
    }

    fclose(output_file);
    sbuffer_free(&shared_buffer);

    return 0;
}


void *start_connection_manager(void *arg) {
    // Start the server and connection manager
    connmgrMain(port, max_conn, shared_buffer);

    pthread_mutex_lock(&readers_mutex);
    exit_readers = 1;
    pthread_cond_signal(&readers_cond);
    pthread_mutex_unlock(&readers_mutex);

    return NULL; // No pthread_exit here
}





