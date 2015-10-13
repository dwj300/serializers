#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

void print_queue(serial_t *serial)
{
    if (serial->queueBeingServed->queue == NULL)
        return;
    printf("Queue: ");
    queue_node_t *cur = serial->queueBeingServed->queue->head;
    while(cur != NULL)
    {
        printf("%d ", cur->tid);
        cur = cur->next;
    }
    printf("\n");
}

serial_t* Create_Serial()
{
    serial_t *serializer = malloc(sizeof(serial_t));
    serializer->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    serializer->queueBeingServed = NULL;
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

bool All_Queues_Empty(serial_t* serial)
{
    //  We want to break when we've checked all the queues. Because our queue
    //  list is circular, we'll be looking for when we 'wrap around.' To keep
    //  track of this, we'll note the active one, but we'll start checking at
    //  the queue *after* the active one. If make it around to the active one,
    //  we can know that it is the last, and we won't check any twice
    queue_list_node_t *activeQueue = serial->queueBeingServed;
    queue_list_node_t *toCheck = serial->queueBeingServed->next;

    while(Queue_Empty(serial, toCheck->queue))
    {
        if(toCheck == activeQueue)
        {
            return true;
        }
        else
        {
            toCheck = toCheck->next;
        }
    }
    return false;
}

void signal_new_thread(serial_t *serial)
{
    print_queue(serial);
    print("starting search\n");
    // Search through the queues of the serializer until we find the next
    // thread waiting and ready to enter
    bool nextHolderFound = false;
    queue_list_node_t *start = serial->queueBeingServed;
    //If there's no one to signal, just leave the serializer
    if(!All_Queues_Empty(serial))
    {
        do
        {
            queue_node_t *node = serial->queueBeingServed->queue->head;
            if(node == NULL)
                print("the fuck?");
            queue_node_t *prev = NULL;
            while(node != NULL)
            {
                if (node->func(node->tid))
                {
                    print("found a valid thing");
                    int res = pthread_cond_signal(node->c);
                    char str[50];
                    sprintf(str, "res+signal2: %d", res);
                    print(str);
                    nextHolderFound = true;
                    if (prev == NULL)
                    {
                        serial->queueBeingServed->queue->head = node->next;
                    }
                    else
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
            //hack
            // nextHolderFound = true;
            if (!nextHolderFound)
                serial->queueBeingServed = serial->queueBeingServed->next;
        } while(!nextHolderFound && (serial->queueBeingServed != start));
    }
    print("finished search\n");
}

void Serial_Exit(serial_t* serial)
{
    signal_new_thread(serial);
    print("unlocking mutex");
    pthread_mutex_unlock(serial->m);
}

queue_t* Create_Queue(serial_t* serial)
{
    //Allocate a new queue and a queue list node to hold it
    queue_list_node_t *newQueueListNode = malloc(sizeof(queue_list_node_t));
    queue_t *newQueue = malloc(sizeof(queue_t));
    newQueue->head = NULL;
    newQueueListNode->queue = newQueue;

    //If this is the only queue for the serializer (as of its creation), make it the active queue
    if (serial->queueBeingServed == NULL)
    {
        serial->queueBeingServed = newQueueListNode;
        newQueueListNode->next = newQueueListNode;  //Link circularly!
    }
    else
    {
        //Otherwise, we'll insert the new queue right after the active queue
        newQueueListNode->next = serial->queueBeingServed->next;
        serial->queueBeingServed->next = newQueueListNode;
    }

    return newQueue;
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

void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t* func, int priority, int tid)
{
    //fprintf(stderr, "%d\n", serial->m->__data__.__owner);
    // Add to back of queue
    // Find first node where condition is true
    // signal that node
    // give up serializer lock
    queue_node_t *temp = targetQueue->head;

    //If the queue is empty
    if (temp == NULL)
    {
        if (func(tid) == true)
        {
            return; //If the node would be the first in the queue, and it's ready, don't join, just continue in the serializer
        }
        temp = targetQueue->head = malloc(sizeof(queue_node_t));
        temp->next = NULL;
    }
    else
    {
        while(temp->next != NULL && priority <= temp->next->priority)
        {
            temp = temp->next;
        }
        if(temp->next != NULL) //If we're inserting into the middle of the queue
        {
            queue_node_t *afterNew = temp->next;
            temp->next = malloc(sizeof(queue_node_t));
            temp->next->next = afterNew;
        }
        else
        {
            temp->next = malloc(sizeof(queue_node_t));
            temp->next->next = NULL;
        }
        temp = temp->next;
    }
    //In all cases, temp points to the new node
    temp->func = func;
    temp->c = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    temp->priority = priority;
    temp->tid = tid;
    //temp->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    pthread_cond_init(temp->c, NULL);

    print("waiting on condition var");

    //pthread_mutex_lock(temp->m);

    signal_new_thread(serial);

    pthread_cond_wait(temp->c, serial->m);
    
    print("coming back to life");
    // Serial_Enter(serial);
}

void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func, int tid)
{
    // already have serializer
    // join the crowd
    // give up serializer (serial exit)
    // call the function
    // serial_enter
    crowd->count += 1;
    Serial_Exit(serial);
    /*if (data != NULL)
    {
        func(data);
    }
    else
    {
        func();
    }*/
    func(tid);
    crowd->count -= 1;
    print("leaving crowd");
    Serial_Enter(serial);

}

void print(char *string)
{
    //fprintf(stderr, "[%li] %s\n", (unsigned long int)pthread_self(), string);
}

