/*
 * disk scheduler
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "ds_serial.h"


void Switch()
{
    if(current_queue == up_queue)
        current_queue = down_queue;
    else
        current_queue = up_queue;
}

bool GoingUp()
{
    return (current_queue == up_queue);
}

bool GoingDown()
{
    return !GoingUp();
}

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





int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id)
{
    char temp[100];

    uint32_t serviceNum = 0;
    //Get access to the serializer's synchronization constructs

	Serial_Enter(serializer);

    scheduler_data_t* newNodeData = (scheduler_data_t*)malloc(sizeof(scheduler_data_t));
    serviceNum = newNodeData->seq = disk_access_sequence_number++;

    PrintQueue(up_queue);
    PrintQueue(down_queue);

    bool scheduledInUp = false;

    //When a request is issued, we have to choose which queue to place the request into.
    //  If the target is 'coming up' in the direction of the queue being served, insert
    //  it into the queue being served, and vice-versa
    if( (GoingUp() && cylinderno >= head_position)) //Going up, target is ahead of us
    {
        sprintf(temp, "Scheduling request for cylinder %d in the up queue", cylinderno);
        print(temp);
        scheduledInUp = true;
        Serial_Enqueue(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno > head_position )
    {
        sprintf(temp, "Scheduling request for cylinder %d in the up queue", cylinderno);
        print(temp);
        scheduledInUp = true;
        Serial_Enqueue(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno <= head_position )  //Going down, target behind us (approaching it)
    {
        sprintf(temp, "Scheduling request for cylinder %d in the down queue", cylinderno);
        print(temp);
        Serial_Enqueue(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else if( GoingUp() && cylinderno < head_position ) //Going up and target is behind us (getting farther)
    {
        sprintf(temp, "Scheduling request for cylinder %d in the down queue", cylinderno);
        print(temp);
        Serial_Enqueue(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else
    {
        print("Logic error");
        exit(-1);
    }


    if( (scheduledInUp && GoingDown()) || (!scheduledInUp && GoingDown()) )
    {
        Switch();
    }

    data_t* newData = (data_t*)malloc(sizeof(data_t));
    newData->tid = id;
    newData->seeked_cylinders = abs(head_position-cylinderno);

    //When we exit the queue, the request can be serviced
    Serial_Join_Crowd(serializer, disk_access_crowd,(void *)model_request, newData);

    //Request satisfied,
    //sprintf(temp, "Seeked %s from %d to %d", (going_up ? "downward" : "upward"), head_position, cylinderno);
    //print(temp);

    *seekedcylinders += abs(head_position-cylinderno);
    head_position = cylinderno;


    Serial_Exit(serializer);
    return serviceNum;
}

void Init_ds(int ncylinders, int CylinderSeekTime)
{
	// Init Serializer
	serializer = Create_Serial();
	cylinder_count = ncylinders;
	up_queue = Create_Queue(serializer);
	current_queue = down_queue = Create_Queue(serializer);
    disk_access_crowd = Create_Crowd(serializer);
    disk_access_sequence_number = 0;
    head_position = 0;
}

void serviceRequest(data_t* data)
{
    model_request(data->tid, data->seeked_cylinders);
}


cond_t* up_cond(void * data)
{
    int seq = ((scheduler_data_t*)data)->seq;

    return ( Crowd_Empty(serializer, disk_access_crowd)
                &&(
                    Queue_Empty(serializer, up_queue) && Queue_Empty(serializer, down_queue)
                    ||
                    (
                        current_queue == up_queue
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->queue->head->data) )->seq == seq
                    )
                    ||
                    (
                        Queue_Empty(serializer, down_queue)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->next->queue->head->data) )->seq == seq
                    )
                   )


            );
}

cond_t* down_cond(void * data)
{
    int seq = ((scheduler_data_t*)data)->seq;

    return ( Crowd_Empty(serializer, disk_access_crowd)
                &&(
                    Queue_Empty(serializer, up_queue) && Queue_Empty(serializer, down_queue)
                    ||
                    (
                        current_queue == down_queue
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->queue->head->data) )->seq== seq
                    )
                    ||
                    (
                        Queue_Empty(serializer, up_queue)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->next->queue->head->data) )->seq == seq
                    )
                   )


            );
}
