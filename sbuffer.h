#ifndef SBUFFER_H
#define SBUFFER_H
#include <stdint.h>
#include "config.h"
#include "pthread.h"
typedef struct sbuffer sbuffer_t;
#define SBUFFER_SUCCESS 0
#define SBUFFER_FAILURE -1
#define SBUFFER_NO_DATA -2
extern sbuffer_t *shared_buffer;
int sbuffer_size(sbuffer_t *buffer);
int sbuffer_init(sbuffer_t **buffer);
int sbuffer_free(sbuffer_t **buffer);
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_peek(sbuffer_t *buffer, sensor_data_t *data);
int check_if_buffer_node_NULL(sbuffer_node_t *buffer_node);
#endif /* SBUFFER_H */
