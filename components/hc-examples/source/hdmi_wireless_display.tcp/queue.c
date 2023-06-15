#include "queue.h"


#define LOCK(q) do{pthread_mutex_lock(&q->mutex);}while(0)
#define UNLOCK(q) do{pthread_mutex_unlock(&q->mutex);}while(0)

/* Create a Queue */
Queue * initQueue(int max)
{
	Queue *Q;
	Q = (Queue *)calloc(1, sizeof(Queue));
	Q->size = 0;
	Q->capacity = max;
	/* initialize the list head. Kernel list method */
	INIT_LIST_HEAD(&Q->list);
	pthread_mutex_init(&Q->mutex, NULL);
	pthread_cond_init(&Q->cond, NULL);
	return Q;
}

/* pop out the queue front, and free the element space */
void dequeue(Queue *Q)
{
	Queue* tmp;
	LOCK(Q);
	if(Q->size==0) {
		UNLOCK(Q);
		/*printf("Queue is Empty.\n");*/
		return;
	} else {
		Q->size--;
		tmp = list_entry(Q->list.next, Queue, list);
		list_del(Q->list.next);
		UNLOCK(Q);
		free(tmp);
	}
}

QueueElement front(Queue *Q)
{
	Queue* first_element;
	struct list_head * first;
	QueueElement e;

	LOCK(Q);
	if(Q->size==0) {
		/*printf("Queue is Empty\n");*/
		pthread_cond_wait(&Q->cond, &Q->mutex);
	}
	/* find the first element first */
	first = Q->list.next;
	/* reconstruct the first structure */
	first_element = list_entry(first, Queue, list);
	list_del(Q->list.next);
	Q->size--;
	UNLOCK(Q);
	e = first_element->e;
	free(first_element);
	/*printf("%s:Q->size: %d\n", __func__, Q->size);*/
	return e;
}

QueueElement tail(Queue *Q)
{
	Queue* last_element;
	struct list_head * last;
	QueueElement e;
	LOCK(Q);
	if(Q->size==0) {
		/*printf("Queue is Empty.\n");*/
		pthread_cond_wait(&Q->cond, &Q->mutex);
	}
	/* find the last element first */
	last = Q->list.prev;
	/* reconstruct the last structure */
	last_element = list_entry(last, Queue, list);
	UNLOCK(Q);
	e = last_element->e;
	free(last_element);
	return e;
}

int enqueue(Queue *Q, QueueElement element)
{
	Queue* newQ;
	LOCK(Q);
	if(Q->size == Q->capacity) {
		/*printf("Queue is Full. No element added.\n");*/
		UNLOCK(Q);
		return -1;
	} else {
		Q->size++;
		newQ = (Queue*) malloc(sizeof(Queue));
		newQ->e = element;
		/* add to the list tail */
		list_add_tail(&(newQ->list), &(Q->list));
		pthread_cond_signal(&Q->cond);
	}
	UNLOCK(Q);
	/*printf("%s:Q->size: %d\n", __func__, Q->size);*/
	return 0;
}

void queue_clear(Queue *Q, element_callback cb)
{
	Queue* tmp;
	LOCK(Q);
	/*printf("%s:Q->size: %d\n", __func__, Q->size);*/
	while ( Q-> size > 0){
		Q->size--;
		tmp = list_entry(Q->list.next, Queue, list);
		list_del(Q->list.next);
		if(cb)
			cb(tmp->e);
		free(tmp);
	}
	UNLOCK(Q);
}

/*
int test()
{
    int max = 20;
    Queue *testQueue = initQueue(max);
    enqueue(testQueue,1);
    enqueue(testQueue,2);
    printf("Front element is %d\n",front(testQueue));
    printf("Last element is %d\n",tail(testQueue));
    enqueue(testQueue,3);
    dequeue(testQueue);
    enqueue(testQueue,4);
    printf("Front element is %d\n", front(testQueue));
    printf("Last element is %d\n",tail(testQueue));
    dequeue(testQueue);
    dequeue(testQueue);
    printf("Front element is %d\n", front(testQueue));
    printf("Last element is %d\n",tail(testQueue));
}
*/
