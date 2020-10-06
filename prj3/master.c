#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>

//steven guo
//10/4/2020

#define LENGTH 256 //max number of characters in a string
#define MAXPROCESS 20 //max number of processes
typedef struct {
	char data[MAXPROCESS][LENGTH]; //array of strings to check if its a palindrome
	int turn;// number for turn
	int flag[MAXPROCESS];// array for flag
	int numOfChild;//total number of children allowed
} shared_memory;

//global vars
int childProcessTotal = 4;//sets default number of child processes master will create
int maxChildInSystem = 2;//sets default number of child processes that can be in system at the same time
double maxTime = 100.00; //sets default time
int currChildCount = 0;//counts the amount of children 
int currChildSysCount = 0;//counts the amount of children in the system
enum state { idle, want_in, in_cs };//specifies the flags
int status = 0; //used for the wait function when creating a child
time_t startTime, currTime; //declared to be used with all functions

void initSharedMemory();
void spawnChild(int num); //create child
void createChildProcess(int num);//writes the log and makes sure there are only so many processes running at once
void timeoutSignal(int signal);//timeout interrupt for timer
void sigHandler(int signal); //crtl^c handler
char* printTime();//prints the current time

//needed to be declared to be used with all functions
shared_memory* shmptr;
pid_t* parent;
int parentId;
key_t parentKey;
key_t key;
int shmid;




char* printTime()//prints out current time
{
	time_t t;
	time(&t);
	return ctime(&t);
}

void initSharedMemory()
{
	key = ftok("makefile", 'a');//unique key from ftok
	shmid= shmget(key, sizeof(shared_memory), (S_IRUSR | S_IWUSR | IPC_CREAT));//obtain access to a shared memory segment.

	if (shmid < 0)//check if shmget is able to access shared memory segment
	{
		perror("shmget error: unable to access shared memory segment");
		exit(1);
	}

	shmptr = (shared_memory*)shmat(shmid, NULL, 0);// shmat to attach to shared memory

	//this is for later on when we have to terminate the parent due to time or when ^c is entered
	parentKey = ftok("makefile", 'b');
	parentId = shmget(parentKey, sizeof(pid_t), (IPC_CREAT | S_IRUSR | S_IWUSR));//allocate shared memory for parent id

	if (parentId < 0)
	{
		perror("shmget: Failed to allocate shared memory for parent id");
		exit(1);
	}
	else
	{
		parent = (pid_t*)shmat(parentId, NULL, 0);//attach parent into shared memory
	}
}

void createChildProcess(int num)//writes the log and makes sure there are only so many processes running at once
{
	

	if (currChildSysCount < maxChildInSystem)//checks if the current processes running are less than the maximum running processes specified by user
	{
		spawnChild(num);//create another child
	}
	else
	{//this means that the processes running at the moment is at max capacity and you have to wait for a child to be done to create another child
		waitpid(-(*parent), &status, 0);//waits for process state to change
		currChildSysCount--;//a child has changed state so another child can be created
		//printf("There are %d processes in the system\n", currChildSysCount);
		spawnChild(num);//create another child
	}

	
}

void spawnChild(int num)//create child, num is index of array for the process
{
	currChildSysCount++;//increment the number of current children in system
	if (fork() == 0)//create children
	{
		if (num == 0) //this means that the process is a parent
		{
			(*parent) = getpid();//set the pid to parent
		}
		setpgid(0, (*parent));//sets(links) process group
		//printf("before exec, num = %d\n", num);
		char buf[256];//temporary string
		sprintf(buf, "%d", num);//convert number into string to pass through exec as cmdline arg
		execlp("./palin", "palin", buf, NULL);//executes palin with num as an argument
		exit(0);
	}
}


void timeoutSignal(int signal)//timeout interrupt for timer
{
	time(&currTime);//get current time
	if (difftime(currTime, startTime) >= maxTime) //if time is up
	{
		printf("time has run out in master\n");
		killpg((*parent), SIGUSR1);//kills all the processes linked to parent
		int i;//initialize i for forloop
		for (i = 0; i < currChildSysCount; i++)//CHECK TO SEE IF PROCESSES HAVE EXITED with waitpid and sigkill
		{
			wait(NULL);
		}
		//release memory
		
		shmdt(shmptr);//detach shared memory
		shmctl(shmid, IPC_RMID, NULL);//remove from memory
		printf("exiting master process");
		exit(0);
	}

	
}

