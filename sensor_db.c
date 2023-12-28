//
// Created by maxime on 15/11/23.
//
#include "sensor_db.h"
#include "config.h"
char logev[1000];
FILE *open_db(const char *filename, bool append) {
    FILE *file = fopen(filename, append ? "a" : "w");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    sprintf(logev, "A new data.csv file has been created");
    write_to_pipe(logev);
    return file;
}
int insert_sensor(FILE *file, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
    if (file == NULL) {
        fprintf(stderr, "Error: File pointer is NULL\n");
        return -1;
    }
    int result = fprintf(file, "%u, %.6f, %ld\n", id, value, ts);
    if (result < 0) {
        perror("Error writing to file");
        return -1;
    }
    sprintf(logev, "Data insertion from sensor %d succeeded.", id);
    write_to_pipe(logev);
    return 0;
}
int close_db(FILE *file) {
    if (file == NULL) {
        fprintf(stderr, "Error: File pointer is NULL\n");
        return -1;
    }
    int result = fclose(file);
    if (result != 0) {
        perror("Error closing file");
        return -1;
    }
    sprintf(logev, "The data.csv file has been closed.");
    write_to_pipe(logev);
    return 0;
}

