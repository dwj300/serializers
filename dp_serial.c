#include "dp_serial.h"

//Returns the index (in the fork array) of the fork at the given philosopher's left hand
int leftHand(int philosopherNumber)
{
	return philosopherNumber;
}

//Returns the index (in the fork array) of the fork at the given philosopher's right hand
int rightHand(int philosopherNumber)
{
	return (philosopherNumber+1) % philosopher_count;
}

//Indicates whether a philosopher may eat
cond_t* eat_queue_cond(void* contendingThreadData)
{
	int phil_id = ((philosopher_thread_data_t*)contendingThreadData)->id;
     //((n % M) + M) % M
    int left = ((((phil_id-1)%nphil) + nphil) % nphil);
    int right = (phil_id+1)%nphil;
    return(Crowd_Empty(serializer, crowds[left]) && Crowd_Empty(serializer, crowds[right]));
}

//Performs the action specified for a philosopher in its data object
void perform_philosopher_task(philosopher_thread_data_t* philosopherData)
{
	philosopherData->task(philosopherData->id);
}

//Syncronizes a given philosopher's attempt to eat
void Eat(int phil_id, void *(*model_eat()))
{
    //Enter the serializer
	Serial_Enter(serializer);

	// Leave the serializer, enter the waiting queue with an intent to eat
	philosopher_thread_data_t* philosopherData = (philosopher_thread_data_t*)malloc(sizeof(philosopher_thread_data_t));
	philosopherData->id = phil_id;
	philosopherData->task = model_eat;
	Serial_Enqueue_Data(serializer, waiting_to_eat_queue, eat_queue_cond, 0, philosopherData);

	// After we leave the queue, we can pick up the forks and eat
	//forks[leftHand(phil_id)] = TAKEN;
	//forks[rightHand(phil_id)] = TAKEN;
	/*    if(phil_id % 2 == 0)
    {
	    Serial_Join_Crowd_Data(serializer, even_eating_crowd, perform_philosopher_task, philosopherData);
    }
    else
    {
	    Serial_Join_Crowd_Data(serializer, odd_eating_crowd, perform_philosopher_task, philosopherData);
    }*/
    // When we finish eating and re-enter the serializer, we give up the forks
	//forks[leftHand(phil_id)] = AVAILABLE;
	//forks[rightHand(phil_id)] = AVAILABLE;

	Serial_Join_Crowd_Data(serializer, crowds[phil_id], perform_philosopher_task, philosopherData);

    //Leave the serializer
	Serial_Exit(serializer);
}

//Allows a given philosopher to think, outside (in a hollow region of) the serializer
void Think(int phil_id, void *(*model_think()))
{
	// Prepare the philosopher to think
	philosopher_thread_data_t *philosopherData = (philosopher_thread_data_t*)malloc(sizeof(philosopher_thread_data_t));
	philosopherData->id = phil_id;
	philosopherData->task = model_think;

    //Join the thinking crowd
	Serial_Enter(serializer);
	Serial_Join_Crowd_Data(serializer, thinking_crowd, perform_philosopher_task, philosopherData);
	Serial_Exit(serializer);
}

//Inializes the dining philosophers serializer solution resources
void Init_dp(int nphilosophers)
{
    //Create the serializer, the queue in which philosophers will wait to eat, and the crowds in which they will eat and think
	serializer = Create_Serial();
	crowds = malloc(sizeof(crowd_t*)*nphilosophers);
	for (int i = 0; i < nphilosophers; i++)
	{
	    crowds[i] = Create_Crowd(serializer);
	}
	waiting_to_eat_queue = Create_Queue(serializer);
	thinking_crowd = Create_Crowd(serializer);
	thinking_crowd = Create_Crowd(serializer);
    nphil = nphilosophers;
    

    //even_eating_crowd = Create_Crowd(serializer);
	//odd_eating_crowd = Create_Crowd(serializer);

	//Set up forks array
	//philosopher_count = nphilosophers;
	//forks = malloc(philosopher_count * sizeof(int));
}
