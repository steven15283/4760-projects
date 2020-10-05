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
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <semaphore.h>

//steven guo
//10/4/2020


#define LENGTH 256 //max number of characters in a string
#define MAXPROCESS 20 //max number of processes

typedef struct {
	char data[MAXPROCESS][LENGTH]; //array of strings to check if its a palindrome
	int semState;
	int turn;// number for turn
	int flag[MAXPROCESS];// array for flag
	int numOfChild;//total number of children allowed
} shared_memory;

enum state { idle, want_in, in_cs };//specifies the flags

//needed to be declared to be used with all functions
shared_memory* shmptr;
int shmid;
int key;
int indexNum = 0;
char* printTime();//prints the current time

struct sembuf sbuf;
int semid;
key_t semKey;


void semWait(int semid, int semnum)
{
	buf.sem_num = semnum;
	buf.sem_op = 1;
	buf.sem_flg = 0;
	if (semop(semid, &buf, 1) == -1) 
	{
		perror("Opening semaphore");
		exit(1);
	}
}

void semSig(int semid, int semnum);
{
	buf.sem_num = semnum;
	buf.sem_op = -1;
	buf.sem_flg = 0;
	if (semop(semid, &buf, 1) == -1) 
	{
		perror("Closing semaphore");
		exit(1);
	}
}

int isPalindrome(char str[])//checks if the string is a palindrome
{
	int palindrome = 0;//sets boolean to true
	int lm = 0;//left most index
	int rm = strlen(str) - 1;//right most index

	while (rm > lm)//this while loop checks both ends of the string,working its way to the middle to check if its a palindrome
	{
		if (tolower(str[lm++]) != tolower(str[rm--]))// if characters at the opposite ends are not equal then the string is not a palindrome
		{
			palindrome = 1;//1 means its not a palindrome
			break;
		}
	}
	return palindrome;
}

void sortPalinOutput(char str[], int palindrome)//sorts the palindrome into two files: palin.out and nopalin.out
{
	if (palindrome == 0)// if palindrome is true then the string is a palindrome. output the string into the file
	{
		FILE* palinYes = fopen("palin.out", "a+");//make file called palin.out
		if (palinYes == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:palin.out");
			exit(1);
		}

		fputs(str, palinYes);//write the string to file
		fputs("\n", palinYes);//write a new line char for next string
		fclose(palinYes);//close file

	}
	else
	{
		FILE* palinNo = fopen("nopalin.out", "a+");//make file called nopalin.out
		if (palinNo == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:nopalin.out");
			exit(1);
		}
		fputs(str, palinNo);//write the string to file
		fputs("\n", palinNo);//write a new line char for next string
		fclose(palinNo);//close file
	}

}

void process(const int i)// critical section. int i is the index of array(so you can get the right string from array)
{
	semKey = ftok("makefile", 'c');
	printf("Process:%d - wants to enter CS - %s\n", i, printTime());
	semid = semget(semKey, 1, IPC_CREAT | 0600);
	if (semid == -1) 
	{
		perror("Creating array of sems");
		exit(1);
	}
	if (semctl(semid, 0, SETVAL, (int)1) == -1) 
	{
		perror("Setting value to 1");
		exit(1);
	}
	/*
	int n = shmptr->numOfChild;// sets n equal to the total number of children
	int j;
	do
	{
		
		shmptr->flag[i] = want_in; // Raise my flag
		j = shmptr->turn; // Set local variable
		while (j != i)//while its not our turn
			//j = (shmptr->flag[j] != idle) ? shmptr->turn : (j + 1) % n;//find who is next
		 Declare intention to enter critical section
		
		shmptr->flag[i] = in_cs;
	
		for (j = 0; j < n; j++)// Check that no one else is in critical section
		{ 
			if ((j != i) && (shmptr->flag[j] == in_cs))//if its not our turn and if our flag is in_cs
			{
				break;
			}
		}
				
	} while ((j < n) || ((shmptr->turn != i) && (shmptr->flag[shmptr->turn] != idle)));
	*/
	semWait(semid, 0);
	printf("Process:%d - entered CS - %s\n", i, printTime());
	

	//shmptr->turn = i;// Assign turn to self and enter critical section

	if (isPalindrome(shmptr->data[i]) == 0)
	{
		sleep(rand() % 3);//0-2 second delay
		sortPalinOutput(shmptr->data[i], 0);//sort the string into appropriate file
	}
	else
	{
		sleep(rand() % 3);//0-2 second delay
		sortPalinOutput(shmptr->data[i], 1);//sort the string int appropriate file
	}

	// Exit critical section
	semSig(semid, 0);
	printf("Process:%d - exited CS - %s\n", i, printTime());
	/*
	j = (shmptr->turn + 1) % n;//find whatever process is next
	
	while (shmptr->flag[j] == idle)
	{
		j = (j + 1) % n;
	}

	// Assign turn to next process waiting to enter cs
	shmptr->turn = j;
	shmptr->flag[i] = idle; //change own flag to idle
	*/
	//printf("before killing process: %d\n", i);
	sem_unlink(semid);
	kill(getppid(), SIGUSR2);//kills process
}

char* printTime()//prints current time
{
	time_t t;
	time(&t);//records current time
	return ctime(&t);//returns the string format 
}

int main(int argc, char* argv[])
{
	indexNum = atoi(argv[1]);//gets index number which is sent in by cmd line argument
	key = ftok("makefile", 'a');//unique key from ftok
	shmid = shmget(key, sizeof(shared_memory), (S_IRUSR | S_IWUSR | IPC_CREAT));//obtain access to a shared memory segment.
	
	if (shmid < 0)//unable to obtain access to a shared memory segment.
	{
		perror("shmget error");
		exit(1);
	}
	else
	{
		shmptr = (shared_memory*)shmat(shmid, NULL, 0);// shmat to attach to shared memory
	}
	process(indexNum);// runs critical section
	return 0;
}


