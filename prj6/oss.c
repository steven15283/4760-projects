//steven guo 
//11/24/20
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oss.h"

FILE* logFile;

int simClockID;
const int SIM_CLOCK_KEY = 123456;
int msqid;
const int MSG_Q_KEY = 234567;

void oss(simtime_t* simClock, int maxActive);
void init_mem_management(int* pids, frame_t* frameTable, pagetable_t* pageTables, int maxActive);
void handle_args(int argc, char* argv[], int* maxActive);
static void time_out();
void cleanup();
FILE* open_file(char* fname, char* opts, char* error);
simtime_t* get_shared_simtime(int key);
void create_msqueue();
int get_simPid(int* pids, int size);
int spawn(char arg1[], char arg2[], int* active);
simtime_t get_next_process_time(simtime_t max, simtime_t currentTime);
void send_msg(int dest, enum actions action);
int get_empty_frame_pos(frame_t* table, int frames);
int get_lru_frame_pos(frame_t* table, int frames);
void shift_refBytes(frame_t* table, int frames);
void print_frame_table(frame_t* table, int frames, simtime_t now, FILE* out);
void print_stats(simtime_t* simClock, int faults, int references, FILE* out);


void oss(simtime_t* simClock, int maxActive)
{
    msg_t msg;//holds the current message being sent or last recieved
    int lines = 0;//counter of lines written to the log
    int activeProcesses = 0;//counter of active processes 
    simtime_t nextSpawn = { .s = 0, .ns = 0 };//simulated time for next spawn
    int spawnInc = 1000000; // 1ms = 1000000 ns
    int noPageFaultInc = 10;
    int pageFaultInc = 1000;
    int readWriteInc = 15;
    int referenceCounter = 0;//counter of total frame references
    int pageFaultCounter = 0;//counter of total page faults
    //array of taken sim pids. index = simpid, value at index = actual pid
    int* pids = (int*)malloc(sizeof(int) * maxActive);
    frame_t* frameTable = (frame_t*)malloc(sizeof(frame_t) * TOTAL_MEMORY);// frame Table. array of frames
    //array of page tables. one page table for each process
    pagetable_t* pageTables = (pagetable_t*)malloc(sizeof(pagetable_t) * maxActive);
    // initialize the array of pids and memory management structures
    init_mem_management(pids, frameTable, pageTables, maxActive);
    // Set up the message queue
    create_msqueue();


    /oss
    fprintf(logFile, "Begin OS Simulation\n");
    lines++;
    while (1)
    {
        //if oss should spawn a new child process
        if (activeProcesses < maxActive && less_or_equal_sim_times(nextSpawn, *simClock) == 1)
        {
            //spawn
            int simPid;//simulated pid
            int pid;//real child pid
            char msqidArg[10];//exec arg
            char simPidArg[3];//exec arg
            simPid = get_simPid(pids, maxActive);
            if (simPid < 0)
            {  //error
                fprintf(logFile, "%d.%09ds ./oss: Error: No available simPids\n", simClock->s, simClock->ns);
                fprintf(stderr, "./oss: Error: No available simPids\n");
                cleanup();
            }
            //write exec args
            sprintf(msqidArg, "%d", msqid);
            sprintf(simPidArg, "%d", simPid);
            //spawn a process and increment active processes counter
            pid = spawn(msqidArg, simPidArg, &activeProcesses);
            //store pid based on simPid
            pids[simPid] = pid;
            //schedule next process spawn time
            //note it is suggested to spawn every 1 - 500ms but this would not spawn enough children fast enough
            //this is reduced to spawn children more frequently
            nextSpawn = get_next_process_time((simtime_t) { 0, 500000 }, (*simClock));
            // logging
            if (lines++ < 100000)
                fprintf(logFile, "%d.%09ds ./oss: Spawned new process p%d\n", simClock->s, simClock->ns, simPid);
            // Increment the simulated clock
            increment_sim_time(simClock, spawnInc);
        }
        //memory management
        // Check the message queue for memory requests or termination
        else if ((msgrcv(msqid, &msg, sizeof(msg_t), ossChannel, IPC_NOWAIT)) > 0)
        {
            increment_sim_time(simClock, readWriteInc);
            referenceCounter++;
            // check page table for frame
            int framePos = pageTables[msg.pid].pages[msg.address / 1024].framePos;
            int action = msg.action;//read(1) or write(2) or terminate(3)
            if (action == TERMINATE) 
            {
                int i;
                // get real pid of terminating process and set to available
                int pid = pids[msg.pid];
                pids[msg.pid] = -1;
                //clear page table
                for (i = 0; i < PROCESS_SIZE; i++)
                    pageTables[msg.pid].pages[i].framePos = -1;
                //clear frames used by this process
                for (i = 0; i < TOTAL_MEMORY; i++)
                {
                    if (frameTable[i].pid == msg.pid)
                    {
                        frameTable[i].ref = 0x0;
                        frameTable[i].dirty = 0x0;
                        frameTable[i].pid = -1;//indicates frame is empty
                    }
                }
                activeProcesses--;
                waitpid(pid, NULL, 0);
                // logging
                if (lines++ < 100000)
                {
                    fprintf(logFile, "%d.%09ds ./oss: p%d terminated\n", simClock->s, simClock->ns, msg.pid);
                }
            }
            else
            {
                if (action == WRITE)
                {
                    // logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: p%d requests WRITE at Address:%d\n", simClock->s, simClock->ns, msg.pid, msg.address);
                    }
                }
                else
                {
                    // logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: p%d requests READ at Address:%d\n", simClock->s, simClock->ns, msg.pid, msg.address);
                    }
                }
                //no page fault
                if (framePos != -1 && frameTable[framePos].pid == msg.pid)
                {
                    // logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: GRANTED p%d Address:%d in Frame:%d\n", simClock->s, simClock->ns, msg.pid, msg.address, framePos);
                    }
                    
                    //set most sig bit with bitwise or ex 00101011 | 10000000 = 10101011 (0x80 = 128 = 10000000)
                    frameTable[framePos].ref = frameTable[framePos].ref | 0x80;
                    //if it is a write then set dirty bit
                    if (action == WRITE)
                    {
                        frameTable[framePos].dirty = 0x1;
                        // logging
                        if (lines++ < 100000)
                        {
                            fprintf(logFile, "%d.%09ds ./oss: Setting dirty bit. Frame: %d\n", simClock->s, simClock->ns, framePos);
                        }
                    }
                    // increment the simulated clock
                    increment_sim_time(simClock, noPageFaultInc);
                }
                else
                {
                    pageFaultCounter++;
                    int framePos = get_empty_frame_pos(frameTable, TOTAL_MEMORY);
                    //page replacement
                    if (framePos == -1)
                    {
                        // get the position of the frame to replace
                        framePos = get_lru_frame_pos(frameTable, TOTAL_MEMORY);
                        // swap in the frame
                        frameTable[framePos].pid = msg.pid;
                        frameTable[framePos].ref = 0x80;
                        if (action == WRITE)
                            frameTable[framePos].dirty = 0x1;
                        else
                            frameTable[framePos].dirty = 0x0;
                        //update page table
                        pageTables[msg.pid].pages[msg.address / 1024].framePos = framePos;
                    }
                    else
                    {
                        //logging
                        if (lines++ < 100000)
                        {
                            fprintf(logFile, "%d.%09ds ./oss: Inserting Page into Frame: %d\n", simClock->s, simClock->ns, framePos);
                        }
                        //set the frame
                        frameTable[framePos].pid = msg.pid;
                        frameTable[framePos].ref = 0x80;
                        if (action == WRITE)
                            frameTable[framePos].dirty = 0x1;
                        else
                            frameTable[framePos].dirty = 0x0;
                        //update page table
                        pageTables[msg.pid].pages[msg.address / 1024].framePos = framePos;
                    }
                    //send_msg(msg.pid, GRANTED);
                    increment_sim_time(simClock, pageFaultInc);
                }
            }
        }
        // shift ref bytes, print memory map, and print statistics
        if ((referenceCounter + 1) % 100 == 0)
        {
            shift_refBytes(frameTable, TOTAL_MEMORY);
            if (lines < 100000)
            {
                print_frame_table(frameTable, TOTAL_MEMORY, *simClock, logFile);
                print_stats(simClock, pageFaultCounter, referenceCounter, logFile);
                lines += 262;
            }
        }
        increment_sim_time(simClock, 10);


    }
    //close log
    fclose(logFile);
    //delete sim clock
    shmctl(simClockID, IPC_RMID, NULL);
    //delete message queue
    msgctl(msqid, IPC_RMID, NULL);
    return;
}






