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

    /*****SETUP*****/
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
    //Array of taken sim pids. index = simpid, value at index = actual pid
    int* pids = (int*)malloc(sizeof(int) * maxActive);
    // Frame Table. array of frames
    frame_t* frameTable = (frame_t*)malloc(sizeof(frame_t) * TOTAL_MEMORY);
    // Array of page tables. one page table for each process
    pagetable_t* pageTables = (pagetable_t*)malloc(sizeof(pagetable_t) * maxActive);
    // Initialize the array of pids and memory management structures
    init_mem_management(pids, frameTable, pageTables, maxActive);
    // Set up the message queue
    create_msqueue();


    /*****OSS*****/
    fprintf(logFile, "Begin OS Simulation\n");
    lines++;
    while (1)
    {
        //If oss should spawn a new child process
        if (activeProcesses < maxActive && less_or_equal_sim_times(nextSpawn, *simClock) == 1)
        {
            //Spawn
            int simPid;//simulated pid
            int pid;//real child pid
            char msqidArg[10];//exec argument
            char simPidArg[3];//exec argument
            simPid = get_simPid(pids, maxActive);
            if (simPid < 0)
            {  //Error
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
            // Logging
            if (lines++ < 100000)
                fprintf(logFile, "%d.%09ds ./oss: Spawned new process p%d\n", simClock->s, simClock->ns, simPid);
            // Increment the simulated clock
            increment_sim_time(simClock, spawnInc);
        }
        /** MEMORY MANAGEMENT **/
        // Check the message queue for memory requests or termination
        else if ((msgrcv(msqid, &msg, sizeof(msg_t), ossChannel, IPC_NOWAIT)) > 0)
        {
            increment_sim_time(simClock, readWriteInc);
            referenceCounter++;
            // check page table for frame
            int framePos = pageTables[msg.pid].pages[msg.address / 1024].framePos;
            int action = msg.action;//read(1) or write(2) or terminate(3)
            if (action == TERMINATE) {
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
                // Logging
                if (lines++ < 100000)
                {
                    fprintf(logFile, "%d.%09ds ./oss: p%d terminated\n", simClock->s, simClock->ns, msg.pid);
                }
            }
            else
            {
                if (action == WRITE)
                {
                    // Logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: p%d requests WRITE at Address:%d\n", simClock->s, simClock->ns, msg.pid, msg.address);
                    }
                }
                else
                {
                    // Logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: p%d requests READ at Address:%d\n", simClock->s, simClock->ns, msg.pid, msg.address);
                    }
                }
                //printf("%d\t%d == %d\n", framePos, frameTable[framePos].pid, msg.pid);
                /** NO PAGE FAULT **/
                if (framePos != -1 && frameTable[framePos].pid == msg.pid)
                {
                    // Logging
                    if (lines++ < 100000)
                    {
                        fprintf(logFile, "%d.%09ds ./oss: GRANTED p%d Address:%d in Frame:%d\n", simClock->s, simClock->ns, msg.pid, msg.address, framePos);
                    }
                    //send_msg(msg.pid, GRANTED);
                    //set most sig bit with bitwise or
                    //ex 00101011 | 10000000 = 10101011 (0x80 = 128 = 10000000)
                    frameTable[framePos].ref = frameTable[framePos].ref | 0x80;
                    //if it s\is a write then set dirty bit
                    if (action == WRITE)
                    {
                        frameTable[framePos].dirty = 0x1;
                        // Logging
                        if (lines++ < 100000)
                        {
                            fprintf(logFile, "%d.%09ds ./oss: Setting dirty bit. Frame: %d\n", simClock->s, simClock->ns, framePos);
                        }
                    }
                    // Increment the simulated clock
                    increment_sim_time(simClock, noPageFaultInc);
                }
                /** END of NO PAGE FAULT **/
                /** PAGE FAULT **/
                else
                {
                    pageFaultCounter++;
                    int framePos = get_empty_frame_pos(frameTable, TOTAL_MEMORY);
                    /** PAGE REPLACEMENT **/
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
                    /** END of PAGE REPLACEMENT **/
                    /** PAGE INSERT **/
                    else
                    {
                        // Logging
                        if (lines++ < 100000)
                        {
                            fprintf(logFile, "%d.%09ds ./oss: Inserting Page into Frame: %d\n", simClock->s, simClock->ns, framePos);
                        }
                        // Set the frame
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
                    /** END of PAGE INSERT **/
                }
                /** END of PAGE FAULT **/
            }
        }

        /** END of MEMORY MANAGEMENT **/
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

    // Should never get here in final version but clean up just in case
    // // Free pids
    // free(pids);
    // // Free tables 
    // free(frameTable);
    // free(pageTables);
    // Close log
    fclose(logFile);
    // Delete sim clock
    shmctl(simClockID, IPC_RMID, NULL);
    // Delete message queue
    msgctl(msqid, IPC_RMID, NULL);
    return;
}