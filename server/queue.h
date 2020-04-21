/**
 * @file queue.h
 * @author Diego Ortín Fernández
 * @brief A thread safe integer queue implementation
 * @details The purpose of this module is to serve as a first-in-first-out queue for storing socket identificators. It
 * is thread safe, and the #queue_add function blocks if the queue is full
 */

#ifndef PRACTICA1_QUEUE_H
#define PRACTICA1_QUEUE_H

/**
 * @brief The queue type
 */
typedef struct queue queue;

/**
 * @brief Creates a new queue with the specified size
 * @param[in] max The maximum number of items that must fit into the queue
 * @return The newly initialized queue
 */
queue *queue_create(int max);

/**
 * @brief Frees all the memory associated with a queue
 * @param[in] queue The queue to free
 */
void queue_free(queue *queue);

/**
 * @brief Checks if the provided queue contains any items
 * @param[in] queue The queue to check
 * @return 1 if the queue is empty, 0 otherwise
 */
int queue_isempty(queue *queue);

/**
 * @brief Extracts the first item from the queue
 * @details If there are no available items in the queue, this function blocks execution until a new item is added.
 * @param[in/out] queue The queue to extract from
 * @return The value of the extracted item
 */
int queue_pop(queue *queue);

/**
 * @brief Adds a new item to the queue
 * @details If the queue is full, this function blocks execution until a new slot is available.
 * @param[in] queue The queue to add the item to
 * @param[in] item The value that must be added
 */
void queue_add(queue *queue, int item);

#endif //PRACTICA1_QUEUE_H
