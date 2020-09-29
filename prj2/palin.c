#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//steven guo
//9/20/2020

enum state { idle, want_in, in_cs };

bool isPalindrome(char str[]);

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
	
	//start at line 132

}
bool isPalindrome(char str[], int index)
{
	FILE* palinYes = fopen("palin.out", "w");
	FILE* palinNo = fopen("nopalin.out", "w");
	if (palinYes == NULL)
	{
		/* File not created hence exit */
		perror("Failed to open file:palin.out");
		exit(EXIT_FAILURE);
	}
	if (palinNo == NULL)
	{
		/* File not created hence exit */
		perror("Failed to open file:nopalin.out");
		exit(EXIT_FAILURE);
	}
	bool palindromeChk = true;
	int lm = 0;//left most index
	int rm = strlen(str) - 1;//right most index

	while (rm > lm)
	{
		if (str[lm++] != str[rm--])
		{
			palindrome = false;
			fputs(str[], palinNo);
			fputs("\n", palinNo);
			fclose(palinNo);
			break;
		}
	}
	fputs(str[], palinYes);
	fputs("\n", palinYes);
	fclose(palinYes);
	return palindromeChk;
}
