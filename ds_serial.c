/*
 * disk scheduler
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "ds_serial.h"

void PrintQueue(queue_t * toPrint)
{
    if(toPrint == NULL)
    {
        print("Well shit");
        return;
    }
    if(toPrint->head == NULL)
    {
        print("Well shit2");
        return;
    }
    queue_node_t * selector = toPrint->head;
    while(selector!=NULL)
    {
        char temp[100];
        sprintf(temp, " %4d ", selector->priority);
        print (temp);
    }
}

bool GoingUp()
{
    return (serializer->queueBeingServed->queue == up_queue);
}

bool GoingDown()
{
    return (serializer->queueBeingServed->queue == down_queue);
}

int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id)
{
    char temp[100];

    uint32_t serviceNum = 0;
    //Get access to the serializer's synchronization constructs
	Serial_Enter(serializer);

    bool going_up = GoingUp();
    if(going_up)
    {
        print("Going Up!");
    }

    if(GoingDown())
    {
        print("Going Down!");
    }

    PrintQueue(up_queue);
    PrintQueue(down_queue);

    //When a request is issued, we have to choose which queue to place the request into.
    //  If the target is 'coming up' in the direction of the queue being served, insert
    //  it into the queue being served, and vice-versa
    if( (going_up && cylinderno > just_finished) || (!going_up && cylinderno <= just_finished) )
    {
        sprintf(temp, "Scheduling request for cylinder %d in the up queue", cylinderno);
        print(temp);
        Serial_Enqueue(serializer, up_queue, up_cond, cylinder_count-cylinderno, NULL); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( (going_up && cylinderno <= just_finished) || (!going_up && cylinderno > just_finished) )
    {
        sprintf(temp, "Scheduling request for cylinder %d in the down queue", cylinderno);
        print(temp);
        Serial_Enqueue(serializer, down_queue, up_cond, cylinder_count, NULL);
    }
    else
    {
        print("Logic error");
    }

    //When we exit the queue, the request can be serviced
    Serial_Join_Crowd(serializer, disk_access_crowd,(void *)model_request, NULL);

    //Request satisfied,
    sprintf(temp, "Seeked %s from %d to %d", (going_up ? "downward" : "upward"), just_finished, cylinderno);
    print(temp);

    *seekedcylinders += abs(just_finished-cylinderno);
    just_finished = cylinderno;

    serviceNum = disk_access_sequence_number++;

    Serial_Exit(serializer);
    return serviceNum;
}

void Init_ds(int ncylinders, int CylinderSeekTime)
{
	// Init Serializer
	serializer = Create_Serial();
	cylinder_count = ncylinders;
	up_queue = Create_Queue(serializer);
	down_queue = Create_Queue(serializer);
    disk_access_crowd = Create_Crowd(serializer);
    disk_access_sequence_number = 0;
}

cond_t* up_cond()
{
    return ( Crowd_Empty(serializer, disk_access_crowd) && serializer->onDeck == up_queue->head );
}

cond_t* down_cond()
{
    return ( Crowd_Empty(serializer, disk_access_crowd) && serializer->onDeck == down_queue->head );
}