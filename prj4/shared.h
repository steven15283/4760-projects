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
} simulationTime;

struct ProcessControlBlock
{
	int pid;//simulated process id, range is [0,18]
	int priority;//Process priority
	int isReady;//if the process is ready to run
	simulationTime arrivalTime;//Arrivial time
	simulationTime cpuTime;//CPU time used
	simulationTime sysTime;//Time in the system
	simulationTime burstTime;//Time used in the last burst
	simulationTime waitTime;//Total sleep time. time waiting for an event
};


typedef struct
{
	long messageType;
	int messageValue;
} messageQueue;

void increment_sim_time(simulationTime* simTime, int increment)
{
	simTime->ns += increment;
	if (simTime->ns >= 1000000000)
	{
		simTime->ns -= 1000000000;
		simTime->s += 1;
	}
}
// returns a - b
simulationTime subtract_sim_times(simulationTime a, simulationTime b)
{
	simulationTime diff = { .s = a.s - b.s, .ns = a.ns - b.ns };
	if (diff.ns < 0)
	{
		diff.ns += 1000000000;
		diff.s -= 1;
	}
	return diff;
}
//returns a + b
simulationTime add_sim_times(simulationTime a, simulationTime b)
{
	simulationTime sum = { .s = a.s + b.s, .ns = a.ns + b.ns };
	if (sum.ns >= 1000000000)
	{
		sum.ns -= 1000000000;
		sum.s += 1;
	}
	return sum;
}

//returns simtime / divisor
simulationTime divide_sim_time(simulationTime simTime, int divisor)
{
	simulationTime quotient = { .s = simTime.s / divisor, .ns = simTime.ns / divisor };
	return quotient;
}

ProcessControlBlock create_pcb(int priority, int pid, simulationTime currentTime)
{
	ProcessControlBlock pcb =
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
