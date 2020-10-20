#pragma once
#ifndef SHARED_H
#define SHARED_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

typedef struct 
{
	unsigned int head;
	unsigned int tail;
	unsigned int size;
	unsigned int items;
	int* data;
} queueTable;

queueTable* create_queue(int size)// initialize queue which is to replicate vector
{
	queueTable* queue = (queueTable*)malloc(sizeof(queueTable));
	queue->head = 0;
	queue->tail = 0;
	queue->size = size;
	queue->data = (int*)malloc(size * sizeof(int));
	queue->items = 0;
	int i;//loop iterator
	for (i = 0; i < size; i++)
		queue->data[i] = -1;//set all elements to -1. no pids will be -1
	return queue;
}

void enqueue(queueTable* queue, int pid)//enqueue for vector replication
{
	queue->data[queue->tail] = pid;//add pid to queue
	queue->tail = (queue->tail + 1) % queue->size;
	queue->items += 1;
	return;
}

int dequeue(queueTable* queue)//dequeue for vector replication
{
	int pid = queue->data[queue->head];
	queue->head = (queue->head + 1) % queue->size;
	queue->items -= 1;
	return pid;
}

typedef struct
{
	unsigned int second;
	unsigned int nanosecond;
} simtime_t;

struct pcb_t
{
	int pid;//simulated process id, range is [0,18]
	int priority;//Process priority
	int isReady;//if the process is ready to run
	simtime_t arrivalTime;//Arrivial time
	simtime_t cpuTime;//CPU time used
	simtime_t sysTime;//Time in the system
	simtime_t burstTime;//Time used in the last burst
	simtime_t waitTime;//Total sleep time. time waiting for an event
};


typedef struct
{
	long messageType;
	int messageValue;
} messageQueue;

void increment_sim_time(simtime_t* simTime, int increment)
{
	simTime->ns += increment;
	if (simTime->ns >= 1000000000)
	{
		simTime->ns -= 1000000000;
		simTime->s += 1;
	}
}
// returns a - b
simtime_t subtract_sim_times(simtime_t a, simtime_t b)
{
	simtime_t diff = { .s = a.s - b.s, .ns = a.ns - b.ns };
	if (diff.ns < 0)
	{
		diff.ns += 1000000000;
		diff.s -= 1;
	}
	return diff;
}
//returns a + b
simtime_t add_sim_times(simtime_t a, simtime_t b)
{
	simtime_t sum = { .s = a.s + b.s, .ns = a.ns + b.ns };
	if (sum.ns >= 1000000000)
	{
		sum.ns -= 1000000000;
		sum.s += 1;
	}
	return sum;
}

//returns simtime / divisor
simtime_t divide_sim_time(simtime_t simTime, int divisor)
{
	simtime_t quotient = { .s = simTime.s / divisor, .ns = simTime.ns / divisor };
	return quotient;
}

pcb_t create_pcb(int priority, int pid, simtime_t currentTime)
{
	pcb_t pcb =
	{
		.pid = pid,
		.priority = priority,
		.isReady = TRUE,
		.arrivalTime = {.s = currentTime.s, .ns = currentTime.ns},
		.cpuTime = {.s = 0, .ns = 0},
		.sysTime = {.s = 0, .ns = 0},
		.burstTime = {.s = 0, .ns = 0},
		.waitTime = {.s = 0, .ns = 0}
	};
	return pcb;
}

int pcbTableId;//shmid for PCB Table
int clockId;//shmid for simulated clock
int msqId;//id for message queue
#endif
