//
// Created by diego on 19/3/20.
//

#ifndef PRACTICA1_QUEUE_H
#define PRACTICA1_QUEUE_H

typedef struct queue queue;

queue *queue_create(int max);

void queue_free(queue *queue);

int queue_isempty(queue *queue);

int queue_pop(queue *queue);

void queue_add(queue *queue, int item);

#endif //PRACTICA1_QUEUE_H
