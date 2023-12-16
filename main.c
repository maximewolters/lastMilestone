#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "sbuffer.h"
#include "sbuffer.c"

FILE *output_file;

void *reader_thread(void *arg) { //func for the reader thread
    sbuffer_t *buffer = (sbuffer_t *)arg;
    sensor_data_t data;
    while (1) {
        if (sbuffer_remove(buffer, &data) == SBUFFER_SUCCESS) { //
            if (data.id == 0 && data.temperature == 0 && data.timestamp == 0) {
                break;
            }
            fprintf(output_file, "%d,%lf,%lu\n", data.id, data.temperature, data.timestamp);
            usleep(25000); //tjis is in microsec
        }
    }
    return NULL;
}
void *writer_thread(void *arg) { //func for the one writer thread
    sbuffer_t *buffer = (sbuffer_t *)arg;
    FILE *input_file = fopen("sensor_data", "rb");
    if (input_file == NULL) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }
    sensor_data_t data;
    while ((fread(&data, sizeof(sensor_data_t), 1, input_file)) > 0) {
        if (sbuffer_insert(buffer, &data) != SBUFFER_SUCCESS) {
            perror("Error inserting data into buffer");
            exit(EXIT_FAILURE);
        }
        usleep(10000); //sleep for 10ms
    }
    data.id = 0;
    data.temperature = 0;
    data.timestamp = 0;
    if (sbuffer_insert(buffer, &data) != SBUFFER_SUCCESS) {
        perror("Error inserting end-of-stream marker into buffer");
        exit(EXIT_FAILURE);
    }
    fclose(input_file);
    return NULL;
}

int main() {
    sbuffer_t *buffer;
    pthread_t reader1, reader2, writer;
    if (sbuffer_init(&buffer) != SBUFFER_SUCCESS) { // initialise buffer
        perror("Error initializing buffer");
        exit(EXIT_FAILURE);
    }
    output_file = fopen("sensor_data_out.csv", "w"); //file where data needs to be inserted as csv
    if (output_file == NULL) { //errior managing
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    pthread_create(&reader1, NULL, reader_thread, buffer); //makubng the three diff threads
    pthread_create(&reader2, NULL, reader_thread, buffer);
    pthread_create(&writer, NULL, writer_thread, buffer);

    pthread_join(writer, NULL);//waiting for all the threads to exit
    pthread_join(reader1, NULL);
    pthread_join(reader2, NULL);

    fclose(output_file);
    sbuffer_free(&buffer); //freeing all memory that is taken by buffer

    return 0;
}




