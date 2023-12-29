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

int port;
int max_conn;

void *start_connection_manager(void *arg);
void *start_data_manager(void *arg);
void *start_storage_manager(void *arg);
void *start_log_process(void *arg);
void write_log_event(FILE *log_file, const char *event);

#ifdef SET_MIN_TEMP
const double MIN_TEMP = SET_MIN_TEMP;
#else
const double MIN_TEMP = 10;
#endif

#ifdef SET_MAX_TEMP
const double MAX_TEMP = SET_MAX_TEMP;
#else
const double MAX_TEMP = 17;
#endif

#ifdef time_out
const int time_out = TIMEOUT;
#else
const int time_out = 5;
#endif

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
    //initialising max and min temperature

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
        start_log_process(NULL);
        exit(0);
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
        pthread_mutex_destroy(&shared_buffer->mutex);
        pthread_cond_destroy(&condition_buffer);
        sbuffer_free(&shared_buffer);
        return 0;
    }
}

void write_to_pipe(char *log_event)
{
    write(fd[1], log_event, strlen(log_event));
}


void *start_log_process(void *arg) {
    // Create log file
    FILE *gateway_log = fopen("gateway.log", "w");
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

    close_db(gateway_log);// Close the log file
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
    //open the file out of which the datamanager should read the room id's and the sensor id's
    FILE *map = fopen("room_sensor.map", "r");
    // Fill the linked list with nodes from room_sensor.map
    fill_list_from_file("room_sensor.map", sensorList);
    // Update nodes with sensor data from the shared buffer data
    update_sensor_data_from_buffer(shared_buffer, sensorList, shared_buffer->mutex);
    SensorList_free(sensorList);
    fclose(map);

    return 0;

}

void *start_storage_manager(void *arg) {
    printf("storage manager startup\n");
    // Open CSV file to write
    FILE *csv = open_db("sensor_data_out.csv", false);
    sbuffer_node_t *buffer_node;
    while (1) {
        if (exit_storage_and_data_manager == 1) {
            break;
        }
        pthread_mutex_lock(&shared_buffer->mutex);
        while (shared_buffer->head == NULL) {
            printf("storage manager waiting for data\n");
            pthread_cond_wait(&condition_buffer, &shared_buffer->mutex);
        }
        pthread_mutex_unlock(&shared_buffer->mutex);


        pthread_mutex_lock(&shared_buffer->mutex);
        buffer_node = shared_buffer->head;
        pthread_mutex_unlock(&shared_buffer->mutex);
        while (!check_if_buffer_node_NULL(buffer_node)) {
            pthread_mutex_lock(&shared_buffer->mutex);
            int actions_performed = 0;
            sensor_data_t data = *buffer_node->data;
            if (exit_storage_and_data_manager == 1) {
                pthread_mutex_unlock(&shared_buffer->mutex);
                break;
            }
            if(buffer_node->next != NULL)
            {
                if (buffer_node->read_by_storage_manager == 1 && buffer_node->read_by_data_manager == 1) {
                    sbuffer_remove(shared_buffer, buffer_node->data);
                    printf("data removed by storage manager\n");
                    actions_performed++;
                }

                if (buffer_node->read_by_storage_manager == 0 && actions_performed != 1) {
                    insert_sensor(csv, data.id, data.value, data.ts);
                    buffer_node->read_by_storage_manager = 1;
                    printf("storageloop\n");
                }
                buffer_node = buffer_node->next;
            }
            pthread_mutex_unlock(&shared_buffer->mutex);
        }
    }

    printf("storage manager shutting down\n");
    close_db(csv);
    return NULL;
}

int check_if_buffer_node_NULL(sbuffer_node_t *buffer_node) {
    pthread_mutex_lock(&shared_buffer->mutex);
    int is_null = (buffer_node == NULL);
    pthread_mutex_unlock(&shared_buffer->mutex);
    return is_null;
}




