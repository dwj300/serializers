#include "serial.h"
#include <stdlib.h>

serial_t* Create_Serial()
{
    return NULL;
}

Serial_Enter(serial_t* serial)
{
    return NULL;
}

Serial_Exit(serial_t* serial)
{
    return NULL;
}

queue_t* Create_Queue(serial_t* serial)
{
    return NULL;
}

crowd_t* Create_Crowd(serial_t* serial)
{
    return NULL;
}

int Queue_Empty(serial_t* serial, queue_t* queue)
{
    return -1;
}

int Crowd_Empty(serial_t* serial, crowd_t* crowd)
{
    return -1;
}

Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t* func)
{
    return NULL;
}

Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func)
{
    return NULL;
}

