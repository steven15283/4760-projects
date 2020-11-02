#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "oss.h"
#include "queue.h"

static void time_out();
void cleanup();
void remove_shm();

pcb_t* create_table(int);
simtime_t* create_sim_clock();
void create_msqueue();

int get_sim_pid(int*, int);
simtime_t get_next_process_time(simtime_t, simtime_t);
int rand_priority(int);
int should_spawn(int, simtime_t, simtime_t, int, int);
int check_blocked(int*, pcb_t*, int);
void oss(int);
int dispatch(int, int, int, simtime_t, int);

int main(int argc, char* argv[]) 
{
    int n = 20;// max amount of processes allowed in the system at one time
    // Open log file
    logFile = fopen("ossLog.txt", "w");
    if (logFile == NULL) 
    {  // error opening file
        perror("error opening file");
        cleanup();
    }
    // Terminate after 3s
    signal(SIGALRM, time_out);
    alarm(20);
    srand(time(0));// seed rand
    printf("Starting Scheduling\n");
    oss(n);
    printf("Scheduling Complete\n");

    // safe cleanup
    msgctl(msqid, IPC_RMID, NULL);  //delete msgqueue
    remove_shm();
    fclose(logFile);  // close log

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
    fclose(logFile);                // close log file
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
pcb_t* create_table(int n) 
{
    pcb_t* table;
    pcbTableId = shmget(PCB_TABLE_KEY, sizeof(pcb_t) * n, IPC_CREAT | 0777);
    if (pcbTableId < 0) 
    {  // error
        perror("./oss: Error: shmget ");
        cleanup();
    }
    table = shmat(pcbTableId, NULL, 0);
    if (table < 0) 
    {  // error
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
    if (simClock < 0) 
    {  // error
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
/*Return a pid if there is one available, returns -1 otherwise*/
int get_sim_pid(int* pids, int pidsCount) 
{
    int i;
    for (i = 0; i < pidsCount; i++) 
    {
        if (pids[i]) // sim pid i is available
        {  
            return i;     // return available pid
        }
    }
    return -1;  // no available pids
}
/*return a random simtime in range [0, max] + current time*/
simtime_t get_next_process_time(simtime_t max, simtime_t currentTime) 
{
    simtime_t nextTime = { .ns = (rand() % (max.ns + 1)) + currentTime.ns,
                          .s = (rand() % (max.s + 1)) + currentTime.s };
    if (nextTime.ns >= 1000000000) 
    {
        nextTime.s += 1;
        nextTime.ns -= 1000000000;
    }
    return nextTime;
}
/* Returns 0 chance % of the time on average. returns 1 otherwise*/
int rand_priority(int chance) 
{
    if ((rand() % 101) <= chance)
        return 0;
    else
        return 1;
}
/* Takes in generation criteria
 * returns 0 if any of the generation criteria fail
 * returns 1 otherwise
 * note: 2 ifs for time because of an edge case where next process spawns at
 * at a high ns value. It would be rare for the s and ns to of the sim clock
 * to be higher than the next process time
 * eg with just s >= s and ns >= ns, 2s0ns >= 1s9ns is false
 * with also checking s > s 2s0ns >= 1s9ns is true*/
int should_spawn(int pid, simtime_t next, simtime_t now, int generated, int max) 
{
    if (generated >= max)  // generated too many processes
        return FALSE;
    if (pid < 0)  // no available pids
        return FALSE;
    // not time for next process
    if (next.s > now.s)
        return FALSE;
    // more specific not time for next process
    if (next.s >= now.s && next.ns > now.ns)
        return FALSE;

    return TRUE;
}

int check_blocked(int* blocked, pcb_t* table, int count) 
{
    int i; //loop iterator
    for (i = 0; i < count; i++) 
    {
        if (blocked[i] == TRUE) 
        {
            if (table[i].isReady == TRUE) 
            {
                return i;
            }
        }
    }
    return -1;
}
/*Dispatches a process by sending a message witht he appropriate time quantum to
 * the given pid
 * waits to recieve a response and returns that response */
int dispatch(int pid, int priority, int msqid, simtime_t currentTime, int quantum) 
{
    mymsg_t msg;// message to be sent
    quantum = quantum * pow(2.0, (double)priority);  // i = queue #, slice = 2^i * quantum
    msg.mtype = pid + 1;// pids recieve messages of type (their pid) + 1
    msg.mvalue = quantum;
    fprintf(logFile, "OSS:Dispatch PID: %3d Queue: %d TIME: %ds%09dns\n", pid, priority, currentTime.s, currentTime.ns);
    // send the message
    if (msgsnd(msqid, &msg, sizeof(msg.mvalue), 0) == -1)
    {
        perror("./oss: Error: msgsnd ");
        cleanup();
    }
    // immediately wait for the response
    if ((msgrcv(msqid, &msg, sizeof(msg.mvalue), pid + 100, 0)) == -1) 
    {
        perror("./oss: Error: msgrcv ");
        cleanup();
    }
    return msg.mvalue;
}
/**OSS: Operating System Simulator**/
void oss(int maxProcesses) 
{
    /*Scheduling structures*/
    pcb_t* table;// Process control block table
    simtime_t* simClock;// simulated system clock
    queue_t* rrQueue; // round robin queue
    queue_t* queue1; // highest level of mlfq
    queue_t* queue2; // mid level
    queue_t* queue3;  // lowest level
    //queue_t* exQueue0; // expired round robin queue-highest level
    //queue_t* exQueue1; // expired high level
    queue_t* exQueue2; // expired mid level
    queue_t* exQueue3;  // expired lowest level
    int blockedPids[maxProcesses];    //blocked "queue"
    int availablePids[maxProcesses];  // pseudo bit vector of available pids

    int schedInc = 1000;// scheduling overhead
    int idleInc = 100000;// idle increment
    int blockedInc = 1000000; // blocked queue check increment
    int quantum = 10000000; // base time quantum
    int burst; // actual time used by the process
    int response; // response from process. will be percentage of quantum used

    simtime_t maxTimeBetweenNewProcesses = { .s = 0, .ns = 500000000 };// rand() %
    simtime_t nextProcess; // holds the time we should spawn the next process
    int terminated = 0; // counter of terminated processes
    int generated = 0; // counter of generated processes
    int maxTotalProcesses = 20; // how many processes to generate total
    int simPid; // holds a simulated pid
    int priority;  // holds priority of a process
    char simPidArg[3]; // exec arg 2. simulated pid as a string
    char msqidArg[10];// exec arg 3. msqid as string
    char quantumArg[10];
    sprintf(quantumArg, "%d", quantum);

    int i = 0;  // loop iterator
    int pid;    // holds wait() and fork() return values

    /*Statistics*/
    simtime_t totalCPU = { .s = 0, .ns = 0 };
    simtime_t totalSYS = { .s = 0, .ns = 0 };
    simtime_t totalIdle = { .s = 0, .ns = 0 };
    simtime_t totalWait = { .s = 0, .ns = 0 };
    double avgCPU = 0.0;
    double avgSYS = 0.0;
    double avgWait = 0.0;

    /*Setup*/
    table = create_table(maxProcesses);// setup shared pcb table
    simClock = create_sim_clock(maxProcesses);  // setup shared simulated clock
    rrQueue = create_queue(maxProcesses);// setup round robin queue
    queue1 = create_queue(maxProcesses);// setup MLFQ. highest level/priority
    queue2 = create_queue(maxProcesses);// mid level/priority
    queue3 = create_queue(maxProcesses);// lowest level/priority
    //exQueue0 = create_queue(maxProcesses);// setup expired highest level/priority
    //exQueue1 = create_queue(maxProcesses);// setup expired high level/priority
    exQueue2 = create_queue(maxProcesses);// setup expired mid level/priority
    exQueue3 = create_queue(maxProcesses);// setup expired lowest level/priorty

    create_msqueue();                           // set up message queue.sets global msqid variable
    sprintf(msqidArg, "%d", msqid);             // write msqid to msqid string arg
    // Loop through available pids to set all to TRUE(available)
    for (i = 0; i < maxProcesses; i++) 
    {
        blockedPids[i] = FALSE;//set blocked "queue" to empty
        availablePids[i] = TRUE;
    }
    // get time to spawn next process
    nextProcess = get_next_process_time(maxTimeBetweenNewProcesses, (*simClock));

    while (terminated < maxTotalProcesses) 
    {
        //test
        /*
         if (queue3->items > 0 && queue1->items > 0 && queue2->items > 0)
           printf("3 queues occupied\n");
         if (queue3->items > 0 && queue1->items > 0)
           printf("3 & 1 occupied\n");
         if (queue3->items > 0 && queue2->items > 0)
           printf("3 & 2 occupied\n");
         if (queue2->items > 0 && queue1->items > 0)
           printf("2 & 1 occupied\n");

        */
        printf("terminated:%d simpid:%d", terminated, simPid);
        // check for an available pid
        simPid = get_sim_pid(availablePids, maxProcesses);
        printf("simPid:%d\n", simPid);
        //printf("%d <= %d && %d <= %d\n", nextProcess.s, simClock->s, nextProcess.ns, simClock->ns);
        if (should_spawn(simPid, nextProcess, (*simClock), generated, maxTotalProcesses)) 
        {
             printf("spawn\n");
            // pid was returned so spawn a new process
            sprintf(simPidArg, "%d", simPid);  // write simpid to simpid string arg
            availablePids[simPid] = FALSE;// set pid to unavailable
            // get random priority(0:real time or 1:user)
            priority = rand_priority(5);
            fprintf(logFile, "OSS:Generating process PID %d in queue %d at %ds%09dns\n", simPid, priority, simClock->s, simClock->ns);
            // create pcb for new process at available pid
            table[simPid] = create_pcb(priority, simPid, (*simClock));
            // queue in round robin if real-time(priority == 0)
            if (priority == 0)
                enqueue(rrQueue, simPid);
            else  // queue in MLFQ otherwise
                enqueue(queue1, simPid);
            // fork a new process
            pid = fork();
            if (pid < 0) 
            {  // error
                perror("./oss: Error: fork ");
                cleanup();
            }
            else if (pid == 0) 
            {  // child
                execl("./user", "user", simPidArg, msqidArg, quantumArg, (char*)NULL);
            }
            // parent
            generated += 1;  // increment generated processes counter
            nextProcess = get_next_process_time(maxTimeBetweenNewProcesses, (*simClock));
            increment_sim_time(simClock, 10000);
        }
        /* Check blocked queue */
        else if ((simPid = check_blocked(blockedPids, table, maxProcesses)) >= 0) 
        {
            blockedPids[simPid] = FALSE;//remove from blocked "queue"
            fprintf(logFile, "OSS: Unblocked. PID: %3d TIME: %ds%09dns\n", simPid, simClock->s, simClock->ns);
            if (table[simPid].priority == 0) 
            {
                fprintf(logFile, "OSS: PID: %3d goes into  Round Robin Queue\n", simPid);
                enqueue(rrQueue, simPid);
            }
            else {
                fprintf(logFile, "OSS: PID: %3d goes into  Queue 1\n", simPid);
                enqueue(queue1, simPid);
            }
            increment_sim_time(simClock, blockedInc);  // and blocked queue overhead to the clock
        }
        /*Check Round Robin*/
        else if (rrQueue->items > 0) 
        {
            increment_sim_time(simClock, schedInc);  // increment for scheduling overhead
            simPid = dequeue(rrQueue);// get pid at head of the queue
            priority = table[simPid].priority;// get stored priority
            // dispatch the process
            response = dispatch(simPid, priority, msqid, (*simClock), quantum);
            // calculate burst time
            burst = response * (quantum / 100) * pow(2.0, (double)priority);

            if (response == 100) 
            {// Used full time slice
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS:RRQueue Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:RRQueue PID: %3d goes into  Round Robin\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(rrQueue, simPid);
            }
            else if (response < 0) 
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS:RRQueue Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:RRQueue PID: %3d goes into Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                blockedPids[simPid] = TRUE;//add to blocked "queue"
            }
            else // Terminated
            {
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);// wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = FALSE;  // set simpid to false
                terminated += 1; // increment terminated process counter
                fprintf(logFile, "OSS:RRQueue Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }
        }
        /*MLFQ
         * Queue 1*/
        else if (queue1->items > 0) 
        {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue1);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) 
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS:Queue1: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue1: PID: %3d goes into  Queue 2\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 2;
                enqueue(queue2, simPid);
            }
            else if (response < 0) 
            {// BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS:Queue1: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue1: PID: %3d goes into Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = TRUE;//add to blocked "queue"
            }
            else 
            {// Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);// wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update the total time for summary of simulation
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = FALSE;  // set simpid to false
                terminated += 1;// increment terminated processes counter
                fprintf(logFile, "OSS:Queue1: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  
        } 
        /* Queue 2 */
        else if (queue2->items > 0) 
        {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue2);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) 
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS:Queue2: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue2: PID: %3d goes into  Queue 3\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 3;
                enqueue(queue3, simPid);
            }
            else if (response < 0) 
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS:Queue2: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue2: PID: %3d goes into Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = TRUE;//add to blocked "queue" 
            }
            else {// Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);// wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = FALSE;  // set simpid to flase
                terminated += 1; // increment terminated processes counter
                fprintf(logFile, "OSS:Queue2: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  // end response ifs

            if (exQueue2->items > 0)
            {
                increment_sim_time(simClock, schedInc);
                simPid = dequeue(exQueue2);
                priority = table[simPid].priority;
                response = dispatch(simPid, priority, msqid, (*simClock), quantum);
                burst = response * (quantum / 100) * pow(2.0, (double)priority);
                if (response == 100)
                {
                    increment_sim_time(simClock, burst);
                    fprintf(logFile, "OSS:EXQueue2: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                    fprintf(logFile, "OSS:EXQueue2: PID: %3d goes into expired queue 3\n", simPid);
                    // updat pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 3;
                    enqueue(exQueue3, simPid);
                }
                else if (response < 0)
                {  // BLocked
                    burst = burst * -1;
                    increment_sim_time(simClock, burst);  // increment the clock
                    fprintf(logFile, "OSS:EXQueue2: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                    fprintf(logFile, "OSS:EXQueue2: PID: %3d goes into Blocked queue\n", simPid);
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 1;  // set priority back to highest level
                    blockedPids[simPid] = 1;//add to blocked "queue"
                }
                else
                {// Terminated
                    increment_sim_time(simClock, burst);  // increment the clock
                    pid = wait(NULL); // wait for child to finish
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    // update totals
                    totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                    totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                    totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                    availablePids[simPid] = FALSE;  // set simpid to available
                    terminated += 1;// increment terminated processes counter
                    fprintf(logFile, "OSS:EXQueue2: Terminated PID: %3d Used: %9dns\n", simPid, burst);
                }  // end response ifs
            }
        }    // end queue 2 check
        /* Queue 3 */
        else if (queue3->items > 0) 
{
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue3);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) 
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS:Queue3: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue3: PID: %3d goes into  Queue 3\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(queue3, simPid);
            }
            else if (response < 0) 
            {  // Blocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS:Queue3: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS:Queue3: PID: %3d goes into Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = TRUE;//add to blocked "queue"
            }
            else 
            {// Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL); // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = FALSE;  // set simpid to false
                terminated += 1;// increment terminated processes counter
                fprintf(logFile, "OSS:Queue3: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  // end response ifs
            if (exQueue3->items > 0)
            {
                increment_sim_time(simClock, schedInc);
                simPid = dequeue(exQueue3);
                priority = table[simPid].priority;
                response = dispatch(simPid, priority, msqid, (*simClock), quantum);
                burst = response * (quantum / 100) * pow(2.0, (double)priority);
                if (response == 100)
                {
                    increment_sim_time(simClock, burst);
                    fprintf(logFile, "OSS:EXQueue3: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                    fprintf(logFile, "OSS:EXQueue3: PID: %3d goes into  expired queue 3\n", simPid);
                    // updat pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 3;
                    enqueue(exQueue3, simPid);
                }
                else if (response < 0)
                {  // BLocked
                    burst = burst * -1;
                    increment_sim_time(simClock, burst);  // increment the clock
                    fprintf(logFile, "OSS:EXQueue3: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                    fprintf(logFile, "OSS:EXQueue3: PID: %3d goes into Blocked queue\n", simPid);
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 1;  // set priority back to highest level
                    blockedPids[simPid] = 1;//add to blocked "queue"
                }
                else
                {// Terminated
                    increment_sim_time(simClock, burst);// increment the clock
                    pid = wait(NULL);// wait for child to finish
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    // update totals
                    totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                    totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                    totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                    availablePids[simPid] = FALSE;  // set simpid to flase
                    terminated += 1;// increment terminated processes counter
                    fprintf(logFile, "OSS:EXQueue3: Terminated PID: %3d Used: %9dns\n", simPid, burst);
                }  // end response ifs
            }
        }  
        /* IDLE: empty queues and nothing ready to spawn */
        else 
        {
            increment_sim_time(&totalIdle, idleInc);
            increment_sim_time(simClock, idleInc);
        }
    }
    // summary of simulation
    avgCPU = (totalCPU.s + (0.000000001 * totalCPU.ns)) / ((double)generated);
    avgSYS = (totalSYS.s + (0.000000001 * totalSYS.ns)) / ((double)generated);
    avgWait = (totalWait.s + (0.000000001 * totalWait.ns)) / ((double)generated);
    fprintf(logFile, "\nTotal Processes: %d\n", generated);
    fprintf(logFile, "Avg. Turnaround: %.2fs\n", avgSYS);
    fprintf(logFile, "Avg. CPU Time:   %.2fs\n", avgCPU);
    fprintf(logFile, "Avg. Wait Time:  %.2fs\n", avgWait);
    fprintf(logFile, "Avg. Sleep Time: %.2fs\n", (avgSYS - avgCPU));
    fprintf(logFile, "Total Idle Time: %d.%ds\n", totalIdle.s, totalIdle.ns / 10000000);
    fprintf(logFile, "Total Run Time:  %d.%ds\n", simClock->s, simClock->ns / 10000000);
    printf("Total Processes: %d\n", generated);
    printf("Avg. Turnaround: %.2fs\n", avgSYS);
    printf("Avg. CPU Time:   %.2fs\n", avgCPU);
    printf("Avg. Wait Time:  %.2fs\n", avgWait);
    printf("Avg. Sleep Time: %.2fs\n", (avgSYS - avgCPU));
    printf("Total Idle Time: %d.%ds\n", totalIdle.s, totalIdle.ns / 10000000);
    printf("Total Run Time:  %d.%ds\n", simClock->s, simClock->ns / 10000000);
    return;
}