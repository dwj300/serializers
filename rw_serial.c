/*
 * r/w serializer
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "rw_serial.h"

long* read_data()
{
	int key = rand() % SIZE;
	usleep(rand() % 10 * DELAY);
	return (long *) database[key];
}

void* write_data()
{
	int key = rand() % SIZE;
	usleep(rand() % 10 * DELAY);
	database[key] = key;
	return NULL;
}

cond_t* read_queue_cond()
{
	return Crowd_Empty(serializer, writers_crowd);
}

cond_t* write_queue_cond()
{
	return (Crowd_Empty(serializer, readers_crowd) &&
		Crowd_Empty(serializer, writers_crowd));
}

void create()
{
	// Init database
	int i;
	srand(time(NULL));
	for (i = 0; i < SIZE; i++)
		database[i] = rand();
	// Init Serializer
	serializer = Create_Serial();
	waiting_q = Create_Queue(serializer);
	readers_crowd = Create_Crowd(serializer);
	writers_crowd = Create_Crowd(serializer);
}

void *read_func(void *id)
{
	long tid;
	tid = (long)id;
	Serial_Enter(serializer);
	char str[128];
	sprintf(str, "Reader thread #%ld starts!", tid);
	print(str);
	//print("Reader thread #%ld starts! (%d)\n", tid, pthread_self());
	Serial_Enqueue(serializer, waiting_q, &read_queue_cond);
	Serial_Join_Crowd(serializer, readers_crowd,(void *) (&read_data));
	Serial_Exit(serializer);
	char s[128];
	sprintf(s, "Reader thread #%ld ends!", tid);
	print(s);
	//printf("Reader thread #%ld ends!\n", tid);
	pthread_exit(NULL);
}

void *write_func(void *id)
{
	long tid;
	tid = (long)id;
	Serial_Enter(serializer);
	char str[128];
	sprintf(str, "Writer thread #%ld starts!", tid);
	print(str);
	//printf("Writer thread #%ld starts! (%d)\n", tid, pthread_self());
	Serial_Enqueue(serializer, waiting_q, &write_queue_cond);
	Serial_Join_Crowd(serializer, writers_crowd, &write_data);
	Serial_Exit(serializer);
	//printf("Writer thread #%ld ends!\n", tid);
	char s[128];
	sprintf(s, "Writer thread #%ld ends!", tid);
	print(s);
	pthread_exit(NULL);
}
