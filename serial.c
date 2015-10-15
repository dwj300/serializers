#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

cond_t* call_func(void* data, cond_t* func)
{
    return (((data != NULL) && (func(data) == true)) || ((data == NULL) && (func() == true)));
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
    pthread_mutex_lock(serial->m);
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
    // Search through the queues of the serializer until we find the next
    // thread waiting and ready to enter
    // If there's no one to signal, just leave the serializer
    if(All_Queues_Empty(serial))
    {
        return;
    }

    // Don't need to deal with single queue case, because all queues empty takes care of that
    // Switch directions
    if(Queue_Empty(serial, serial->queueBeingServed->queue))
    {
        serial->queueBeingServed = serial->queueBeingServed->next;
    }

    queue_node_t *node = serial->queueBeingServed->queue->head;
    queue_node_t *prev = NULL;
    while(node != NULL)
    {
        serial->onDeck = node;
        if (    call_func(node->data, node->func))
        {
            pthread_cond_signal(node->c);
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
}

void Serial_Exit(serial_t* serial)
{
    signal_new_thread(serial);
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

void Serial_Enqueue_Data(serial_t* serial, queue_t* targetQueue, cond_t* func, int priority, void *data)
{
    signal_new_thread(serial);

    // Add to correct spot if queue
    // Find first node where condition is true
    // give up serializer lock
    queue_node_t *temp = targetQueue->head;

    //If the queue is empty
    if (temp == NULL)
    {
        if (call_func(data, func))
        {
            return; //If the node would be the first in the queue, and it's ready, don't join, just continue in the serializer
        }
        temp = targetQueue->head = malloc(sizeof(queue_node_t));
        temp->next = NULL;
    }
    else
    {
        bool all_false = true;
        if (priority > temp->priority)
        {
            queue_node_t *next = temp;
            targetQueue->head = malloc(sizeof(queue_node_t));
            targetQueue->head->next = next;
            temp = targetQueue->head;
        }
        else
        {
        while(temp->next != NULL && priority <= temp->next->priority)
        {
            if(call_func(temp->data, temp->func)) //((temp->data && temp->func(temp->data) == 1) || (temp->func() == 1))
            {
            all_false = false;
            }

            temp = temp->next;
        }
        if(temp->next != NULL) //If we're inserting into the middle of the queue
        {
            if (all_false && call_func(data, func)) //((data && func(data)  == 1) || func() == 1))
            {
            return;
            }
            queue_node_t *afterNew = temp->next;
            temp->next = malloc(sizeof(queue_node_t));
            temp->next->next = afterNew;
        }
        // End of queue
        else
        {
            if (all_false && call_func(data, func)) //((data && func(data)  == 1) || func() == 1))
            {
            return;
            }
            temp->next = malloc(sizeof(queue_node_t));
            temp->next->next = NULL;
        }
        temp = temp->next;
        }
    }
    //In all cases, temp points to the new node
    temp->func = func;
    temp->c = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    temp->priority = priority;
    temp->data = data;

    pthread_cond_init(temp->c, NULL);

    pthread_cond_wait(temp->c, serial->m);

    // Serial_Enter(serial);
}

void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t *func)
{
    Serial_Enqueue_Data(serial, targetQueue, func, 0, NULL);
}

void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func)
{
    Serial_Join_Crowd_Data(serial, crowd, func, NULL);
}

void Serial_Join_Crowd_Data(serial_t* serial, crowd_t* crowd, cond_t* func, void *data)
{
    // already have serializer
    // join the crowd
    // give up serializer (serial exit)
    // call the function
    // serial_enter
    crowd->count += 1;
    Serial_Exit(serial);
    call_func(data, func);
    Serial_Enter(serial);
    crowd->count -= 1;
}
