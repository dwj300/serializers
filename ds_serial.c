/*
 * disk scheduler
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "ds_serial.h"


/*void Switch()
{
    if(current_queue == up_queue)
    {
        current_queue = down_queue;
        fprintf(stderr, "Now going down. Head: %d\n", head_position);
    }
    else
    {
        current_queue = up_queue;
        fprintf(stderr, "Now going up. Head: %d\n", head_position);
    }
}*/

bool GoingUp()
{
    return (serializer->queueBeingServed->queue == up_queue);
}

bool GoingDown()
{
    return !GoingUp();
}



void serviceRequest(data_t* data)
{
    //printf("Servicing id: %d\n", data->tid);
    data->func(data->seq, data->seeked_cylinders);
}



int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id)
{
/*
    fprintf(stderr, "Up Queue: ");
    PrintQueue(up_queue);
    fprintf(stderr, "\n");
    fprintf(stderr, "Down Queue: ");
    PrintQueue(down_queue);
    fprintf(stderr, "\n"); */
    char temp[100];

    uint32_t serviceNum = 0;
    //Get access to the serializer's synchronization constructs

    Serial_Enter(serializer);

    scheduler_data_t* newNodeData = (scheduler_data_t*)malloc(sizeof(scheduler_data_t));
    //serviceNum = newNodeData->seq = disk_access_sequence_number++;

    // PrintQueue(up_queue);
    // PrintQueue(down_queue);

    //bool scheduledInUp = false;

    //When a request is issued, we have to choose which queue to place the request into.
    //  If the target is 'coming up' in the direction of the queue being served, insert
    //  it into the queue being served, and vice-versa
    if( (GoingUp() && cylinderno >= head_position)) //Going up, target is ahead of us
    {
        //fprintf(stderr, "Scheduling request #%d for cylinder %d in the up queue\n", serviceNum, cylinderno);
        //sprintf(temp, "Scheduling request for cylinder %d in the up queue", cylinderno);
        //print(temp);
        //scheduledInUp = true;
        Serial_Enqueue(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno > head_position )
    {
        //fprintf(stderr, "Scheduling request for cylinder %d in the up queue\n", cylinderno);
        //sprintf(temp, "Scheduling request for cylinder %d in the up queue", cylinderno);
        //print(temp);
        //scheduledInUp = true;
        Serial_Enqueue(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno <= head_position )  //Going down, target behind us (approaching it)
    {
        //fprintf(stderr, "Scheduling request for cylinder %d in the down queue\n", cylinderno);
        //sprintf(temp, "Scheduling request for cylinder %d in the down queue", cylinderno);
        //print(temp);
        Serial_Enqueue(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else if( GoingUp() && cylinderno < head_position ) //Going up and target is behind us (getting farther)
    {
        //fprintf(stderr, "Scheduling request for cylinder %d in the down queue\n", cylinderno);
        //sprintf(temp, "Scheduling request for cylinder %d in the down queue", cylinderno);
        //print(temp);
        Serial_Enqueue(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else
    {
        print("Logic error");
        exit(-1);
    }

    serviceNum = disk_access_sequence_number++;
    /*if( (scheduledInUp && GoingDown()) || (!scheduledInUp && GoingDown()) )
    {
        Switch();
    }*/

    int oldPosition = head_position;

    data_t* newData = (data_t*)malloc(sizeof(data_t));
    newData->seq = serviceNum;
    newData->tid = id;
    seekedcylinders[id] = newData->seeked_cylinders = abs(head_position-cylinderno);
    newData->func = model_request;

    head_position = cylinderno;

    fprintf(stderr, "Head moved to %d for service. We moved %s from %d \n", head_position, GoingUp()? "upward" : "downward", oldPosition);

    //When we exit the queue, the request can be serviced
    Serial_Join_Crowd(serializer, disk_access_crowd, &serviceRequest, newData);

    //Request satisfied,
    //sprintf(temp, "Seeked %s from %d to %d", (going_up ? "downward" : "upward"), head_position, cylinderno);
    //print(temp);


    Serial_Exit(serializer);
    return serviceNum;
}

void Init_ds(int ncylinders, int CylinderSeekTime)
{
	// Init Serializer
	serializer = Create_Serial();
	cylinder_count = ncylinders;
	up_queue = Create_Queue(serializer);
	//current_queue = down_queue = Create_Queue(serializer);
	down_queue = Create_Queue(serializer);
	disk_access_crowd = Create_Crowd(serializer);
	disk_access_sequence_number = 0;
	head_position = 0;
}



cond_t* up_cond(void * data)
{
    int seq = ((scheduler_data_t*)data)->seq;
    return (Crowd_Empty(serializer, disk_access_crowd) && (serializer->queueBeingServed->queue == up_queue) && ((Queue_Empty(serializer, up_queue) || (((scheduler_data_t*)serializer->queueBeingServed->queue->head->data)->seq == seq))));

    return ( Crowd_Empty(serializer, disk_access_crowd)
                &&(
                    (Queue_Empty(serializer, up_queue) && Queue_Empty(serializer, down_queue))
                    ||
                    (
                        (current_queue == up_queue)
                        && (serializer->queueBeingServed->queue->head != NULL)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->queue->head->data) )->seq == seq
                    )
                    ||
                    (
                        Queue_Empty(serializer, down_queue)
                        && (serializer->queueBeingServed->next->queue->head != NULL)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->next->queue->head->data) )->seq == seq
                    )
                   )


            );
}

cond_t* down_cond(void * data)
{
    int seq = ((scheduler_data_t*)data)->seq;
    return (Crowd_Empty(serializer, disk_access_crowd) && (serializer->queueBeingServed->queue == down_queue) && ((Queue_Empty(serializer, down_queue) || (((scheduler_data_t*)serializer->queueBeingServed->queue->head->data)->seq == seq))));


    return ( Crowd_Empty(serializer, disk_access_crowd)
                && (
                    (Queue_Empty(serializer, up_queue) && Queue_Empty(serializer, down_queue))
                    ||
                    (
                        (current_queue == down_queue)
                        && (serializer->queueBeingServed->queue->head != NULL)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->queue->head->data) )->seq== seq
                    )
                    ||
                    (
                        Queue_Empty(serializer, up_queue)
                        && (serializer->queueBeingServed->next->queue->head != NULL)
                        && ( (scheduler_data_t*)(serializer->queueBeingServed->next->queue->head->data) )->seq == seq
                    )
                   )


            );
}
