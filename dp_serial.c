#include "dp_serial.h"
#include <stdio.h>


int left(int i)
{
	return i;
}

int right(int i)
{
	return (i+1) % num_phil;
}

int true_func()
{
	return 1;
}

int eat_queue_cond(data_t *data)
{
	int tid = data->tid;
	// printf("tid: %d\n", tid);
	//fprintf(stderr, "thread %d in eat queue cond: %d\n", tid, (forks[left(tid)] && forks[right(tid)]));
	return forks[left(tid)] && forks[right(tid)];
}

void Eat(int phil_id, void *(*model_eat()))
{
	Serial_Enter(serializer);
	data_t *data = malloc(sizeof(data_t));
	data->tid = phil_id;
	// Enter queue
	Serial_Enqueue(serializer, waiting_q, &eat_queue_cond, 0, data);
	//printf("%d gots a serializer\n", phil_id);
	// Got the serializer, means we can eat.
	forks[left(phil_id)] = 0;
	forks[right(phil_id)] = 0;

	Serial_Join_Crowd(serializer, eating_crowd, model_eat, data);

	//Serial_Enqueue(serializer, waiting_q, &eat_queue_cond, phil_id);
	forks[left(phil_id)] = 1;
	forks[right(phil_id)] = 1;
	Serial_Exit(serializer);
}

void Think(int phil_id, void *(*model_think()))
{
	//Serial_Exit(serializer);

	data_t *data = malloc(sizeof(data_t));
	data->tid = phil_id;

	Serial_Enter(serializer);
	Serial_Join_Crowd(serializer, thinking_crowd, model_think, data);
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
