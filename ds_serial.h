#include "serial.h"
#include <math.h>

#include "stdint.h"

int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id);
void Init_ds(int ncylinders, float CylinderSeekTime);

bool GoingUp();
bool GoingDown();
int NextRequestInQueueBeingServed();
bool DiskIdle();


serial_t* serializer;
queue_t* up_queue;
queue_t* down_queue;
queue_t* current_queue;
crowd_t* disk_access_crowd;

int cylinder_count;

uint32_t disk_access_sequence_number;
uint32_t disk_request_sequence_number;
int head_position;

typedef struct RequestData
{
    int id;
    int seeked_cylinders;
    action_t *serviceRequest;
} request_data_t;
