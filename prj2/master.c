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

#define SIZE 50
#define LENGTH 256

typedef struct {
	int index;  //key_t key;
	char data[SIZE][LENGTH];
} shared_memory;

//global vars
int childProcessNum = 4;//sets default number of child processes master will create
int childSystemNum = 2;//sets default number of child processes that can be in system at the same time
int time = 100; //sets default time
int currChildNum = 0;
int currSysChildNum = 0;
enum state { idle, want_in, in_cs };
extern int turn;
extern state flag[n]; // Flag corresponding to each process in shared memory

void createChild();

int main(int argc, char* argv[])
{
	FILE* fptr;//file pointer
	
	char* filename = NULL;//name of the file
	char line[255];//one string
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
			childProcessNum = atoi(optarg);
			if (childProcessNum <= 0)
			{
				perror("maximum total of child process can not be <= 0");
				return EXIT_FAILURE;
			}
			break;
		case 's':
			childSystemNum = atoi(optarg);
			if (childSystemNum <= 0)
			{
				perror(" the number of children allowed to exist in the system at the same time can not be <= 0");
				return EXIT_FAILURE;
			}
			break;
		case 't':
			time = atoi(optarg);
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
	shared_memory* shmptr = (shared_memory*)shmat(shmid, NULL, 0);

	// shmat to attach to shared memory 


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
	
	while (currChildNum < childProcessNum)
	{
		createChild(currChildNum);
		currChildNum++;
	}


	void createChild(int num)
	{
		if (currChildNum < childSystemNum)
		{
			childProcessNum--;
			currSysChildNum++;
			if (fork() == 0)
			{

			}

		}
		
		
	}

}