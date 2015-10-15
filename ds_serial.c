/*
 * disk scheduler
 */

#include <unistd.h>
#include "ds_serial.h"

bool GoingUp()
{
    return (serializer->queueBeingServed->queue == up_queue);
}

bool GoingDown()
{
    return !GoingUp();
}

void serviceRequest(scheduler_data_t* data)
{
    data->func(data->seq, data->seeked_cylinders);
}

int Disk_Request(int cylinderno, void* model_request, int* seekedcylinders, int id)
{
    uint32_t serviceNum = 0;
    //Get access to the serializer's synchronization constructs

    Serial_Enter(serializer);

    scheduler_data_t* newNodeData = (scheduler_data_t*)malloc(sizeof(scheduler_data_t));
    newNodeData->seq = disk_access_sequence_number;

    // When a request is issued, we have to choose which queue to place the request into.
    // If the target is 'coming up' in the direction of the queue being served, insert
    // it into the queue being served, and vice-versa
    if( (GoingUp() && cylinderno >= head_position)) //Going up, target is ahead of us
    {
        Serial_Enqueue_Data(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno > head_position )
    {
        Serial_Enqueue_Data(serializer, up_queue, up_cond, cylinder_count-cylinderno, newNodeData); //When going up, requests that are lower on disk should have a higher priority
    }
    else if( GoingDown() && cylinderno <= head_position )  //Going down, target behind us (approaching it)
    {
        Serial_Enqueue_Data(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else if( GoingUp() && cylinderno < head_position ) //Going up and target is behind us (getting farther)
    {
        Serial_Enqueue_Data(serializer, down_queue, down_cond, cylinderno, newNodeData);
    }
    else
    {
        exit(-1);
    }

    serviceNum = disk_access_sequence_number++;

    scheduler_data_t* newData = (scheduler_data_t*)malloc(sizeof(scheduler_data_t));

    newData->seq = serviceNum;
    newData->tid = id;
    newData->func = model_request;

    seekedcylinders[id] = newData->seeked_cylinders = abs(head_position-cylinderno);

    head_position = cylinderno;

    //When we exit the queue, the request can be serviced
    Serial_Join_Crowd_Data(serializer, disk_access_crowd, &serviceRequest, newData);

    Serial_Exit(serializer);
    return serviceNum;
}

void Init_ds(int ncylinders, float CylinderSeekTime)
{
    // Init Serializer
    serializer = Create_Serial();
    cylinder_count = ncylinders;
    up_queue = Create_Queue(serializer);
    down_queue = Create_Queue(serializer);
    disk_access_crowd = Create_Crowd(serializer);
    disk_access_sequence_number = 0;
    head_position = 0;
}

cond_t* up_cond(void* data)
{
    int seq = ((scheduler_data_t*)data)->seq;
    return (Crowd_Empty(serializer, disk_access_crowd) 
        && (serializer->queueBeingServed->queue == up_queue) 
        && ((Queue_Empty(serializer, up_queue) || (((scheduler_data_t*)serializer->queueBeingServed->queue->head->data)->seq == seq))));
}

cond_t* down_cond(void* data)
{
    int seq = ((scheduler_data_t*)data)->seq;
    return (Crowd_Empty(serializer, disk_access_crowd)
        && (serializer->queueBeingServed->queue == down_queue) && 
        ((Queue_Empty(serializer, down_queue) || (((scheduler_data_t*)serializer->queueBeingServed->queue->head->data)->seq == seq))));
}
