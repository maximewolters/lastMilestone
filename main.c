#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "datamgr.h"
#include "connmgr.h"
#include "dplist.h"

#define SET_MIN_TEMP 10.0
#define SET_MAX_TEMP 15.0

int port;
int max_conn;

void *start_connection_manager(void *arg);
void *start_data_manager(void *arg);
void *storage_manager(void *arg);

pthread_cond_t condition_buffer = PTHREAD_COND_INITIALIZER;

int exit_readers = 0;
int exit_storage_and_data_manager = 0;
int read_by_data_manager = 0;

int main(int argc, char* argv[]) {
    printf("main func startup\n");
    //initialising three different threads, together they form the 'server gateway'
    pthread_t reader1, connection_manager, data_manager;
    //initialising the port and max_conn that get passed by the arguments
    port = atoi(argv[1]);
    max_conn = atoi(argv[2]);
    //check if the right arguments are filled in
    if(argc < 3) {
        printf("please fill in port number and the maximum connections!");
        return -1;
    }
    // Initialize shared buffer where sensor data will be stored until they get removed by the storage manager
    if (sbuffer_init(&shared_buffer) != SBUFFER_SUCCESS) {
        perror("Error initializing shared buffer");
        exit(EXIT_FAILURE);
    }


    // Create threads
    pthread_create(&connection_manager, NULL, start_connection_manager, NULL);
    pthread_create(&data_manager, NULL, start_data_manager, NULL);
    pthread_create(&reader1, NULL, storage_manager, shared_buffer);
    printf("all threads created\n");


    // Wait for all threads to finish by join
    pthread_join(connection_manager, NULL);

    //Set the exit_storage_manager flag after connection_manager has finished, this ensures that the storage manager can finish too
    pthread_mutex_lock(&shared_buffer->mutex);
    exit_storage_and_data_manager = 1;
    update_exit();
    pthread_mutex_unlock(&shared_buffer->mutex);

    pthread_join(reader1, NULL);
    pthread_join(data_manager, NULL);

    //quick check if the buffer is empty at the end
    printf("Final buffer size: %d\n", sbuffer_size(shared_buffer));

    //quick check if csv file actually got filled
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
    //the memory allocated by the buffer gets freed
    sbuffer_free(&shared_buffer);

    return 0;
}


void *start_connection_manager(void *arg) {
    /*this function starts the connection manager, which is the starting point of the server.
     * after the server has started, the sensor nodes aka clients can connect to it. If a client connects,
     * the data that it sends to the server gets put in the shared buffer so that it can be accessed by the other
     * threads. The server shuts down when the MAX_CONN clients has disconnected. */
    printf("connection manager startup\n");
    // Start the server
    connmgrMain(port, max_conn, shared_buffer, condition_buffer);
    pthread_cond_signal(&condition_buffer);
    printf("connection manager shut down\n");
    return NULL; // No pthread_exit here
}

void *start_data_manager(void *arg) {
    printf("data manager startup\n");
    // Initialize the linked list
    SensorList *sensorList = SensorList_create();
    // Initialize minimum and maximum temperature values
    double minTemperature = SET_MIN_TEMP;
    double maxTemperature = SET_MAX_TEMP;
    //open the file out of which the datamanager should read the room id's and the sensor id's
    FILE *map = fopen("room_sensor.map", "r");
    // Fill the linked list with nodes from room_sensor.map
    fill_list_from_file("room_sensor.map", sensorList);
    printf("sensor list is filled with initial values\n");
    // Update nodes with sensor data from the shared buffer data
    update_sensor_data_from_buffer(shared_buffer, minTemperature, maxTemperature, sensorList, shared_buffer->mutex);

    // Print the contents of the linked list to test if main func works, preprocessor directives are not yet used
    SensorNode *current = sensorList->head;
    while (current != NULL) {
        printf("Room ID: %u, Sensor ID: %u, Average Temperature: %f, Last Modified: %ld\n",
               current->roomID, current->sensorID, current->average,
               current->lastModified);
        current = current->next;
    }

    SensorList_free(sensorList);
    fclose(map);

    return 0;

}

void *storage_manager(void *arg) {
    printf("storage manager startup\n");
    // Open CSV file to write
    FILE *csv = fopen("sensor_data_out.csv", "w");
    if (csv == NULL) {
        perror("Error opening CSV file");
        pthread_exit(NULL);
    }
    /*in this while loop, there is another while loop that checks if the buffer is filled and then removes the
    * data from the buffer. If that is a success, that data gets put in the csv file. if the buffer is empty the
    * storage manager checks if the exit store manager flag is set and if it is it breaks out of the loop
    * and the program exits. Else, it will just wait 25 micro sec before it removes and writes the next set of
    * data.*/
    //run the remover function
    pthread_mutex_lock(&shared_buffer->mutex);
    sensor_data_t data;
    while(sbuffer_size(shared_buffer) == 0)
    {
        printf("storage manager waiting for data\n");
        pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);
    }
    pthread_mutex_unlock(&shared_buffer->mutex);
    //pthread_cond_signal(&condition_buffer);
    // Process data in the buffer
    sbuffer_node_t *buffer_node;
    sbuffer_node_t *next_node;
    buffer_node = shared_buffer->head;
    while (1) {
        if(buffer_node == NULL)
        {
            //if the buffer node is null->wait
            printf("ERROR");
            pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);
            buffer_node = shared_buffer->head;
        }
        if(exit_storage_and_data_manager == 1)
        {
            break;
        }
        pthread_mutex_lock(&shared_buffer->mutex);
        if(buffer_node->next == NULL)
        {
            next_node = buffer_node;
        }
        else
        {
            next_node = buffer_node->next;
        }
        data = *buffer_node->data;
        if(sbuffer_size(shared_buffer) > 0)
        {
            // Insert data into the CSV file
            fprintf(csv, "%d,%lf,%lu\n", data.id, data.value, data.ts);
            buffer_node->read_by_storage_manager = 1;
            printf("storageloop\n");
            buffer_node = next_node;
            //pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);

        }
        if(buffer_node->read_by_storage_manager == 1 && buffer_node->read_by_data_manager == 1){
            if (sbuffer_size(shared_buffer) > 2) {
                sbuffer_remove(shared_buffer, buffer_node->data);
                printf("data removed\n");
            }
        }
        pthread_mutex_unlock(&shared_buffer->mutex);
        usleep(250000);  // Sleep for 25 microseconds

    }
    printf("storage manager shutting down\n");
    sbuffer_free(&shared_buffer);
    fclose(csv);  // Close CSV file
    pthread_exit(NULL);
    return NULL;
}




