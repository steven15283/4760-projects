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
	int maxProcesses = 20;// max amount of processes allowed in the system at one time

	srand(time(0));// seed rand
	printf("----------Starting Simulation----------\n");

    /*Scheduling structures*/
    processControlBlock* table;// Process control block table
    simtime_t* simClock;// simulated system clock
    queue_t* queue0; // round robin queue-highest level
    queue_t* queue1; // high level
    queue_t* queue2; // mid level
    queue_t* queue3;  // lowest level
    queue_t* exQueue0; // expired round robin queue-highest level
    queue_t* exQueue1; // expired high level
    queue_t* exQueue2; // expired mid level
    queue_t* exQueue3;  // expired lowest level
    int blockedPids[maxProcesses]; //blocked "queue"
    int availablePids[maxProcesses]; // pseudo bit vector of available pids

    int schedInc = 1000;// scheduling overhead
    int idleInc = 100000;// idle increment
    int blockedInc = 1000000; // blocked queue check increment
    int quantum = 10000000; // base time quantum
    int burst; // actual time used by the process
    int response; // response from process. will be percentage of quantum used

    simtime_t maxTimeBetweenNewProcesses = { .s = 0, .ns = 500000000 };
    simtime_t nextProcess; // holds the time we should spawn the next process
    int terminated = 0; // counter of terminated processes
    int generated = 0; // counter of generated processes
    int lines = 0;  // lines written to the file
    int maxTotalProcesses = 20; // how many processes to generate total
    int simPid; // holds a simulated pid
    int priority;  // holds priority of a process
    char simPidArg[3]; // exec arg 2. simulated pid as a string
    char msqidArg[10];// exec arg 3. msqid as string
    char quantumArg[10];
    sprintf(quantumArg, "%d", quantum);

    int i = 0;  // loop iterator
    int pid;    // holds wait() and fork() return values

    //initialize summary for log
    simtime_t totalCPU = { .s = 0, .ns = 0 };
    simtime_t totalSYS = { .s = 0, .ns = 0 };
    simtime_t totalIdle = { .s = 0, .ns = 0 };
    simtime_t totalWait = { .s = 0, .ns = 0 };
    double avgCPU = 0.0;
    double avgSYS = 0.0;
    double avgWait = 0.0;

    /*Setup*/
    table = create_table(maxProcesses);// setup shared pcb table
    simClock = create_sim_clock(maxProcesses);// setup shared simulated clock
    queue0 = create_queue(maxProcesses);// setup highest level/priority
    queue1 = create_queue(maxProcesses);// setup high level/priority
    queue2 = create_queue(maxProcesses);// mid level/priority
    queue3 = create_queue(maxProcesses);// lowest level/priority
    exQueue0 = create_queue(maxProcesses);// setup expired highest level/priority
    exQueue1 = create_queue(maxProcesses);// setup expired high level/priority
    exQueue2 = create_queue(maxProcesses);// setup expired mid level/priority
    exQueue3 = create_queue(maxProcesses);// setup expired lowest level/priorty
    create_msqueue();                           // set up message queue.sets global msqid variable
    sprintf(msqidArg, "%d", msqid);             // write msqid to msqid string arg
    // Loop through available pids to set all to 1(true)
    for (i = 0; i < maxProcesses; i++)
    {
        blockedPids[i] = 0;//set blocked "queue" to empty
        availablePids[i] = 1;
    }
    // get time to spawn next process
    nextProcess = get_next_process_time(maxTimeBetweenNewProcesses, (*simClock));

    while (terminated < maxTotalProcesses)
    {
        // if we have written 9000 lines stop generating
        // criteria is to stop at 10000 however lines will continue to be written
        // because there are still processes in the system
        if (lines >= 9000)
            maxTotalProcesses = generated;

        /* Uncomment to easily show that all queues are occupied at some point */
        // if (queue3->items > 0 && queue1->items > 0 && queue2->items > 0)
        //   printf("3 queues occupied\n");
        // if (queue3->items > 0 && queue1->items > 0)
        //   printf("3 & 1 occupied\n");
        // if (queue3->items > 0 && queue2->items > 0)
        //   printf("3 & 2 occupied\n");
        // if (queue2->items > 0 && queue1->items > 0)
        //   printf("2 & 1 occupied\n");


        // check for an available pid
        simPid = get_sim_pid(availablePids, maxProcesses);

        
        // printf("%d <= %d && %d <= %d\n", nextProcess.s, simClock->s,nextProcess.ns, simClock->ns);

        if (should_spawn(simPid, nextProcess, (*simClock), generated, maxTotalProcesses))
        {
            // printf("spawn\n");
            // pid was returned so spawn a new process
            sprintf(simPidArg, "%d", simPid);  // write simpid to simpid string arg
            availablePids[simPid] = 0;     // set pid to unavailable
            // get random priority(0:real time or 1:user)
            priority = rand_priority(5);
            fprintf(logFile, "%-5d: OSS: Generating process PID %d in queue %d at %ds%09dns\n",lines++, simPid, priority, simClock->s, simClock->ns);
            // create pcb for new process at available pid
            table[simPid] = create_pcb(priority, simPid, (*simClock));
            // queue in highest level if real-time(priority == 0)
            if (priority == 0)
                enqueue(queue0, simPid);
            else  // queue in MLFQ otherwise
                enqueue(queue1, simPid);
            // fork a new process
            pid = fork();
            if (pid < 0)
            {  // error
                perror("./oss: Error: fork ");
                cleanup();
            }
            else if (pid == 0) {  // child
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
            blockedPids[simPid] = 0;//remove from blocked "queue"
            fprintf(logFile, "%-5d: OSS: Unblocked. PID: %3d TIME: %ds%09dns\n", lines++, simPid, simClock->s, simClock->ns);
            if (table[simPid].priority == 0)
            {
                fprintf(logFile, "%-5d: OSS: PID: %3d -> round robin\n", lines++, simPid);
                enqueue(queue0, simPid);
            }
            else {
                fprintf(logFile, "%-5d: OSS: PID: %3d -> queue 1\n", lines++, simPid);
                enqueue(queue1, simPid);
            }
            increment_sim_time(simClock, blockedInc);  // and blocked queue overhead to the clock
        }

        /*Check Round Robin/highest queue*/
        else if (queue0->items > 0)
        {
            increment_sim_time(simClock, schedInc);  // increment for scheduling overhead
            simPid = dequeue(queue0);               // get pid at head of the queue
            priority = table[simPid].priority;       // get stored priority
            // dispatch the process
            response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
            // calculate burst time
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100)
            {                  // Used full time slice
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> expired high priority queue\n", lines++, simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(exqueue1, simPid);
            }
            else if (response < 0)
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> blocked queue\n", lines++, simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else
            {// Terminated
                increment_sim_time(simClock, burst); // increment the clock
                pid = wait(NULL); // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated process counter
                fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
            }
        }
        /*MLFQ
         * Queue 1*/
        else if (queue1->items > 0)
        {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue1);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100)
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> expired queue 2\n", lines++, simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 2;
                enqueue(exqueue2, simPid);
            }
            else if (response < 0)
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> blocked queue\n", lines++, simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else
            {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
            }  // end response ifs

            
        }    // end queue 1 check
        /* Queue 2 */
        else if (queue2->items > 0)
        {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue2);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100)
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> Expired Queue 3\n", lines++, simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 3;
                enqueue(exqueue3, simPid);
            }
            else if (response < 0)
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> Blocked queue\n", lines++, simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue" 
            }
            else
            {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
            }  // end response ifs

            if (exQueue2->items > 0)
            {
                increment_sim_time(simClock, schedInc);
                simPid = dequeue(exQueue1);
                priority = table[simPid].priority;
                response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
                burst = response * (quantum / 100) * pow(2.0, (double)priority);
                if (response == 100)
                {
                    increment_sim_time(simClock, burst);
                    fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                    fprintf(logFile, "%-5d: OSS: PID: %3d -> expired queue 3\n", lines++, simPid);
                    // updat pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 3;
                    enqueue(exqueue3, simPid);
                }
                else if (response < 0)
                {  // BLocked
                    burst = burst * -1;
                    increment_sim_time(simClock, burst);  // increment the clock
                    fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                    fprintf(logFile, "%-5d: OSS: PID: %3d -> blocked queue\n", lines++, simPid);
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 1;  // set priority back to highest level
                    blockedPids[simPid] = 1;//add to blocked "queue"
                }
                else
                {                                // Terminated
                    increment_sim_time(simClock, burst);  // increment the clock
                    pid = wait(NULL);                     // wait for child to finish
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    // update totals
                    totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                    totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                    totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                    availablePids[simPid] = 1;  // set simpid to available
                    terminated += 1;               // increment terminated processes counter
                    fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
                }  // end response ifs
            }
        }    // end queue 2 check
        /* Queue 3 */
        else if (queue3->items > 0)
        {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue3);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100)
            {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> Expired Queue 3\n", lines++, simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(exqueue3, simPid);
            }
            else if (response < 0)
            {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                fprintf(logFile, "%-5d: OSS: PID: %3d -> Blocked queue\n", lines++, simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else
            {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
            }  // end response ifs

            if (exQueue3->items > 0)
            {
                increment_sim_time(simClock, schedInc);
                simPid = dequeue(exQueue1);
                priority = table[simPid].priority;
                response = dispatch(simPid, priority, msqid, (*simClock), quantum, &lines);
                burst = response * (quantum / 100) * pow(2.0, (double)priority);
                if (response == 100)
                {
                    increment_sim_time(simClock, burst);
                    fprintf(logFile, "%-5d: OSS: Full Slice PID: %3d Used: %9dns\n", lines++, simPid, burst);
                    fprintf(logFile, "%-5d: OSS: PID: %3d -> expired queue 3\n", lines++, simPid);
                    // updat pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 3;
                    enqueue(exqueue3, simPid);
                }
                else if (response < 0)
                {  // BLocked
                    burst = burst * -1;
                    increment_sim_time(simClock, burst);  // increment the clock
                    fprintf(logFile, "%-5d: OSS: Blocked    PID: %3d Used: %9dns\n", lines++, simPid, burst);
                    fprintf(logFile, "%-5d: OSS: PID: %3d -> blocked queue\n", lines++, simPid);
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    table[simPid].priority = 1;  // set priority back to highest level
                    blockedPids[simPid] = 1;//add to blocked "queue"
                }
                else
                {                                // Terminated
                    increment_sim_time(simClock, burst);  // increment the clock
                    pid = wait(NULL);                     // wait for child to finish
                    // update pcb
                    increment_sim_time(&table[simPid].cpuTime, burst);
                    table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                    // update totals
                    totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                    totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                    totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                    availablePids[simPid] = 1;  // set simpid to available
                    terminated += 1;               // increment terminated processes counter
                    fprintf(logFile, "%-5d: OSS: Terminated PID: %3d Used: %9dns\n", lines++, simPid, burst);
                }  // end response ifs
            }
        }    // end queue 3 check
        /* IDLE: empty queues and nothing ready to spawn */
        else {
            increment_sim_time(&totalIdle, idleInc);
            increment_sim_time(simClock, idleInc);
        }
    }
    // easier to divide decimals than simtime_t
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
	printf("----------Simulation Complete----------\n");
	msgctl(msqid, IPC_RMID, NULL);  //delete msgqueue
	remove_shm();
	return 0;

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

