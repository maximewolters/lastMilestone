/**
 * \author {AUTHOR}
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>
#include "pthread.h"
#include <unistd.h>

// config.h

#ifndef CONFIG_H
#define CONFIG_H

#ifdef SET_MIN_TEMP
extern const double MIN_TEMP;
#else
extern const double MIN_TEMP;
#endif

#ifdef SET_MAX_TEMP
extern const double MAX_TEMP;
#else
extern const double MAX_TEMP;
#endif

#ifdef time_out
extern const int time_out;
#else
extern const int time_out;
#endif

#endif // CONFIG_H


typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;
// UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

typedef struct sbuffer_node {
    struct sbuffer_node *next;
    sensor_data_t *data;
    int read_by_storage_manager;
    int read_by_data_manager;
} sbuffer_node_t;

struct sbuffer {
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_mutex_t mutex;
    int count;
};

extern pthread_cond_t condition_buffer;

void write_to_pipe(char *log_event);




#endif /* _CONFIG_H_ */
