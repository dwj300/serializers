#include "serial.h"
#include <math.h>

#include "stdint.h"

int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id);

void Init_ds(int ncylinders, float CylinderSeekTime);

serial_t* serializer;
queue_t* up_queue;
queue_t* down_queue;
queue_t* current_queue;
crowd_t* disk_access_crowd;

uint32_t disk_access_sequence_number;

int cylinder_count;

int head_position;
cond_t* up_cond();
cond_t* down_cond();

typedef struct scheduler_data
{
    int seq;
    int tid;
    int seeked_cylinders;
    cond_t *func;
} scheduler_data_t;
