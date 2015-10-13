#include "serial.h"
#include "stdint.h"

int Disk_Request(int cylinderno, void* model_request, int * seekedcylinders, int id);

void Init_ds(int ncylinders, int CylinderSeekTime);

serial_t* serializer;
queue_t* up_queue;
queue_t* down_queue;
crowd_t* disk_access_crowd;

uint32_t disk_access_sequence_number;

int cylinder_count;

int just_finished;
cond_t* up_cond();
cond_t* down_cond();