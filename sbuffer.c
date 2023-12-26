#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"
#include "config.h"
//making the structs


int sbuffer_init(sbuffer_t **buffer) {//initialising array, same as always
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->count = 0;
    pthread_mutex_init(&((*buffer)->mutex), NULL);
    return SBUFFER_SUCCESS;
}
int sbuffer_free(sbuffer_t **buffer) { //free allocated mem, but witj mutually excl flags to ensure no race conditions blabla
    sbuffer_node_t *newNode;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    pthread_mutex_lock(&((*buffer)->mutex));
    while ((*buffer)->head) {
        newNode = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(newNode);
    }
    pthread_mutex_unlock(&((*buffer)->mutex));
    pthread_mutex_destroy(&((*buffer)->mutex));
    (*buffer)->count = 0;
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {//safe removal, again with mutex flags
    pthread_mutex_lock(&(buffer->mutex));
    sbuffer_node_t *newNode;
    if (buffer == NULL) {
        pthread_mutex_unlock(&(buffer->mutex));
        return SBUFFER_FAILURE;
    }
    if (buffer->head == NULL) {
        pthread_mutex_unlock(&(buffer->mutex));
        return SBUFFER_NO_DATA;
    }
    buffer->count--;
    *data = *buffer->head->data;
    newNode = buffer->head;
    if (buffer->head == buffer->tail) {
        buffer->head = buffer->tail = NULL;
    } else {
        buffer->head = buffer->head->next;
    }
    free(newNode);
    pthread_mutex_unlock(&(buffer->mutex));
    return SBUFFER_SUCCESS;
}
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {//safe insert
    pthread_mutex_lock(&(buffer->mutex));
    sbuffer_node_t *newNode;
    if (buffer == NULL) {
        pthread_mutex_unlock(&(buffer->mutex));
        return SBUFFER_FAILURE;
    }
    newNode = malloc(sizeof(sbuffer_node_t));
    if (newNode == NULL) {
        pthread_mutex_unlock(&(buffer->mutex));
        return SBUFFER_FAILURE;
    }
    buffer->count++;
    newNode->data = data;
    newNode->next = NULL;
    if (buffer->tail == NULL) {
        buffer->head = buffer->tail = newNode;
    } else {
        buffer->tail->next = newNode;
        buffer->tail = buffer->tail->next;
    }
    pthread_mutex_unlock(&(buffer->mutex));
    return SBUFFER_SUCCESS;
}

int sbuffer_size(sbuffer_t *buffer) {
    if (buffer == NULL) {
        return -1; // Invalid buffer
    }

    return buffer->count;
}


int sbuffer_peek(sbuffer_t *buffer, sensor_data_t *data) {
    if (buffer == NULL || data == NULL) {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&buffer->mutex);

    if (buffer->tail == NULL) {
        pthread_mutex_unlock(&buffer->mutex);
        return SBUFFER_NO_DATA;
    }

    // Copy data from the first node without removing it
    *data = *buffer->tail->data;

    pthread_mutex_unlock(&buffer->mutex);

    return SBUFFER_SUCCESS;
}




