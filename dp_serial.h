#include "serial.h"
#define TAKEN 0
#define AVAILABLE 1

//void *(*start_routine)(void *)
void Init_dp(int nphilosophers);
void Eat(int phil_id, void *(*model_eat()));
void Think(int phil_id, void *(*model_think()));

serial_t* serializer;
queue_t* waiting_to_eat_queue;
crowd_t* eating_crowd;
crowd_t* thinking_crowd;
int* forks;
int philosopher_count;

typedef struct PhilosopherThreadData
{
    action_t* task;
    int id;
} philosopher_thread_data_t;
