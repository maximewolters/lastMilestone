#include "lib/dplist.h"
#include "datamgr.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "sbuffer.h"
#include <pthread.h>
#include "config.h"

//
// Created by maxime on 9/11/23.
//


int exit_data_manager = 0;
char log_event[1000];

SensorList *SensorList_create() {
    SensorList *list;
    list = malloc(sizeof(struct SensorList));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void SensorList_free(SensorList* list) {
    if (list == NULL) {
        return;
    }
    SensorNode* curr = list->head;
    while (curr != NULL) {
        SensorNode* nxt = curr->next;
        free(curr);
        curr = nxt;
    }

    free(list);
    list = NULL;
}

SensorNode* create_sensor_node(uint16_t roomID, uint16_t sensorID, double temperature, time_t timestamp) {
    SensorNode* node = malloc(sizeof(SensorNode));
    ERROR_HANDLER(node == NULL, "Memory allocation failed for SensorNode");

    // Initialize the members of the SensorNode
    node->roomID = roomID;
    node->sensorID = sensorID;
    node->average = temperature;
    node->lastModified = timestamp;

    // Set next and prev pointers to NULL initially
    node->next = NULL;
    node->prev = NULL;

    return node;
}

SensorList* insert_sensor_data(SensorList* list, uint16_t roomID, uint16_t sensorID, double temperature, time_t timestamp) {
    // Create a new SensorNode with the provided data
    SensorNode* new_sensor_data = create_sensor_node(roomID, sensorID, temperature, timestamp);

    // Insert the new SensorNode into the SensorList
    if (list == NULL) {
        // Handle the case where the list is NULL
        free(new_sensor_data); // Clean up allocated memory
        return NULL;
    }

    //insert at the beginning of the list
    new_sensor_data->next = list->head;
    if (list->head != NULL) {
        list->head->prev = new_sensor_data;
    }
    list->head = new_sensor_data;

    return list;
}




SensorList* remove_sensor_node_at_index(SensorList* list, int index) {
    if (list == NULL || list->head == NULL) {
        // Handle cases where the list is NULL or empty
        return list;
    }

    SensorNode* nodeToRemove = SensorList_get_reference_at_index(list, index);

    if (nodeToRemove == NULL) {
        // Handle the case where the index is out of bounds
        return list;
    }

    if (nodeToRemove->prev != NULL) {
        nodeToRemove->prev->next = nodeToRemove->next;
    } else {
        list->head = nodeToRemove->next;
    }

    if (nodeToRemove->next != NULL) {
        nodeToRemove->next->prev = nodeToRemove->prev;
    }

    free(nodeToRemove);

    return list;
}



int SensorList_size(SensorList* list) {
    if (list == NULL) {
        return -1;
    }

    int cnt = 0;
    SensorNode *curr = list->head;

    while (curr != NULL) {
        cnt++;
        curr = curr->next;
    }

    return cnt;
}




SensorNode* SensorList_get_reference_at_index(SensorList* list, int index) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    SensorNode *curr = list->head;

    for (int i = 0; i < index && curr->next != NULL; i++) {
        curr = curr->next;
    }

    return curr;
}





