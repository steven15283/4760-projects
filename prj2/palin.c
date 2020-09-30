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
#include <ctime>
//steven guo
//9/20/2020


#define LENGTH 256 //max number of characters in a string
#define MAXPROCESS 20 //max number of processes

typedef struct {
	char data[MAXPROCESS][LENGTH]; //array of strings to check if its a palindrome
	int flag[MAXPROCESS];// array for flag
	int turn;// number for turn
	int numOfChild;//total number of children allowed
} shared_memory;

enum state { idle, want_in, in_cs };//specifies the flags

bool isPalindrome(char str[])//checks if the string is a palindrome
{
	bool palindrome = true;//sets boolean to true
	int lm = 0;//left most index
	int rm = strlen(str) - 1;//right most index

	while (rm > lm)//this while loop checks both ends of the string,working its way to the middle to check if its a palindrome
	{
		if (str[lm++] != str[rm--])// if characters at the opposite ends are not equal then the string is not a palindrome
		{
			palindrome = false;
			break;
		}
	}
	return palindrome;
}

void sortPalinOutput(char str[], bool palindrome)//sorts the palindrome into two files: palin.out and nopalin.out
{
	if (bool palindrome)// if palindrome is true then the string is a palindrome. output the string into the file
	{
		FILE* palinYes = fopen("palin.out", "w");//make file called palin.out
		if (palinYes == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:palin.out");
			exit(1);
		}

		fputs(str[], palinYes);//write the string to file
		fputs("\n", palinYes);//write a new line char for next string
		fclose(palinYes);//close file

	}
	else
	{
		FILE* palinNo = fopen("nopalin.out", "w");//make file called nopalin.out
		if (palinNo == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:nopalin.out");
			exit(1);
		}
		fputs(str[], palinNo);//write the string to file
		fputs("\n", palinNo);//write a new line char for next string
		fclose(palinNo);//close file
	}

}

void process(const int i)// critical section. int i is the index of array(so you can get the right string from array)
{
	int n = shmptr->numOfChild;// sets n equal to the total number of children
	int j;

	do
	{
		do
		{
			
			shmptr->flag[i] = want_in; // Raise my flag
			j = shmptr->turn; // Set local variable
			while (j != i)
				j = (shmptr->flag[j] != idle) ? shmptr->turn : (j + 1) % n;//?
			// Declare intention to enter critical section
			printf("Process:%d - wants to enter CS\n", i);
			flag[i] = in_cs;
			// Check that no one else is in critical section
			for (j = 0; j < n; j++)
				if ((j != i) && (flag[j] == in_cs))//?
					break;
		} while (j < n) || (shmptr->turn != i && shmptr->flag[turn] != idle);//?

		printf("Process:%d - entered CS\n", i);
		shmptr->turn = i;// Assign turn to self and enter critical section

		if (isPalindrome(shmptr->data[i]))
		{
			sleep(rand() % 3);//0-2 second delay
			sortPalinOutput(shmptr->data[i], true)//sort the string
		}
		else
		{
			sleep(rand() % 3);//0-2 second delay
			sortPalinOutput(shmptr->data[i], false)//sort the string
		}

		// Exit section
		j = (shmptr->turn + 1) % n;
		printf("Process:%d - exited CS\n", i);
		while (shmptr->flag[j] == idle)
		{
			j = (j + 1) % n;
		}

		// Assign turn to next waiting process; change own flag to idle
		shmptr->turn = j;
		shmptr->flag[i] = idle;
		kill(getppid(), SIGUSR2);
	} while (1);
}

int main(int argc, char* argv[])
{
	int index = atoi(argv[1]);//gets index number which is sent in by cmd line argument
	int key = ftok("makefile", 'a');//unique key from ftok
	int shmid = shmget(key, sizeof(shared_memory), (S_IRUSR | S_IWUSR | IPC_CREAT);//obtain access to a shared memory segment.

	if (shmid < 0)
	{
		perror("shmget error");
		exit(1);
	}
	else
	{
		shared_memory* shmptr = (shared_memory*)shmat(shmid, NULL, 0);// shmat to attach to shared memory
	}
	
	process(index);// runs critical section
	return 0;
}


