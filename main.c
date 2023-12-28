#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "datamgr.h"
#include "connmgr.h"
#include "lib/dplist.h"
#include <string.h>
#include "sensor_db.h"

#define SET_MIN_TEMP 10.0
#define SET_MAX_TEMP 15.0

int port;
int max_conn;

void *start_connection_manager(void *arg);
void *start_data_manager(void *arg);
void *start_storage_manager(void *arg);
void *start_log_process(void *arg);
void write_log_event(FILE *log_file, const char *event);


pthread_cond_t condition_buffer = PTHREAD_COND_INITIALIZER;

int exit_readers = 0;
int exit_storage_and_data_manager = 0;
int read_by_data_manager = 0;

int fd[2]; //file descriptor for the pipe
char buffer_for_pipe[10000]; //10 kB buffer
int sequence_number;


int main(int argc, char* argv[]) {

    //initialising the port and max_conn that get passed by the arguments
    port = atoi(argv[1]);
    max_conn = atoi(argv[2]);

    //check if the right arguments are filled in
    if(argc < 3) {
        printf("please fill in port number and the maximum connections!");
        return -1;
    }

    //initialising three different threads, together they form the 'server gateway'
    pthread_t storage_manager, connection_manager, data_manager;

    // Initialize shared buffer where sensor data will be stored until they get removed by the storage manager
    if (sbuffer_init(&shared_buffer) != SBUFFER_SUCCESS) {
        perror("Error initializing shared buffer");
        exit(EXIT_FAILURE);
    }

    //create pipe
    int create_pipe;
    create_pipe = pipe(fd);
    if(create_pipe != 0)
    {
        printf("error creating pipe\n");
        return -1;
    }

    //fork
    int pid = fork();
    if(pid == -1)
    {
        printf("error creating child process\n");
        return -1;
    }

    //child process -> pid == 0
    if(pid == 0)
    {
        void *start_log_process(void *arg);
    }

    //parent process -> pid != 0
    else
    {
        // Create threads
        int res1 = pthread_create(&connection_manager, NULL, start_connection_manager, NULL);
        int res2 = pthread_create(&data_manager, NULL, start_data_manager, NULL);
        int res3 = pthread_create(&storage_manager, NULL, start_storage_manager, shared_buffer);

        if(res1 != 0 || res2 != 0 || res3 != 0)
        {
            printf("error creating threads\n");
        }

        //Wait for all threads to finish by join
        pthread_join(connection_manager, NULL);

        //Set the exit_storage_manager flag and update exit flag in data manager
        exit_storage_and_data_manager = 1;
        update_exit();

        pthread_join(data_manager, NULL);
        pthread_join(storage_manager, NULL);

        //check if the storage manager freed the shared buffer
        if(sbuffer_size(shared_buffer) > 0)
        {
            printf("buffer not empty\n");
        }

        //check if the storage manager inserted values in sensor_data_out.csv
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
        pthread_mutex_destroy(&shared_buffer->mutex);
        pthread_cond_destroy(&condition_buffer);
        pthread_exit(NULL);
    }
    return 0;

}

void write_to_pipe(char *log_event)
{
    write(fd[0], log_event, strlen(log_event));
}


void *start_log_process(void *arg) {
    // Create log file
    FILE *gateway_log = fopen("gateway_log", "w");
    if (gateway_log == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    close(fd[1]); // Close writing end of the pipe

    // Read from the pipe and write log events to the log file
    while (read(fd[0], buffer_for_pipe, sizeof(buffer_for_pipe)) > 0) {
        char *event = strtok(buffer_for_pipe, "\n");

        while (event != NULL) {
            write_log_event(gateway_log, event);
            event = strtok(NULL, "\n");
        }
    }

    fclose(gateway_log); // Close the log file
    close(fd[0]); // Close reading end of the pipe

    exit(EXIT_SUCCESS);
}

void write_log_event(FILE *log_file, const char *event) {
    // Get current time as string
    time_t mytime = time(NULL);
    char *time_str = ctime(&mytime);
    time_str[strlen(time_str) - 1] = '\0'; // Remove trailing newline from ctime output

    // Write log event to file
    fprintf(log_file, "%d %s %s\n", sequence_number, time_str, event);
    sequence_number++;
}

void *start_connection_manager(void *arg) {
    /*this function starts the connection manager, which is the starting point of the server.
     * after the server has started, the sensor nodes aka clients can connect to it. If a client connects,
     * the data that it sends to the server gets put in the shared buffer so that it can be accessed by the other
     * threads. The server shuts down when the MAX_CONN clients has disconnected. */
    printf("connection manager startup\n");
    // Start the server
    connmgrMain(port, max_conn, shared_buffer, condition_buffer);
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

void *start_storage_manager(void *arg) {
    printf("storage manager startup\n");
    // Open CSV file to write
    FILE *csv = open_db("csv", false);

    pthread_mutex_lock(&shared_buffer->mutex);
    while(sbuffer_size(shared_buffer) == 0)
    {
        printf("storage manager waiting for data\n");
        pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);
    }
    pthread_mutex_unlock(&shared_buffer->mutex);

    pthread_mutex_lock(&shared_buffer->mutex);
    while (1) {
        if (exit_storage_and_data_manager == 1) {
            pthread_mutex_unlock(&shared_buffer->mutex);
            break;
        }

        while (shared_buffer->head == NULL) {
            printf("storage manager waiting for data\n");
            pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);
        }

        sbuffer_node_t *buffer_node = shared_buffer->head;
        while (buffer_node != NULL && buffer_node->next != NULL) {
            int actions_performed = 0;
            sensor_data_t data = *buffer_node->data;

            if (buffer_node->read_by_storage_manager == 1 && buffer_node->read_by_data_manager == 1) {
                sbuffer_node_t *temp = buffer_node->next;
                sbuffer_remove(shared_buffer, buffer_node->data);
                printf("data removed\n");
                buffer_node = temp;
                actions_performed++;
            }

            if (buffer_node->read_by_storage_manager == 0 && actions_performed != 1) {
                insert_sensor(csv, data.id, data.value, data.ts);
                buffer_node->read_by_storage_manager = 1;
                printf("storageloop\n");
                buffer_node = buffer_node->next;
            }
        }

        pthread_mutex_unlock(&shared_buffer->mutex);
    }

    printf("storage manager shutting down\n");
    close_db(csv);
    sbuffer_free(&shared_buffer);
    return NULL;
}






