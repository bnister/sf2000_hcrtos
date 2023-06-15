#ifndef QUEUE_H_H
#define QUEUE_H_H
#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>

#include <kernel/list.h>

/* by default, QueueElement is int
 Usage: #define QueueElement <TYPE> */
#ifndef QueueElement
#define QueueElement void*
#endif

typedef struct Queue{
    int capacity;
    int size;
    QueueElement e;
    struct list_head list;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;


typedef void (*element_callback)(QueueElement e);
typedef struct Queue Queue;

Queue * initQueue(int max);
void deinitQueue(Queue *Q);
QueueElement front(Queue *Q);	
QueueElement tail(Queue* Q);
void dequeue(Queue *Q);
int enqueue(Queue *Q, QueueElement element);
void queue_clear(Queue *Q, element_callback cb);

#endif //QUEUE_H_H
