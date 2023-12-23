#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "connmgr.h"


int port;
int max_conn;
void *start_connection_manager(void *arg);
pthread_cond_t readers_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readers_mutex = PTHREAD_MUTEX_INITIALIZER;
int exit_readers = 0;
int exit_storage_manager = 0;



void *storage_manager(void *arg) {
    // Open CSV file for write
    FILE *csv = fopen("sensor_data_out.csv", "w");
    if (csv == NULL) {
        perror("Error opening CSV file");
        pthread_exit(NULL);
    }

    sensor_data_t data;

    while (1) {
        pthread_mutex_lock(&readers_mutex);

        // Process data in the buffer
        while (sbuffer_size(shared_buffer) > 0) {
            if (sbuffer_remove(shared_buffer, &data) == SBUFFER_SUCCESS) {
                // Insert data into the CSV file
                fprintf(csv, "%d,%lf,%lu\n", data.id, data.value, data.ts);
            }
        }

        pthread_mutex_unlock(&readers_mutex);

        // Check if it's time to exit
        if (exit_storage_manager) {
            break;
        }

        usleep(25000);  // Sleep for 25 microseconds
    }

    fclose(csv);  // Close CSV file
    pthread_exit(NULL);
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
    // Set the exit_storage_manager flag after connection_manager has finished
    pthread_mutex_lock(&readers_mutex);
    exit_storage_manager = 1;
    pthread_mutex_unlock(&readers_mutex);
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

    sbuffer_free(&shared_buffer);

    return 0;
}


void *start_connection_manager(void *arg) {
    // Start the server and connection manager
    int result = connmgrMain(port, max_conn, shared_buffer);

    pthread_mutex_lock(&readers_mutex);
    if(result ==0){ // this is if statement is just to make sure the connmgrMain is really finished
        exit_readers = 1;
    }
    pthread_cond_signal(&readers_cond);
    pthread_mutex_unlock(&readers_mutex);

    return NULL; // No pthread_exit here
}





