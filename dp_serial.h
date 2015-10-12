#include "serial.h"

void Init_dp(int nphilosophers); 
void Eat(int phil_id, (void *)model_eat()); 
void Think(int phil_id, (void *)model_think());

serial_t* serializer;
queue_t* waiting_q;
crowd_t* eating_crowd;
crowd_t* thinking_crowd;
int *philosophers;
