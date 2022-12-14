/**
 * @file queue.c
 * @author Diego Ortín Fernández
 * @brief Implementation of a thread-safe integer queue, built for storing socket identifiers.
 */

#include "queue.h"
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

/**
 * @struct queue
 * @brief An integer first-in-first-out queue, implemented as a linked list. It includes a mutex for thread safety,
 * and a semaphore to regulate addition of elements to the queue
 */
struct queue {
    struct queue_item *first; ///< Points to the first element in the queue
    struct queue_item *last; ///< Points to the last element in the queue
    int size; ///< The current number of elements in the queue
    int max; ///< The maximum possible number of elements in the queue

    sem_t *available_items; ///< Semaphore signalling the number of available items in the queue
    sem_t *free_slots; ///< Semaphore signalling the number of free slots in the queue
    pthread_mutex_t *mutex; ///< Mutex for thread safety
};

/**
 * @struct queue_item
 * @brief Component of the queue linked list representing a queue item.
 */
struct queue_item {
    struct queue_item *next; ///< Pointer to the next item in the queue, or NULL if it's the last
    int value; ///< Value of the item
};

queue *queue_create(int max) {
    queue *new = calloc(1, sizeof(queue));

    new->first = NULL;
    new->last = NULL;
    new->max = max;
    new->size = 0;

    // Initialize the available items semaphore with a value of 0
    new->available_items = malloc(sizeof(sem_t));
    if (sem_init(new->available_items, 0, 0) != 0) { // Initialize semaphore
        return NULL;
    }


    // Initialize the free slots semaphore with a value corresponding to the maximum amount of items possible
    new->free_slots = malloc(sizeof(sem_t));
    if (sem_init(new->free_slots, 0, max) != 0) { // Initialize semaphore
        return NULL;
    }

    // Initialize the mutex
    new->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(new->mutex, NULL);

    return new;
}

void queue_free(queue *queue) {
    pthread_mutex_lock(queue->mutex);
    if (queue->size > 0) {
        struct queue_item *current = queue->first;
        while (current != NULL) {
            struct queue_item *prev = current;
            current = current->next;
            free(prev);
        }
    }
    free(queue);
    pthread_mutex_unlock(queue->mutex);
}

int queue_isempty(queue *queue) {
    if (!queue) return -1;
    if (queue->size == 0) {
        return 1;
    }
    return 0;
}

int queue_pop(queue *queue) {
    int ret;
    sem_wait(queue->available_items); // Wait until there's an item available in the queue
    pthread_mutex_lock(queue->mutex); // Lock the mutex to protect the critical section
    if (queue->size > 0) {

        struct queue_item *first = queue->first;
        int val = first->value;

        queue->first = first->next;
        free(first);

        queue->size--;

        pthread_mutex_unlock(queue->mutex); // Unlock the mutex
        ret = val;
    } else {
        pthread_mutex_unlock(queue->mutex); // Unlock the mutex
        ret = -1;
    }

    sem_post(queue->free_slots); // Signal the increase in number of free slots in the semaphore by 1
    return ret;
}

void queue_add(queue *queue, int item) {
    sem_wait(queue->free_slots); // Wait until a free slot is available in the queue
    pthread_mutex_lock(queue->mutex); // Lock the mutex to protect the critical section
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
    pthread_mutex_unlock(queue->mutex); // Unlock the mutex
    sem_post(queue->available_items); // Signal that an item is available in the queue
}