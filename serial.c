#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

serial_t* Create_Serial()
{
    serial_t *serializer = malloc(sizeof(serial_t));
    serializer->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    serializer->queues = NULL;
    pthread_mutex_init(serializer->m, NULL);
    return serializer;
}

void Serial_Enter(serial_t* serial)
{
    print("locking mutex");
    pthread_mutex_lock(serial->m);
    print("got lock!");

    // have lock
}

void Serial_Exit(serial_t* serial)
{
    int stop = 0;
    serial_node_t *serial_node = serial->queues;
    while(serial_node != NULL && stop == 0)
    {
        queue_node_t *node = serial_node->queue->head;
        queue_node_t *prev = NULL;
        while(node != NULL)
        {
            if (node->func())
            {
                print("found a valid thing");
                int res = pthread_cond_signal(node->c);
                char str[50];
                sprintf(str, "res+signal2: %d", res);
                print(str);
                stop = 1;
                if (prev != NULL)
                {
                    prev->next = node->next;
                }
                break;
            }
            else
            {
                prev = node;
                node = node->next;
            }
        }
        serial_node = serial_node->next;
    }

    print("unlocking mutex");
    pthread_mutex_unlock(serial->m);
}

queue_t* Create_Queue(serial_t* serial)
{
    // maybe call serial enter??
    queue_t *queue = malloc(sizeof(queue_t));
    queue->head = NULL;

    serial_node_t *temp = serial->queues;

    if (temp == NULL)
    {
        serial->queues = malloc(sizeof(serial_node_t));
        temp = serial->queues;
    }
    else
    {
        while (temp->next != NULL)
        {
            temp = temp->next;
            temp = malloc(sizeof(serial_node_t));
        }
    }
    temp->next = NULL;
    temp->queue = queue;

    return queue;
}

crowd_t* Create_Crowd(serial_t* serial)
{
    crowd_t *crowd = malloc(sizeof(crowd_t));
    crowd->count = 0;
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
    if (crowd->count == 0)
    {
        return true;
    }
    return false;
}

void Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t* func, int priority)
{
    //fprintf(stderr, "%d\n", serial->m->__data__.__owner);
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
        if (queue->going_up)
        {
            while(temp->next != NULL && temp->next->priority < priority)
            {
                temp = temp->next;
            }
        }
        else
        {
            while(temp->next != NULL && temp->next->priority > priority)
            {
                temp = temp->next;
            }
        }
        temp->next = malloc(sizeof(queue_node_t));
        temp = temp->next;
    }
    temp->func = func;
    temp->next = NULL;
    temp->c = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    temp->priority = priority;
    //temp->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    pthread_cond_init(temp->c, NULL);
    //pthread_mutex_init(temp->m, NULL);
    /*
    queue_node_t *node = queue->head;
    queue_node_t *prev = NULL;

    while(node != NULL)
    {
        if (node->func())
        {
            int res = pthread_cond_signal(node->c);
            char str[50];
            sprintf(str, "res+signal2: %d");
            print(str);
            if (prev != NULL)
            {
                 prev->next = node->next;
            }
            break;
        }
        else
        {
            prev = node;
            node = node->next;
        }
    }
    print("waiting on condition var");

    //pthread_mutex_lock(temp->m);
    pthread_cond_wait(temp->c, serial->m);
    */
    Serial_Exit(serial);
    print("coming back to life");
    Serial_Enter(serial);
}

void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func)
{
    // already have serializer
    // join the crowd
    // give up serializer (serial exit)
    // call the function
    // serial_enter
    crowd->count += 1;
    Serial_Exit(serial);
    func();
    crowd->count -= 1;
    Serial_Enter(serial);
}

void print(char *string)
{
    // fprintf(stderr, "[%li] %s\n", (unsigned long int)pthread_self(), string);
}