//initialize the array of pids and memory management structures
void init_mem_management(int* pids, frame_t* frameTable, pagetable_t* pageTables, int maxActive)
{
    int i, j;
    //initialize all pids to available ( = -1)
    for (i = 0; i < maxActive; i++)
    {
        pids[i] = -1;
    }
    //initialize frame table
    for (i = 0; i < TOTAL_MEMORY; i++)
    {
        frameTable[i].ref = 0x0;
        frameTable[i].dirty = 0x0;
        frameTable[i].pid = -1;//indicates frame is empty
    }
    //initialize page tables
    for (i = 0; i < maxActive; i++)
    {
        for (j = 0; j < PROCESS_SIZE; j++)
        {
            pageTables[i].pages[j].framePos = -1;
        }
    }
}

//handle command line arguments using getopt
void handle_args(int argc, char* argv[], int* maxActive)
{
    int opt;
    char* logName = "log.txt";
    if (argc < 2)
    {
        printf("No arguments given\n");
        printf("Using default values\n");
    }
    while ((opt = getopt(argc, argv, "hn:o:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("This program takes the following possible arguments\n");
            printf("\n");
            printf("  -h           : to display this help message\n");
            printf("  -n x         : x = maximum concurrent child processes\n");
            printf("  -o filename  : to specify log file\n");
            printf("\n");
            printf("Defaults:\n");
            printf("  Log File: log.txt\n");
            printf("         n: 18\n");
            printf("  Non-verbose printing\n");
            exit(EXIT_SUCCESS);
        case 'n':
            *maxActive = atoi(optarg);
            break;
        case 'o':
            logName = optarg;
            break;
        default:
            printf("No arguments given\n");
            printf("Using default values\n");
            break;
        }
    }
    if (*maxActive > 18)
    {
        printf("n must be <= 18\n");
        exit(EXIT_SUCCESS);
    }
    else if (*maxActive <= 0)
    {
        printf("n must be >= 1\n");
        exit(EXIT_SUCCESS);
    }
    printf("Log File: %s\n", logName);
    printf("       n: %d\n", *maxActive);
    //open log file
    logFile = open_file(logName, "w", "./oss: Error: ");
    return;
}

//SIGALRM handler
static void time_out()
{
    fprintf(stderr, "Timeout\n");
    cleanup();
    exit(EXIT_SUCCESS);
}

//clean up for abnormal termination
void cleanup()
{
    fclose(logFile);
    shmctl(simClockID, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);
    kill(0, SIGTERM);
    exit(EXIT_SUCCESS);
}

//fopen with simple error check
FILE* open_file(char* fname, char* opts, char* error)
{
    FILE* fp = fopen(fname, opts);
    if (fp == NULL) {  // error opening file
        perror(error);
        cleanup();
    }
    return fp;
}

//return a pointer to simtime_t object in shared memory
simtime_t* get_shared_simtime(int key)
{
    simtime_t* simClock;
    simClockID = shmget(key, sizeof(simtime_t), IPC_CREAT | 0777);
    if (simClockID < 0)
    {
        perror("./oss: Error: shmget for simulated clock");
        cleanup();
    }
    simClock = shmat(simClockID, NULL, 0);
    if (simClock < 0)
    {
        perror("./oss: Error: shmat for simulated clock");
        cleanup();
    }
    return simClock;
}

//set the msqid
void create_msqueue()
{
    msqid = msgget(MSG_Q_KEY, 0666 | IPC_CREAT);
    if (msqid < 0) {
        perror("./oss: Error: msgget ");
        cleanup();
    }
    return;
}

//return index of free spot in pids array, -1 if no spots
int get_simPid(int* pids, int size)
{
    int i;
    for (i = 0; i < size; i++)
        if (pids[i] == -1)
        {
            return i;
        }
    return -1;
}

//spawn a process with the args and increment running process counter
//essentially just fork() and execl()
int spawn(char arg1[], char arg2[], int* active)
{
    int pid = fork();
    if (pid < 0)
    {  // error
        perror("./oss: Error: fork ");
        cleanup();
    }
    else if (pid == 0)
    {  // child
        execl("./user", "user", arg1, arg2, (char*)NULL);
    }
    (*active)++;
    return pid;
}

//get a random time that the next process should spawn
//returns a simClock + random time between 0 and given max simtime
simtime_t get_next_process_time(simtime_t max, simtime_t currentTime)
{
    simtime_t nextTime = { .ns = (rand() % (max.ns + 1)) + currentTime.ns,.s = (rand() % (max.s + 1)) + currentTime.s };
    if (nextTime.ns >= 1000000000)
    {
        nextTime.s += 1;
        nextTime.ns -= 1000000000;
    }
    return nextTime;
}

//only two important parts of an oss to child message...
//the destination (pid of receiving process)
//and the action sent to them (denied or granted)
void send_msg(int dest, enum actions action)
{
    msg_t msg = { .mtype = dest,
                  .pid = -1,//child doesn't need to know
                  .address = -1,//child already know what it asked for
                  .action = action
    };
    if (msgsnd(msqid, &msg, sizeof(msg_t), 0) == -1)
    {
        perror("./oss: Error: msgsnd ");
        cleanup();
    }
    return;
}

// returns the position of the first empty frame in the table. -1 if their are no empty frames
int get_empty_frame_pos(frame_t* table, int frames)
{
    int i;
    for (i = 0; i < frames; i++)
    {
        if (table[i].pid == -1)
        {
            return i;
        }
    }
    return -1;
}
//return the index of the least recently used frame
//- linear search through the frame table and storing the index of the frame
//with the lowest reference that we have seen
int get_lru_frame_pos(frame_t* table, int frames)
{
    int i;
    int lru_frame = 0; // Least recently used frame index
    for (i = 0; i < frames; i++)
    {
        // If frame is occupied and has a lower reference byte the the previous lowest
        // then set lowest frame index to that frame
        if (table[i].pid > 0 && table[i].ref < table[lru_frame].ref)
            lru_frame = i;
    }
    return lru_frame;
}

//bit shift the reference bytes of all the occupied frames down by 1
void shift_refBytes(frame_t* table, int frames)
{
    int i;
    for (i = 0; i < frames; i++)
    {
        table[i].ref = table[i].ref >> 1;//shift bits down by 1 bit
    }
}

//print the formatted frame table
void print_frame_table(frame_t* table, int frames, simtime_t now, FILE* out)
{
    int i;
    fprintf(out, "Current memory layout at time %d:%9d is:\n", now.s, now.ns);
    fprintf(out, "           Occupied  RefByte  DirtyBit\n");
    for (i = 0; i < frames; i++)
    {
        if (table[i].pid >= 0) // > 0 indicates occupied frame
            fprintf(out, "Frame %3d: %-9s %-8d %-8d\n", i, "Yes", table[i].ref, table[i].dirty);
        else
            fprintf(out, "Frame %3d: %-9s %-8d %-8d\n", i, "No", table[i].ref, table[i].dirty);
    }
    return;
}

void print_stats(simtime_t* simClock, int faults, int references, FILE* out)
{
    double totalTimeInSeconds = (double)(simClock->s) + ((double)(simClock->ns) / 1000000000);
    double refsPerSecond = (double)references / totalTimeInSeconds;
    double faultsPerReference = (double)faults / (double)references;
    fprintf(out, "References per second: %lf\n", refsPerSecond);
    fprintf(out, "Faults per Reference: %lf\n", faultsPerReference);
    return;
}

int main(int argc, char* argv[])
{
    int maxActive = 18;
    simtime_t* simClock;
    //seed rand
    srand(time(0));
    //two second time out
    signal(SIGALRM, time_out);
    alarm(2);
    //handle command line arguments
    handle_args(argc, argv, &maxActive);
    //set up shared simulated clock
    simClock = get_shared_simtime(SIM_CLOCK_KEY);
    simClock->s = 0;
    simClock->ns = 0;

    oss(simClock, maxActive);

    return 0;
}