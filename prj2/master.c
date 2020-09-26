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


int main(int argc, char* argv[])
{
	FILE* fptr;
	int childProcessNum = 4;
	int childSystemNum = 2;
	int time = 100;
	char* filename = NULL;
	char* mylist[10];
	char* strArray[10];
	char line[255];
	char c;
	int lineCount = 0;
	int arrayCount = 0;
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
			break;
		case 's':
			childSystemNum = atoi(optarg);
			break;
		case 't':
			time = atoi(optarg);
			break;
		default:
			fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
			return EXIT_FAILURE;

		}
	}

	if (argv[optind] == NULL)
	{
		perror("file not specified");
	}
	else
	{
		filename = argv[optind];
	}

	fptr = fopen(filename, "r");
	if (fptr == NULL)
	{
		fprintf(stderr, "File %s not found\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	c = fgetc(fptr);

	if (isalpha(c))
	{
		line[lineCount] = c;
		count++;
	}
	elseif(isspace(c))
	{

	}
	else
	{

	}

	while (c != EOF)
	{
		c = fgetc(fptr);
		if (isalpha(c))
		{
			line[lineCount] = c;
			count++;
		}
		elseif (isspace(c))
		{

		}
		else
		{
			if (c == '\n')
			{
				line[lineCount] = c;
				strArray[arrayCount] = line;
				mylist[arrayCount] = *strArray[arrayCount];
				arrayCount++;
				char line[255];
				count = 0;

			}
		}		
	}
		


	

}