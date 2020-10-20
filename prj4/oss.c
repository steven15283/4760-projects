#include "shared.h"

pcb_t* create_table(int);
simtime_t* create_sim_clock();
void create_msqueue();
int get_sim_pid(int*, int);
simtime_t get_next_process_time(simtime_t, simtime_t);
int rand_priority(int);
int should_spawn(int, simtime_t, simtime_t, int, int);
int check_blocked(int*, pcb_t*, int);

void oss(int);
int dispatch(int, int, int, simtime_t, int, int*);

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

/*delete shared memory, terminate children*/
void cleanup() 
{
    remove_shm();                   // remove shared memory
    msgctl(msqid, IPC_RMID, NULL);  // this deletes msgqueue
    kill(0, SIGTERM);               // terminate users/children
    return;
}

/*Remove the simulated clock and pcb table from shared memory*/
void remove_shm() 
{
    shmctl(clockId, IPC_RMID, NULL);
    shmctl(pcbTableId, IPC_RMID, NULL);
    return;
}

/*Create pcb table in shared memory for n processes*/
processControlBlock* create_table(int n) 
{
    processControlBlock* table;
    pcbTableId = shmget(processControlBlockABLE_KEY, sizeof(processControlBlock) * n, IPC_CREAT | 0777);
    if (pcbTableId < 0) 
    {  // error
        perror("./oss: Error: shmget ");
        cleanup();
    }
    table = shmat(pcbTableId, NULL, 0);
    if (table < 0) {  // error
        perror("./oss: Error: shmat ");
        cleanup();
    }
    return table;
}

/*Create a simulated clock in shared memory initialized to 0s0ns*/
simtime_t* create_sim_clock() 
{
    simtime_t* simClock;
    clockId = shmget(CLOCK_KEY, sizeof(simtime_t), IPC_CREAT | 0777);
    if (clockId < 0) 
    {  // error
        perror("./oss: Error: shmget ");
        cleanup();
    }
    simClock = shmat(clockId, NULL, 0);
    if (simClock < 0) {  // error
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

