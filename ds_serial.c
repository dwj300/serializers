#include <unistd.h>
#include "ds_serial.h"

//Indicates if a request inside (or aimed at) the upwards queue should be next to service
cond_t* ready_to_service_from_up_queue(void* request)
{
    int requestID = ((request_data_t*)request)->id;

    return ( DiskIdle() && GoingUp() && (NextRequestInQueueBeingServed() == requestID || Queue_Empty(serializer, up_queue)) );
}

//Indicates if a request inside (or aimed at) the downwards queue should be next to service
cond_t* ready_to_service_from_down_queue(void* request)
{
    int requestID = ((request_data_t*)request)->id;

    return ( DiskIdle() && GoingDown() && (NextRequestInQueueBeingServed() == requestID || Queue_Empty(serializer, down_queue)) );
}

//Returns the request ID (the scheduling sequence number) for the request at the head of the queue the serializer is serving
int NextRequestInQueueBeingServed()
{
    if(!serializer->queueBeingServed->queue->head)
        return -1;
    else
        return ((request_data_t*)serializer->queueBeingServed->queue->head->data)->id;
}

//Indicates if the disk is idle (if no requests are being served)
bool DiskIdle()
{
    return Crowd_Empty(serializer, disk_access_crowd);
}

//Indicates if the disk head is heading upwards
bool GoingUp()
{
    return (serializer->queueBeingServed->queue == up_queue);
}

//Indicates if the disk head is heading downwards
bool GoingDown()
{
    return !GoingUp();
}

// Execute the request's servicing routine
void accessDisk(request_data_t* request)
{
    request->serviceRequest(request->id, request->seeked_cylinders);
}

// Process a request to access the disk
int Disk_Request(int cylinderno, void* model_request, int* seekedcylinders, int id)
{
    //Get access to the serializer's synchronization constructs
    Serial_Enter(serializer);

    //Encapsulate the new request for servicing
    request_data_t* newRequest = (request_data_t*)malloc(sizeof(request_data_t));
    newRequest->id = disk_request_sequence_number++;
    newRequest->serviceRequest = model_request;

    // When a request is issued, we have to choose which queue to place the request into.
    // If the target is 'coming up' in the direction of the queue being served, insert
    // it into the queue being served, and vice-versa
    if( (GoingUp() && cylinderno >= head_position)) //The head is going up and the target is above it (approaching the target)
    {
        //When going up, requests that are lower on disk should have a higher priority
        Serial_Enqueue_Data(serializer, up_queue, ready_to_service_from_up_queue, cylinder_count-cylinderno, newRequest);
    }
    else if( GoingDown() && cylinderno > head_position ) //The head is going down and the target is above it (getting farther away from the target)
    {
        Serial_Enqueue_Data(serializer, up_queue, ready_to_service_from_up_queue, cylinder_count-cylinderno, newRequest);
    }
    else if( GoingDown() && cylinderno <= head_position ) //The head is going down and the target is below it (approaching the target)
    {
        Serial_Enqueue_Data(serializer, down_queue, ready_to_service_from_down_queue, cylinderno, newRequest);
    }
    else if( GoingUp() && cylinderno < head_position ) //The head is going up and target is below it (getting farther away from the target)
    {
        Serial_Enqueue_Data(serializer, down_queue, ready_to_service_from_down_queue, cylinderno, newRequest);
    }
    else
    {
        //Program fault
        exit(-1);
    }

    //When we are about to be serviced, find out what our place in the service order will be
    uint32_t serviceSeqNum = disk_access_sequence_number++;

    //Find how far we're moving the head to service this request
    seekedcylinders[id] = newRequest->seeked_cylinders = abs(head_position-cylinderno);

    //Update the head position to this request's target
    head_position = cylinderno;

    //Exit the queue and access the disk
    Serial_Join_Crowd_Data(serializer, disk_access_crowd, &accessDisk, newRequest);

    //Release the serializer and the request resources
    Serial_Exit(serializer);
    free(newRequest);

    return serviceSeqNum;
}

//Initializes the disk scheduler
void Init_ds(int ncylinders, float CylinderSeekTime)
{
    // Instantiate the serializer, queues for requests in the upward and downward direction, and a
    //   crowd to encapsulate active requests
    serializer = Create_Serial();
    up_queue = Create_Queue(serializer);
    down_queue = Create_Queue(serializer);
    disk_access_crowd = Create_Crowd(serializer);

    //Initialize the disk's state variables
    cylinder_count = ncylinders;
    disk_access_sequence_number = 1;
    disk_request_sequence_number = 1;
    head_position = 0;
}
