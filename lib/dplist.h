/**
 * \author {AUTHOR}
 */

#ifndef _DPLIST_H_
#define _DPLIST_H_

#include <stdint-gcc.h>
#include <bits/types/time_t.h>
#include "../datamgr.h"
#include "pthread.h"

typedef enum {
    false, true
} bool; // or use C99 #include <stdbool.h>

/**
 * dplist_t is a struct containing at least a head pointer to the start of the list;
 */
typedef struct SensorList SensorList;

typedef struct SensorNode SensorNode;

typedef struct dplist dplist_t;
typedef struct dplist_node dplist_node_t;
/* General remark on error handling
 * All functions below will:
 * - use assert() to check if memory allocation was successfully.
 */
void* element_copy(void * element);
void element_free(void ** element);
int element_compare(void * x, void * y);

dplist_t *dpl_create(
        void* (*element_copy)(void *element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
);
/** Create and allocate memory for a new list
 * \param element_copy callback function to duplicate 'element'; If needed allocated new memory for the duplicated element.
 * \param element_free callback function to free memory allocated to element
 * \param element_compare callback function to compare two element elements; returns -1 if x<y, 0 if x==y, or 1 if x>y
 * \return a pointer to a newly-allocated and initialized list.
 */
SensorList *SensorList_create();

/** Deletes all elements in the list
 * - Every list node of the list needs to be deleted. (free memory)
 * - The list itself also needs to be deleted. (free all memory)
 * - '*list' must be set to NULL.
 * \param list a double pointer to the list
 * \param free_element if true call element_free() on the element of the list node to remove
 */
void SensorList_free(SensorList* list);

/** Returns the number of elements in the list.
 * - If 'list' is is NULL, -1 is returned.
 * \param list a pointer to the list
 * \return the size of the list
 */
int SensorList_size(SensorList *list);


SensorList* insert_sensor_data(SensorList* list, uint16_t roomID, uint16_t sensorID, double temperature, time_t timestamp);
/** Removes the list node at index 'index' from the list.
 * - The list node itself should always be freed.
 * - If 'index' is 0 or negative, the first list node is removed.
 * - If 'index' is bigger than the number of elements in the list, the last list node is removed.
 * - If the list is empty, return the unmodified list.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param index the position at which the node should be removed from the list
 * \param free_element if true, call element_free() on the element of the list node to remove
 * \return a pointer to the list or NULL
 */
SensorList *dpl_remove_at_index(SensorList *list, int index, bool free_element);

/** Returns a reference to the list node with index 'index' in the list.
 * - If 'index' is 0 or negative, a reference to the first list node is returned.
 * - If 'index' is bigger than the number of list nodes in the list, a reference to the last list node is returned.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param index the position of the node for which the reference is returned
 * \return a pointer to the list node at the given index or NULL
 */
SensorNode* SensorList_get_reference_at_index(SensorList* list, int index);

#endif  // _DPLIST_H_
