#include "shared.h"

int main(int argc, char* argv[])
{
	int maxProcesses = 18;// max amount of processes allowed in the system at one time

	srand(time(0));// seed rand
	printf("----------Starting Simulation----------\n");
	oss(n);
	printf("----------Simulation Complete----------\n");
	msgctl(msqid, IPC_RMID, NULL);  //delete msgqueue
	remove_shm();
	return 0;

}

/*SIGALRM handler*/
static void time_out() 
{
	fprintf(stderr, "3 second timeout\n");
	fprintf(stderr, "Killing user/child processes\n");
	cleanup();
	exit(EXIT_SUCCESS);
}

void cleanup() 
{
	remove_shm();                   // remove shared memory
	msgctl(msqid, IPC_RMID, NULL);  // this deletes msgqueue
	kill(0, SIGTERM);               // terminate users/children
	return;
}

void remove_shm() 
{
	shmctl(clockId, IPC_RMID, NULL);
	shmctl(pcbTableId, IPC_RMID, NULL);
	return;
}

ProcessControlBlock* create_table(int n) 
{
	ProcessControlBlock* table;
	pcbTableId = shmget(PCB_TABLE_KEY, sizeof(ProcessControlBlock) * n, IPC_CREAT | 0777);
	if (pcbTableId < 0) {  // error
		perror("./oss: Error: shmget ");
		cleanup();
	}
	table = shmat(pcbTableId, NULL, 0);
	if (table < 0) 
	{
		perror("./oss: Error: shmat ");
		cleanup();
	}
	return table;
}

simtime_t* create_sim_clock() 
{
	simtime_t* simClock;
	clockId = shmget(CLOCK_KEY, sizeof(simtime_t), IPC_CREAT | 0777);
	if (clockId < 0) 
	{  
		perror("./oss: Error: shmget ");
		cleanup();
	}
	simClock = shmat(clockId, NULL, 0);
	if (simClock < 0) 
	{  
		perror("./user: Error: shmat ");
		cleanup();
	}
	simClock->s = 0;
	simClock->ns = 0;
	return simClock;
}

/*Create a message queue*/
void create_msqueue() 
{
	msqid = msgget(MSG_KEY, 0666 | IPC_CREAT);
	if (msqid < 0) 
	{
		perror("./oss: Error: msgget ");
		cleanup();
	}
	return;
}