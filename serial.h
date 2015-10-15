#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef long (cond_t)();
typedef long (action_t)();
typedef int bool;
#define true 1
#define false 0

typedef struct queue_node
{
    pthread_cond_t *cVar;
    cond_t *progress_condition;
    int priority;
    void *data;
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
    pthread_mutex_t *lock;
} serial_t;

serial_t* Create_Serial();
void Serial_Enter(serial_t*);
void Serial_Exit(serial_t*);
queue_t* Create_Queue(serial_t*);
crowd_t* Create_Crowd(serial_t*);
int Queue_Empty(serial_t* serial, queue_t* queue);
int Crowd_Empty(serial_t* serial, crowd_t* crowd);
void Serial_Enqueue_Data(serial_t* serial, queue_t* targetQueue, cond_t* progressCondition, int priority, void *data);
void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t *progressCondition);
void Serial_Join_Crowd_Data(serial_t* serial, crowd_t* targetCrowd, action_t* action, void *data);
void Serial_Join_Crowd(serial_t* serial, crowd_t* targetCrowd, action_t* action);
