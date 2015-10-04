#include <pthread.h>
#include <stdlib.h>

#define true 1
#define false 0 

typedef int (cond_t)();

typedef struct serial 
{
    pthread_mutex_t *m;
} serial_t;

typedef struct queue_node
{
    pthread_cond_t *c;
    pthread_mutex_t *m;
    cond_t *func;
    struct queue_node *next;
} queue_node_t;

typedef struct queue
{
    queue_node_t *head;
} queue_t;

typedef struct crowd_node
{
    pthread_t p;
    struct crowd_node *next;
} crowd_node_t;

typedef struct crowd 
{
    crowd_node_t *head;
} crowd_t;

typedef int bool;



serial_t* Create_Serial();
void Serial_Enter(serial_t*);
void Serial_Exit(serial_t*);
queue_t* Create_Queue(serial_t*);
crowd_t* Create_Crowd(serial_t*);
int Queue_Empty(serial_t* serial, queue_t* queue);
int Crowd_Empty(serial_t* serial, crowd_t* crowd);
void Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t *func);
void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func);
