#include "shared.h"

pcb_t* attach_pcb_table();
simtime_t* attach_sim_clock();
void get_clock_and_table(int n);
int get_outcome();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./user pid msqid quantum\n");
        exit(EXIT_SUCCESS);
    }
    int pid;
    mymsg_t msg;
    int quantum;
    int outcome = 0;
    pid = atoi(argv[1]);
    msqid = atoi(argv[2]);
    quantum = atoi(argv[3]);
    srand(time(0) + (pid + 1));  // seeding rand. seeding w/ time(0) caused
                                 // processes spawned too close to have same seed
    pcb_t* table;
    simtime_t* simClock;
    simtime_t timeBlocked;//holds the time that the process was blocked at
    simtime_t event;  // time of the event that will unblock the process
    int burst;//burst to calculate unblock time
    get_clock_and_table(pid);
    table = attach_pcb_table();
    simClock = attach_sim_clock();