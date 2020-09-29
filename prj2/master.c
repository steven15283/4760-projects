#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include<sys/shm.h>
//steven guo
//9/20/2020

#define LENGTH 256
#define MAXPROCESS 20

typedef struct {
	char data[MAXPROCESS][LENGTH]; //array of strings to check if its a palindrome
	int flag[MAXPROCESS];// array for flag
	int turn;// number for turn
	int numOfChild;
} shared_memory;

//global vars
int childProcessTotal = 4;//sets default number of child processes master will create
int total_Child_In_System = 2;//sets default number of child processes that can be in system at the same time
int time = 100; //sets default time
int currChildCount = 0;//sets the amount of children 
int currSysChildCount = 0;//sets the amount of children in the system

enum state { idle, want_in, in_cs };

extern state flag[n]; // Flag corresponding to each process in shared memory
int status = 0

void createChildProcess(int num)
{

	if (currChildCount < total_Child_In_System)
	{
		spawnChild(num);
	}
	else
	{
		waitpid(-(*parent), &status, 0);
		currSysChildCount--;
		cout << currentNumOfProcessesInSystem << " processes in system.\n";
		spawnChild(num);
	}


}

void spawnChild(int num)
{
	currSysChildCount++;
	if (fork() == 0)
	{
		if (num == 1)
		{
			(*parent) = getpid();
		}
		setpgid(0, (*parent));
		execl("./palin", "palin", to_string(num).c_str(), (char*)NULL);//executes palin with num as an argument
		exit(0);
	}
}

int main(int argc, char* argv[])
{
	FILE* fptr;//file pointer
	
	char* filename = NULL;//name of the file
	char line[256];//one string
	char c;//gets each character of the file
	int opt;//for getopt
	int count = 0;//count for putting strings into 2d array


	while ((opt = getopt(argc, argv, "hn:xs:xt:x")) != -1)
	{
		switch (opt)
		{
		case 'h':
			printf("commands:\n");
			printf("master [-n x] [-s x] [-t time] infile\n");
			printf("\n");
			printf("-n x = Indicate the maximum total of child processes master will ever create.(Default 4)\n");
			printf("-s x = Indicate the number of children allowed to exist in the system at the same time.(Default 2)\n");
			printf("-t time = The time in seconds after which the process will terminate, even if it has not finished.(Default 100)\n");
			return EXIT_SUCCESS;
		case 'n':
			childProcessTotal = atoi(optarg);//gets user value of total child processes
			if (childProcessTotal <= 0)
			{
				perror("maximum total of child process can not be <= 0");
				return EXIT_FAILURE;
			}
			if (childProcessTotal > 20)//hard limit for childProcessTotal
			{
				childProcessTotal = 20;
			}
			break;
		case 's':
			total_Child_In_System = atoi(optarg);//gets user value of total_Child_In_System
			if (total_Child_In_System <= 0)
			{
				perror(" the number of children allowed to exist in the system at the same time can not be <= 0");
				return EXIT_FAILURE;
			}
			break;
		case 't':
			time = atoi(optarg);//gets user value of time
			if (time <= 0)
			{
				perror("time can not be <= 0");
				return EXIT_FAILURE;
			}
			break;
		default:
			fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
			return EXIT_FAILURE;

		}
	}


	int key = ftok("Makefile", 'a');

	int shmid = shmget(key, sizeof(shared_memory), (S_IRUSR | S_IWUSR | IPC_CREAT);

	if (shmid < 0)
	{
		perror("shmget error");
		exit(1);
	}

	shared_memory* shmptr = (shared_memory*)shmat(shmid, NULL, 0);// shmat to attach to shared memory

	if (shmptr == (void*)-1)
	{
		perror("Failed to attach existing shared memory segment");
		return 1;
	}

	int parentKey = ftok("Makefile", 'b');
	int parentID;
	pid_t* parent;

	if (argv[optind] == NULL)//check if user entered a file
	{
		perror("file not specified");
	}
	else
	{
		filename = argv[optind];//gets file name from cmd line
	}

	fptr = fopen(filename, "r");//open file for reading

	if (fptr == NULL)
	{
		fprintf(stderr, "File %s not found\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	c = fgetc(fptr);

	if (isalpha(c))
	{
		line[count] = c;
		count++;
	}
	else if(isspace(c))
	{

	}
	else
	{

	}
	//puts each character from each line into line char array then puts into 2d array when \n ==c
	int i = 0;
	while (c != EOF)
	{
		if(i==20)//hard limit for hsared array 20 processes = 20 strings
		{
			break;
		}
		c = fgetc(fptr);
		if (isalpha(c))
		{
			line[count] = c;
			count++;
		}
		elseif (isspace(c))
		{

		}
		else
		{
			if (c == '\n')
			{
				line[count] = c;
				strcpy(shmptr->data[i], line);
				char line[LENGTH];//reset array for next line
				count = 0;
				i++;
			}
		}		
	}

	shmptr->numOfChild = childProcessTotal;//gets total number of childern and puts into struct to be shared through memory
	parentId = shmget(parentKey, sizeof(pid_t), IPC_CREAT | S_IRUSR | S_IWUSR);//allocate shared memory for parent id

	if (parentId < 0) 
	{
		perror("shmget: Failed to allocate shared memory for parent id");
		exit(1);
	}
	else 
	{
		parent = (pid_t*)shmat(parentID, NULL, 0);//attach parent into shared memory
	}

	while (currChildCount <= childProcessTotal)
	{
		createChildProcess(currChildCount);
		currChildCount++;
	}

	//release memory
	shmdt(shmptr);
	shmctl(shmid, IPC_RMID, NULL);
	exit(0);
	

}