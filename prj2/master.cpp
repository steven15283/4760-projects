#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>



int main(int argc, char* argv[])
{

	int childProcessNum = 4;
	int childSystemNum = 2;
	int time = 100;
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
			option.push_back('n');
			childProcessNum = atoi(optarg);
			break;
		case 's':
			option.push_back('s');
			childSystemNum = atoi(optarg);
			break;
		case 't':
			option.push_back('t');
			time = atoi(optarg);
			break;
		default:
			fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
			return EXIT_FAILURE;

		}
	}

}