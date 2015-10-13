#include "dp_serial.h"

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

cond_t* eat_queue_cond(int tid)
{
	return forks[left(tid)] && forks[right(tid)];
}

void Eat(int phil_id, void *(*model_eat()))
{
	// Enter queue
	Serial_Enqueue(serializer, waiting_q, &eat_queue_cond, 0, phil_id);

	// Got the serializer, means we can eat.
	forks[left(phil_id)] = 0;
	forks[right(phil_id)] = 0;

	Serial_Join_Crowd(serializer, eating_crowd, model_eat, phil_id);

	Serial_Enqueue(serializer, waiting_q, &true_func, 0, phil_id);
	forks[left(phil_id)] = 1;
	forks[right(phil_id)] = 1;
	Serial_Exit(serializer);

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
	num_phil = nphilosophers;
	forks = malloc(nphilosophers * sizeof(int));
	for (int i = 0; i < nphilosophers; i++)
	{
		forks[i] = 1;
	}
}
