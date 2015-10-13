#include "dp_serial.h"

cond_t* eat_queue_cond()
{
	return 1 < 2;
}

void Eat(int phil_id, void *(*model_eat()))
{
	// Enter queue
	Serial_Enqueue(serializer, waiting_q, &eat_queue_cond, 0);
	Serial_Join_Crowd(serializer, eating_crowd, model_eat, phil_id);
}

void Think(int phil_id, void *(*model_think()))

{
	Serial_Join_Crowd(serializer, thinking_crowd, model_think, phil_id);
	Serial_Exit(serializer);
}

void Init_dp(int nphilosophers)
{
	serializer = Create_Serial();
	waiting_q = Create_Queue(serializer);
	thinking_crowd = Create_Crowd(serializer);
	eating_crowd = Create_Crowd(serializer);

	philosophers = malloc(nphilosophers * sizeof(int));
	for (int i = 0; i < nphilosophers; i++)
	{
		philosophers[i] = 0;
	}
}
