// Created by maxime on 6/11/23.

/*
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src_element);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};

typedef struct{
    uint16_t sensor_id;
    uint16_t room_id;
    time_t last_modified;
    double temperatures[RUN_AVG_LENGTH];
    double temp_avg;

} my_element_t;


struct dplist_node {
    dplist_node_t *prev, *next;
    my_element_t *element;
};


dplist_t *dpl_create(void *(*element_copy)(void *src_element), void (*element_free)(void **element), int (*element_compare)(void *x, void *y)) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    if (*list == NULL) {
        return;
    }
    dplist_node_t *curr = (*list)->head;
    while (curr != NULL) {
        dplist_node_t *nxt = curr->next;

        if (free_element) {
            (*list)->element_free(&(curr->element));
        }

        free(curr);
        curr = nxt;
    }

    free(*list);
    *list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    if (list == NULL) return NULL;

    dplist_node_t *ref_at_index, *list_node;
    list_node = malloc(sizeof(dplist_node_t));

    if (insert_copy) {
        list_node->element = list->element_copy(element);
    } else {
        list_node->element = element;
    }

    if (list->head == NULL) {
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
    } else if (index <= 0) {
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);

        if (index < dpl_size(list)) {
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
        } else {
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
        }
    }

    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL || list->head == NULL) {
        return list;
    }

    dplist_node_t *nodeToRemove = dpl_get_reference_at_index(list, index);

    if (nodeToRemove == NULL) {
        return list;
    }

    if (free_element) {
        list->element_free(&(nodeToRemove->element));
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


int dpl_size(dplist_t *list) {
    if (list == NULL) {
        return -1;
    }

    int cnt = 0;
    dplist_node_t *curr = list->head;

    while (curr != NULL) {
        cnt++;
        curr = curr->next;
    }

    return cnt;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    dplist_node_t *nodeAtIndex = dpl_get_reference_at_index(list, index);

    if (nodeAtIndex == NULL) {
        return NULL;
    }

    return nodeAtIndex->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL || list->head == NULL) {
        return -1;
    }

    dplist_node_t *curr = list->head;
    int i = 0;

    while (curr != NULL) {
        if (list->element_compare(curr->element, element) == 0) {
            return i;
        }

        curr = curr->next;
        i++;
    }

    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    dplist_node_t *curr = list->head;

    for (int i = 0; i < index && curr->next != NULL; i++) {
        curr = curr->next;
    }

    return curr;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || reference == NULL) {
        return NULL;
    }

    return reference->element;
}
*/