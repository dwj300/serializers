#include <pthread.h>
#include <stdlib.h>

#define true 1
#define false 0

typedef int (cond_t)();

typedef int bool;

typedef struct queue_node
{
    pthread_mutex_t *m;
    pthread_cond_t *c;
    cond_t *func;
    int priority;
    struct queue_node *next;
} queue_node_t;

typedef struct queue
{
    queue_node_t *head;
    bool going_up;
} queue_t;

typedef struct crowd
{
    int count;
} crowd_t;

typedef struct serial_node
{
    queue_t *queue;
    struct serial_node *next;
} serial_node_t;

typedef struct serial
{
    serial_node_t *queues;
    pthread_mutex_t *m;
} serial_t;


serial_t* Create_Serial();
void Serial_Enter(serial_t*);
void Serial_Exit(serial_t*);
queue_t* Create_Queue(serial_t*);
crowd_t* Create_Crowd(serial_t*);
int Queue_Empty(serial_t* serial, queue_t* queue);
int Crowd_Empty(serial_t* serial, crowd_t* crowd);
void Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t *func, int priority);
void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func);
void print(char *string);

