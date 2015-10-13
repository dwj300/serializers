#include <pthread.h>
#include <stdlib.h>

#define true 1
#define false 0

#define INT2VOIDP(i) (void*)(uintptr_t)(i)

typedef int (cond_t)();

//typedef int *(*(cond_t)()) ;

typedef int bool;

typedef struct queue_node
{
    pthread_mutex_t *m;
    pthread_cond_t *c;
    cond_t *func;
    int priority;
    int *tid;
    struct queue_node *next;
} queue_node_t;

typedef struct queue
{
    queue_node_t *head;
} queue_t;

typedef struct crowd
{
    int count;
} crowd_t;

typedef struct queueListNode
{
    queue_t *queue;
    struct queueListNode *next;
} queue_list_node_t;

typedef struct serial
{
    queue_list_node_t *queueBeingServed;
    pthread_mutex_t *m;
} serial_t;


serial_t* Create_Serial();
void Serial_Enter(serial_t*);
void Serial_Exit(serial_t*);
queue_t* Create_Queue(serial_t*);
crowd_t* Create_Crowd(serial_t*);
int Queue_Empty(serial_t* serial, queue_t* queue);
int Crowd_Empty(serial_t* serial, crowd_t* crowd);
void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t *func, int priority, int tid);
void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func, int tid);
void print(char *string);
bool All_Queues_Empty(serial_t* serial);

