#ifndef SBUFFER_H
#define SBUFFER_H
#include <stdint.h>
typedef struct {
    int32_t id;
    double temperature;
    uint64_t timestamp;
} sensor_data_t;
typedef struct sbuffer sbuffer_t;
#define SBUFFER_SUCCESS 0
#define SBUFFER_FAILURE -1
#define SBUFFER_NO_DATA -2
int sbuffer_init(sbuffer_t **buffer);
int sbuffer_free(sbuffer_t **buffer);
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);
#endif /* SBUFFER_H */