void sigHandler(int signal) //crtl^c handler
{
	killpg((*parent), SIGTERM);//kills all the processes linked to parent 
	int i;//initialize i for forloop
	for (i = 0; i < currChildSysCount; i++)//while there is still children running wait until they finish
	{
		wait(NULL);
	}

	//release memory
	
	shmdt(shmptr);//detach shared memory
	shmctl(shmid, IPC_RMID, NULL);//remove from memory
	printf("interrupt crtl^c caught:exiting master process - %s", printTime());
	exit(0);
}

int main(int argc, char* argv[])
{
	initSharedMemory();
	signal(SIGUSR2, timeoutSignal);//registers the signal handler for timeoutsignal
	signal(SIGINT, sigHandler);//registers the signal handler for crtl ^c
	FILE* fptr;//file pointer
	char* filename;//name of the file
	char line[256];//one string
	char c;//gets each character of the file
	int opt;//for getopt
	int count = 0;//count for putting strings into 2d array

	//getopt to get arguments from cmd line
	while ((opt = getopt(argc, argv, "hn:xs:xt:x")) != -1)
	{
		switch (opt)
		{
		case 'h'://help command
			printf("commands:\n");
			printf("master [-n x] [-s x] [-t time] infile\n");
			printf("\n");
			printf("-n x = Indicate the maximum total of child processes master will ever create.(Default 4)\n");
			printf("-s x = Indicate the number of children allowed to exist in the system at the same time.(Default 2)\n");
			printf("-t time = The time in seconds after which the process will terminate, even if it has not finished.(Default 100)\n");
			return 0;
		case 'n':
			childProcessTotal = atoi(optarg);//gets user value of total child processes
			if (childProcessTotal < 0)
			{
				perror("maximum total of child process can not be <= 0");
				return 1;
			}
			if (childProcessTotal > 20)//hard limit for childProcessTotal
			{
				childProcessTotal = 20;
			}
			break;
		case 's':
			maxChildInSystem = atoi(optarg);//gets user value of maxChildInSystem
			if (maxChildInSystem < 0)
			{
				perror(" the number of children allowed to exist in the system at the same time can not be <= 0");
				return 1;
			}
			break;
		case 't':
			maxTime = atoi(optarg);//gets user value of time
			if (maxTime <= 0)
			{
				perror("time can not be <= 0");
				return 1;
			}
			break;
		default:
			fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
			return 1;

		}
	}

	if (argv[optind] == NULL)//check if user entered a file
	{
		perror("file not specified");
		exit(1);
	}
	else
	{
		filename = argv[optind];//gets file name from cmd line
	}

	fptr = fopen(filename, "r");//open file for reading

	if (fptr == NULL)//if the file exists
	{
		fprintf(stderr, "File %s not found\n", argv[1]);
		exit(1);
	}
	
	
	int i = 0;//initiaize i 
	
	while (fgets(line, sizeof(line), fptr)) 
	{
		if (i == 20)//hard limit for hsared array 20 processes = 20 strings
		{
			break;
		}
		line[strlen(line) - 1] = '\0';//adds null char to the end of each string
		strcpy(shmptr->data[i], line);//copys string into array
		i++;//increment i for index of array
	}
	childProcessTotal = i;//i is hard limited at 20 but if there are less process than the strings in the file, then it sets the number of processes to the number of strings
	shmptr->numOfChild = childProcessTotal;//gets total number of childern and puts into struct to be shared through memory
	time(&startTime);//start the timer
	//time(&curtime);

	FILE* log = fopen("output.log", "w");//make file called output.log
	fprintf(log,"PID\t Index\t String\t Time\t\n");
	if (log == NULL)
	{
		/* File not created hence exit */
		perror("Failed to open file:output.log");
		exit(1);
	}
	while (currChildCount < childProcessTotal)//loop to create the processes
	{
		createChildProcess(currChildCount);//create child
		fprintf(log, "%d\t  %d\t %s\t %s\t\n", getpid(), currChildCount, shmptr->data[currChildCount], printTime());
		currChildCount++;//each time a child is created increment currChildCount
	}
	fclose(log);//close file
	
	time(&currTime);//gets current time
	while (((difftime(currTime, startTime)) < maxTime) && currChildSysCount > 0)//wait for all children to be finished with processing
	{
		wait(NULL);
		--currChildSysCount;
		//printf("Processes in system:%d\n", currChildSysCount);
	}
	printf("done processing at %s\n", printTime());
	//release memory
	
	shmdt(shmptr);//detach shared memory
	shmctl(shmid, IPC_RMID, NULL);//remove from memory
	exit(0);
	
	
}

