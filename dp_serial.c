#include "dp_serial.h"

int left(int i)
{
	return i;
}

int right(int i)
{
	return (i+1) % num_phil;
}

cond_t* eat_queue_cond(void* data)
{
	int tid = ((data_t*)data)->tid;
	return forks[left(tid)] && forks[right(tid)];
}

void wrapper(data_t* data)
{
	data->func(data->tid);
}

void Eat(int phil_id, void *(*model_eat()))
{
	Serial_Enter(serializer);
	data_t* data = (data_t*)malloc(sizeof(data_t));
	data->tid = phil_id;
	data->func = model_eat;

	// Enter queue
	Serial_Enqueue_Data(serializer, waiting_q, eat_queue_cond, 0, data);
	// Got the serializer, means we can eat.
	forks[left(phil_id)] = 0;
	forks[right(phil_id)] = 0;

	Serial_Join_Crowd_Data(serializer, eating_crowd, wrapper, data);

	forks[left(phil_id)] = 1;
	forks[right(phil_id)] = 1;

	Serial_Exit(serializer);
}

void Think(int phil_id, void *(*model_think()))
{
	data_t *data = malloc(sizeof(data_t));
	data->tid = phil_id;
	data->func = model_think;

	Serial_Enter(serializer);
	Serial_Join_Crowd_Data(serializer, thinking_crowd, wrapper, data);
	Serial_Exit(serializer);
}

void Init_dp(int nphilosophers)
{
	serializer = Create_Serial();
	waiting_q = Create_Queue(serializer);
	thinking_crowd = Create_Crowd(serializer);
	eating_crowd = Create_Crowd(serializer);
	num_phil = nphilosophers;
	forks = malloc(nphilosophers * sizeof(int));
	for (int i = 0; i < nphilosophers; i++)
	{
		forks[i] = 1;
	}
}
