//
// Created by diego on 19/3/20.
//

#include "queue.h"
#include <stdlib.h>

struct queue {
    struct queue_item *first;
    struct queue_item *last;
    int size;
    int max;
};

struct queue_item {
    struct queue_item *next;
    int value;
};

queue *queue_create(int max) {
    queue *new = calloc(1, sizeof(queue));

    new->first = NULL;
    new->last = NULL;
    new->max = max;
    new->size = 0;

    return new;
}

void queue_free(queue *queue) {
    if (queue->size > 0) {
        struct queue_item *current = queue->first;
        while (current != NULL) {
            struct queue_item *prev = current;
            current = current->next;
            free(prev);
        }
    }
    free(queue);
}

int queue_isempty(queue *queue) {
    if (!queue) return -1;
    if (queue->size == 0) {
        return 1;
    }
    return 0;
}

int queue_pop(queue *queue) {
    if (queue->size > 0) {
        struct queue_item *first = queue->first;
        int val = first->value;

        queue->first = first->next;
        free(first);

        queue->size--;

        return val;
    } else {
        return -1;
    }
}

void queue_add(queue *queue, int item) {
    if (queue->size < queue->max) {
        struct queue_item *new = calloc(1, sizeof(struct queue_item));
        new->value = item;

        if (queue->size == 0) {
            queue->first = new;
        } else {
            queue->last->next = new;
        }
        queue->last = new;
        queue->size++;
    }
}