// Function to read sensor data from shared buffer and update the corresponding node
void update_sensor_data_from_buffer(sbuffer_t *buffer, SensorList *sensorList, pthread_mutex_t mutex_buffer) {
    pthread_mutex_lock(&mutex_buffer);
    while(sbuffer_size(buffer) == 0) {
        printf("data manager waiting for data\n");
        pthread_cond_wait(&condition_buffer, &mutex_buffer);
    }
    pthread_mutex_unlock(&mutex_buffer);
    //pthread_cond_signal(&condition_buffer);
    sbuffer_node_t *buffer_node;
    sbuffer_node_t *next_node;
    buffer_node = buffer->head;
    while (1)
    {
        pthread_mutex_lock(&mutex_buffer);
        //printf("entering dataloop\n");
        if(exit_data_manager == 1)
        {
            break;
        }
        while(buffer_node->next != NULL)
        {
            if(exit_data_manager == 1)
            {
                break;
            }
            int actions_performed = 0;
            next_node = buffer_node->next;
            printf("data loop mutex lock\n");
            SensorNode *sensor_node = sensorList->head;
            while (sensor_node->sensorID != buffer_node->data->id) {
                sensor_node = sensor_node->next;
            }
            printf("dataloop selected sensor node\n");
            if(sensor_node == NULL)
            {
                sprintf(log_event, "Received sensor data with invalid sensor node ID %d.\n", buffer_node->data->id);
                write_to_pipe(log_event);
            }
            //delete if read by both storage and data manager
            if(buffer_node->read_by_storage_manager == 1 && buffer_node->read_by_data_manager == 1){
                if (sbuffer_size(shared_buffer) > 2) {
                    sbuffer_remove(shared_buffer, buffer_node->data);
                    printf("data removed\n");
                    buffer_node = next_node;
                    actions_performed++;

                }
            }
            if(sbuffer_size(shared_buffer) > 0 && buffer_node->read_by_data_manager == 0 && actions_performed != 1 && sensor_node != NULL){
                printf("datamanager performing logic\n");
                //assign timestamp to last modified
                sensor_node->lastModified = buffer_node->data->ts;

                //update the temperatures[RUN_AVG_LENGTH] array
                for (int i = RUN_AVG_LENGTH - 1; i > 0; i--) {
                    sensor_node->temperatures[i] = sensor_node->temperatures[i - 1];
                }
                // Add a new element at the first index
                sensor_node->temperatures[0] = buffer_node->data->value;

                //count the number of temperatures that are not zero, then update the average according to the number of available temperatures
                int count_temp_not_zero = 0;
                double sum_of_temperatures = 0;
                for(int i = 0; i < RUN_AVG_LENGTH; i++)
                {
                    if(sensor_node->temperatures[i] != 0.0)
                    {
                        count_temp_not_zero++;
                        sum_of_temperatures += sensor_node->temperatures[i];
                    }
                }
                if(count_temp_not_zero == 0){
                    count_temp_not_zero++;
                }
                sensor_node->average = (sum_of_temperatures/count_temp_not_zero);
                //set read_by_data_manager flag to one
                buffer_node->read_by_data_manager = 1;
                printf("dataloop\n");


                if(sensor_node->average > MAX_TEMP)
                {
                    sprintf(log_event, "Sensor node %d reports it’s too hot (avg temp = %f)", sensor_node->sensorID, sensor_node->average);
                    write_to_pipe(log_event);
                }
                if(sensor_node->average < MIN_TEMP)
                {
                    sprintf(log_event, "Sensor node %d reports it’s too cold (avg temp = %f)", sensor_node->sensorID, sensor_node->average);
                    write_to_pipe(log_event);
                }

            }
            if(buffer_node->read_by_storage_manager == 1 && buffer_node->read_by_data_manager != 1 && actions_performed != 1){
                pthread_mutex_unlock(&mutex_buffer);
                continue;
            }

        }
        buffer_node = next_node; //changed this, might affect code but i keep it for now
        pthread_mutex_unlock(&mutex_buffer);

    }

    printf("data manager shutting down\n");
    pthread_exit(NULL);

}








//function to read the room_sensor.map file and create a new node for each line in the file
void fill_list_from_file(const char *filename, SensorList *list) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        // Handle file opening error
        perror("Error opening file");
        return;
    }

    uint16_t roomID, sensorID;
    while (fscanf(file, "%hu %hu\n", &roomID, &sensorID) == 2) {
        // Insert the new node into the list with initial values
        list = insert_sensor_data(list, roomID, sensorID, 0.0, time(NULL));
        printf("RID: %d, SID: %d\n", list->head->roomID, list->head->sensorID);
    }
    fclose(file);
}

void update_exit()
{
    exit_data_manager = 1;
}




