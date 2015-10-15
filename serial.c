#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

cond_t* call_func(void* data, cond_t* func)
{
    return (((data != NULL) && (func(data) == true)) || ((data == NULL) && (func() == true)));
}

//Indicates if all queues associated with the serializer are empty
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

// If there is a thread ready to leave a queue, signal it to do so
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

    queue_node_t *visiting = serial->queueBeingServed->queue->head;
    queue_node_t *prev = NULL;
    while(visiting != NULL)
    {
        if (    call_func(visiting->data, visiting->progress_condition))
        {
            pthread_cond_signal(visiting->cVar);
            if (prev == NULL)
            {
                serial->queueBeingServed->queue->head = visiting->next;
            }
            else
            {
                prev->next = visiting->next;
            }
            break;
        }
        else
        {
            prev = visiting;
            visiting = visiting->next;
        }
    }
}

// Instantiates a serializer object
serial_t* Create_Serial()
{
    serial_t *serializer = malloc(sizeof(serial_t));
    serializer->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    serializer->queueBeingServed = NULL;
    pthread_mutex_init(serializer->lock, NULL);
    return serializer;
}

//"Enters" the serializer, assuming sole control over it
void Serial_Enter(serial_t* serial)
{
    pthread_mutex_lock(serial->lock);
}

// Exit the serializer, and attempt to signal a new thread waiting to enter
void Serial_Exit(serial_t* serial)
{
    signal_new_thread(serial);
    pthread_mutex_unlock(serial->lock);
}

// Creates a priority queue to hold threads waiting to enter their critical section
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

// Creates a crowd which allows for execution of actions 'outside' the serializer
crowd_t* Create_Crowd(serial_t* serial)
{
    crowd_t *crowd = malloc(sizeof(crowd_t));
    crowd->count = 0;
    return crowd;
}

// Inidicates if a given queue is empty
int Queue_Empty(serial_t* serial, queue_t* queue)
{
    // check if queue is empty
    if (queue->head == NULL)
    {
        return true;
    }
    return false;
}

// Inidicates if a given queue is empty
int Crowd_Empty(serial_t* serial, crowd_t* crowd)
{
    // check if crowd is empty
    if (crowd->count == 0)
    {
        return true;
    }
    return false;
}

// Enqueues a thread into the given queue (unless it should just proceed), along with the condition of its exit and any supporting execution data
void Serial_Enqueue_Data(serial_t* serial, queue_t* targetQueue, cond_t* progressCondition, int priority, void *data)
{
    //We signal first to avoid signaling ourselves - the signaled thread can not continue until we exit the serialzier
    signal_new_thread(serial);

    queue_node_t *visiting = targetQueue->head;
    queue_node_t *toInstantiate = NULL;
    //If the queue is empty
    if (visiting == NULL)
    {
        if (call_func(data, progressCondition))
        {
            return; //If the node would be the first in the queue, and it's ready, don't join, just continue in the serializer
        }
        toInstantiate = targetQueue->head = malloc(sizeof(queue_node_t));
        toInstantiate->next = NULL;
    }
    else
    {
        bool all_false = true;
        if (priority > visiting->priority)
        {
            queue_node_t *oldHead = visiting;
            toInstantiate = targetQueue->head = malloc(sizeof(queue_node_t));
            toInstantiate->next = oldHead;
        }
        else
        {
            while(visiting->next != NULL && priority <= visiting->next->priority)
            {
                if(call_func(visiting->data, visiting->progress_condition)) //((temp->data && temp->func(temp->data) == 1) || (temp->func() == 1))
                {
                    all_false = false;
                }

                visiting = visiting->next;
            }
            if(visiting->next != NULL) //If we're inserting into the middle of the queue
            {
                if (all_false && call_func(data, progressCondition)) //((data && func(data)  == 1) || func() == 1))
                {
                    return;
                }
                queue_node_t *afterNew = visiting->next;
                visiting->next = malloc(sizeof(queue_node_t));
                visiting->next->next = afterNew;
            }
            // End of queue
            else
            {
                if (all_false && call_func(data, progressCondition)) //((data && func(data)  == 1) || func() == 1))
                {
                    return;
                }
                visiting->next = malloc(sizeof(queue_node_t));
                visiting->next->next = NULL;
            }
            toInstantiate = visiting->next;
        }
    }
    //In all cases, temp points to the new node, which we instantiate
    toInstantiate->progress_condition = progressCondition;
    toInstantiate->cVar = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    toInstantiate->priority = priority;
    toInstantiate->data = data;

    pthread_cond_init(toInstantiate->cVar, NULL);
    pthread_cond_wait(toInstantiate->cVar, serial->lock);
}

// Enqueues a thread into the given queue (unless it should just proceed), along with the condition of its exit
void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t *progressCondition)
{
    Serial_Enqueue_Data(serial, targetQueue, progressCondition, 0, NULL);
}

// Adds a queue to the given crowd, exiting the serializer during its time in the crowd and re-entering when the crowd action completes
void Serial_Join_Crowd(serial_t* serial, crowd_t* targetCrowd, action_t* action)
{
    Serial_Join_Crowd_Data(serial, targetCrowd, action, NULL);
}

// Adds a queue to the given crowd, along with any execution data, exiting the serializer during its time in the crowd and re-entering when the crowd action completes
void Serial_Join_Crowd_Data(serial_t* serial, crowd_t* targetCrowd, action_t* action, void *data)
{
    targetCrowd->count += 1;
    Serial_Exit(serial);
    call_func(data, action);
    Serial_Enter(serial);
    targetCrowd->count -= 1;
}
