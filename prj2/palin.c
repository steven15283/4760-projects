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


#define LENGTH 255
#define MAXPROCESS 20

typedef struct {
	char data[MAXPROCESS][LENGTH];
	int flag[MAXPROCESS];
	int turn;
	int numOfChild;
} shared_memory;

enum state { idle, want_in, in_cs };

void process(const int i)
{
	int n = shmptr->numOfChild;
	int j;

	do
	{
		do
		{
			shmptr->flag[i] = want_in; // Raise my flag
			j = shmptr->turn; // Set local variable
			while (j != i)
				j = (shmptr->flag[j] != idle) ? shmptr->turn : (j + 1) % n;
			// Declare intention to enter critical section
			flag[i] = in_cs;
			// Check that no one else is in critical section
			for (j = 0; j < n; j++)
				if ((j != i) && (flag[j] == in_cs))
					break;
		} while (j < n) || (shmptr->turn != i && shmptr->flag[turn] != idle);
		// Assign turn to self and enter critical section
		shmptr->turn = i;
		if (isPalindrome(shmptr->data[i]))
		{
			sleep(rand() % 3);
			sortPalinOutput(shmptr->data[i], true)
		}
		else
		{
			sleep(rand() % 3);
			sortPalinOutput(shmptr->data[i], false)
		}
		// Exit section
		j = (shmptr->turn + 1) % n;
		while (shmptr->flag[j] == idle)
		{
			j = (j + 1) % n;
		}

		// Assign turn to next waiting process; change own flag to idle
		shmptr->turn = j;
		shmptr->flag[i] = idle;
		remainder_section();
	} while (1);
}

bool isPalindrome(char str[])
{
	bool palindrome = true;
	int lm = 0;//left most index
	int rm = strlen(str) - 1;//right most index

	while (rm > lm)
	{
		if (str[lm++] != str[rm--])
		{
			palindrome = false;
			break;
		}
	}
	return palindrome;
}

void sortPalinOutput(char str[], bool palindrome)
{
	if (bool palindrome)
	{
		FILE* palinYes = fopen("palin.out", "w");
		if (palinYes == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:palin.out");
			exit(EXIT_FAILURE);
		}

		fputs(str[], palinYes);
		fputs("\n", palinYes);
		fclose(palinYes);

	}
	else
	{
		FILE* palinNo = fopen("nopalin.out", "w");
		if (palinNo == NULL)
		{
			/* File not created hence exit */
			perror("Failed to open file:nopalin.out");
			exit(EXIT_FAILURE);
		}
		fputs(str[], palinNo);
		fputs("\n", palinNo);
		fclose(palinNo);
	}

}

int main(int argc, char* argv[])
{
	int index = atoi(argv[1]);

	int key = ftok("Makefile", 'a');

	int shmid = shmget(key, sizeof(shared_memory), (S_IRUSR | S_IWUSR | IPC_CREAT);

	if (shmid < 0)
	{
		perror("shmget error");
		exit(1);
	}
	else
	{
		shared_memory* shmptr = (shared_memory*)shmat(shmid, NULL, 0);// shmat to attach to shared memory
	}
	
	process(index);

}


