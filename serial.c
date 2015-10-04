#include "serial.h"
#include <stdlib.h>

serial_t* Create_Serial()
{
    serial_t *serializer = malloc(sizeof(serial_t));
    serializer->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(serializer->m, NULL);
    return serializer;
}

void Serial_Enter(serial_t* serial)
{
    pthread_mutex_lock(serial->m);
    // have lock
}

void Serial_Exit(serial_t* serial)
{
    pthread_mutex_unlock(serial->m);
}

queue_t* Create_Queue(serial_t* serial)
{
    // maybe call serial enter??
    queue_t *queue = malloc(sizeof(queue_t));
    queue->head = NULL;
    return queue;
}

crowd_t* Create_Crowd(serial_t* serial)
{
    crowd_t *crowd = malloc(sizeof(crowd_t));
    crowd->head = NULL;
    return crowd;
}

int Queue_Empty(serial_t* serial, queue_t* queue)
{
    // check if queue is empty
    if (queue->head == NULL)
    {
        return true;
    }
    return false;
}

int Crowd_Empty(serial_t* serial, crowd_t* crowd)
{
    // check if crowd is empty
    if (crowd->head == NULL)
    {
        return true;
    }
    return false;
}

void Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t* func)
{
    // Add to back of queue
    // Find first node where condition is true
    // signal that node
    // give up serializer lock
    queue_node_t *temp = queue->head;
    
    if (temp == NULL) 
    {
        if (func() == true)
        {
            return;
        }
        temp = malloc(sizeof(queue_node_t));
        queue->head = temp;
    }
    else
    {
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = malloc(sizeof(queue_node_t));
        temp = temp->next;
    }
    temp->func = func;
    temp->next = NULL;
    temp->c = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    
    pthread_cond_init(temp->c, NULL);
    
    queue_node_t *node = queue->head;
    
    while(node != NULL)
    {
        if (node->func())
        {
            break;
        }
        else
        {
            node = node->next;
        }
    }

    if (node != NULL)
    {

        pthread_cond_signal(node->c);
    }
    
    pthread_cond_wait(node->c, serial->m);
    Serial_Enter(serial);
}

void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func)
{
    printf("here1");
    // already have serializer
    // join the crowd
    // give up serializer (serial exit)
    // call the function
    // serial_enter

    crowd_node_t *temp = crowd->head;
    
    if (temp == NULL) 
    {
        temp = malloc(sizeof(crowd_node_t));
        crowd->head = temp;
    }
    else
    {
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = malloc(sizeof(crowd_node_t));
        temp = temp->next;
    }
    temp->p = pthread_self();
    temp->next = NULL; 
    Serial_Exit(serial);
    func();
    Serial_Enter(serial);
}