/*Return a pid if there is one available, returns -1 otherwise*/
int get_sim_pid(int* pids, int pidsCount)
{
    int i;
    for (i = 0; i < pidsCount; i++)
    {
        if (pids[i])
        {  // sim pid i is available
            return i;     // return available pid
        }
    }
    return -1;  // no available pids
}

/*return a random simtime in range [0, max] + current time*/
simtime_t get_next_process_time(simtime_t max, simtime_t currentTime)
{
    simtime_t nextTime = { .ns = (rand() % (max.ns + 1)) + currentTime.ns, .s = (rand() % (max.s + 1)) + currentTime.s };
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
 * eg with just s >= s and ns >= ns, 2s0ns >= 1s9ns is 0
 * with also checking s > s 2s0ns >= 1s9ns is 1*/
int should_spawn(int pid, simtime_t next, simtime_t now, int generated, int max)
{
    if (generated >= max)  // generated enough/too many processes
        return 0;
    if (pid < 0)  // no available pids
        return 0;
    // not time for next process
    if (next.s > now.s)
        return 0;
    // more specific not time for next process
    if (next.s >= now.s && next.ns > now.ns)
        return 0;

    return 1;
}

int check_blocked(int* blocked, processControlBlock* table, int count)
{
    int i; //loop iterator
    for (i = 0; i < count; i++)
    {
        if (blocked[i] == 1)
        {
            if (table[i].isReady == 1)
            {
                return i;
            }
        }
    }
    return -1;
}

/*Dispatches a process by sending a message with the appropriate time quantum to
 * the given pid
 * waits to recieve a response and returns that response */
int dispatch(int pid, int priority, int msqid, simtime_t currentTime, int quantum, int* lines)
{
    mymsg_t msg;// message to be sent
    quantum = quantum * pow(2.0, (double)priority);// i = queue #, slice = 2^i * quantum
    msg.mtype = pid + 1;// pids recieve messages of type (pid) + 1
    msg.mvalue = quantum;
    fprintf(logFile, "%-5d: OSS: Dispatch   PID: %3d Queue: %d TIME: %ds%09dns\n", *lines, pid, priority, currentTime.s, currentTime.ns);
    *lines += 1;
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

