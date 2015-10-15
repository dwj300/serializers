#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

void print_queue(serial_t *serial)
{
    if (serial->queueBeingServed->queue == NULL)
        return;
    print("Queue: ");
    queue_node_t *cur = serial->queueBeingServed->queue->head;
    while(cur != NULL)
    {
        char str[50];
        //sprintf(str, "%d ", ((data_t *)(cur->data))->tid);
        print(str);
        cur = cur->next;
    }

    print("\n");
}

void PrintQueue(queue_t * toPrint)
{
    if(toPrint == NULL)
    {
//        print("Well shit");
        return;
    }
    if(toPrint->head == NULL)
    {
//        print("Well shit2");
        return;
    }
    queue_node_t * selector = toPrint->head;
    while(selector!=NULL)
    {
        //char temp[100];
        //sprintf(temp, " %4d ", selector->priority);
        //print (temp);
        fprintf(stderr, "p:%d, ", selector->priority);
        selector = selector->next;
    }

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
    queue_list_node_t *start = serial->queueBeingServed;
    //If there's no one to signal, just leave the serializer
    if(!All_Queues_Empty(serial))
    {
	// Don't need to deal with single queue case, because all queues empty takes care of that
	if(Queue_Empty(serial, serial->queueBeingServed->queue))
	{
            serial->queueBeingServed = serial->queueBeingServed->next;
            printf("switching direction\n");
	}

            queue_node_t *node = serial->queueBeingServed->queue->head;
            queue_node_t *prev = NULL;
            while(node != NULL)
            {
                serial->onDeck = node;
                if (node->func(node->data))
                {
                    print("found a valid thing");

                    fprintf(stderr, "signaling %d\n", node->priority);
                    //char str[50];
                    //sprintf(str, "signaling tid: %d\n", ((data_t *)node->data)->tid);
                    //print(str);
                    pthread_cond_signal(node->c);
                    //int res = pthread_cond_signal(node->c);
                    //char str1[50];
                    //sprintf(str1, "res+signal2: %d", res);
                    //print(str1);
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

void Serial_Enqueue(serial_t* serial, queue_t* targetQueue, cond_t* func, int priority, void *data)
{
    signal_new_thread(serial);

    fprintf(stderr, "before enqueue: pri:%d\n", priority);
    fprintf(stderr, "Current Queue: ");
    PrintQueue(serial->queueBeingServed->queue);
    fprintf(stderr, "\n");
    fprintf(stderr, "Other Queue: ");
    PrintQueue(serial->queueBeingServed->next->queue);
    fprintf(stderr, "\n");

    //fprintf(stderr, "%d\n", serial->m->__data__.__owner);
    // Add to back of queue
    // Find first node where condition is true
    // signal that node
    // give up serializer lock
    queue_node_t *temp = targetQueue->head;

    //If the queue is empty
    if (temp == NULL)
    {
        if (func(data) == true)
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
		    if(temp->func(temp->data) == 1)
		    {
			all_false = false;
		    }

		    temp = temp->next;
		}
		if(temp->next != NULL) //If we're inserting into the middle of the queue
		{
		    if (all_false && (func(data) == 1))
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
		    if (all_false && (func(data) == 1))
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
    //temp->m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    fprintf(stderr, "after enqueue:\n");
    fprintf(stderr, "Current Queue: ");
    PrintQueue(serial->queueBeingServed->queue);
    fprintf(stderr, "\n");
    fprintf(stderr, "Other Queue: ");
    PrintQueue(serial->queueBeingServed->next->queue);
    fprintf(stderr, "\n");



    pthread_cond_init(temp->c, NULL);

    print("waiting on condition var");

    //pthread_mutex_lock(temp->m);

    char str[50];
    //sprintf(str, "starting cond wait: %d\n", ((data_t *)data)->tid);
    print(str);

    pthread_cond_wait(temp->c, serial->m);

    fprintf(stderr, "coming back to life: %d\n", priority);
    // Serial_Enter(serial);
}

void Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func, void *data)
{
    // already have serializer
    // join the crowd
    // give up serializer (serial exit)
    // call the function
    // serial_enter
    crowd->count += 1;
    Serial_Exit(serial);
    if (data != NULL)
    {
        func(data);
    }
    else
    {
        func();
    }
    print("leaving crowd");
    Serial_Enter(serial);
    crowd->count -= 1;

}

void print(char *string)
{
    // fprintf(stderr, "[%li] %s\n", (unsigned long int)pthread_self(), string);
}




