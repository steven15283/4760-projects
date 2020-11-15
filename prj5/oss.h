//steven guo 
//11/05/20
#ifndef OSS_H
#define OSS_H

#define TRUE 1
#define FALSE 0
#include <stdio.h>
#include <sys/types.h>
//shared memory keys and IDs
int descriptorID;
const key_t DESCRIPTOR_KEY = 110594;
int simClockID;
const key_t SIM_CLOCK_KEY = 110197;
//mesage Queue key and ID
int msqid;
const key_t MSG_Q_KEY = 052455;
//log file pointer and verbose printing option
FILE* logFile;
int verbose = FALSE;//default verbose set to false

//possible values for msg action
//sent from child
const int request = 0;//resource request
const int release = 1;//resource release
const int terminate = 2;
//sent from oss
const int granted = 3;//resource granted
const int denied = 4;//wait for release 

typedef struct 
{
	long mtype;
	int rid;//resource id [0-19]
	int action;//0 = request, 1 = release
	int pid;
	int sender;//process id of the sender
} msg_t;

typedef struct 
{
	unsigned int s;//seconds
	unsigned int ns;//nanoseconds
} simtime_t;

//increment given simulated time t by given increment inc
void increment_sim_time(simtime_t* t, int inc) 
{
    t->ns += inc;
    if (t->ns >= 1000000000) 
    {
        t->ns -= 1000000000;
        t->s += 1;
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
    simtime_t sum = { .s = a.s + b.s,.ns = a.ns + b.ns };
    if (sum.ns >= 1000000000)
    {
        sum.ns -= 1000000000;
        sum.s += 1;
    }
    return sum;
}
//returns 1 if a <= b
//0 otherwise
int less_or_equal_sim_times(simtime_t a, simtime_t b) 
{
    if (a.s > b.s) 
    {
        return 0;
    }
    else if (a.s == b.s && a.ns > b.ns) 
    {
        return 0;
    }
    return 1;
}

#endif