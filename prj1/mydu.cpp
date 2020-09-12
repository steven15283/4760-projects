#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <vector> 
//steven guo
//9/9/2020

int depthfirstapply(char* path, int pathfun(char* path1, std::vector<char>* option));
int sizepathfun(char* path, std::vector<char>* option);


int sizepathfun(char* path, std::vector<char>* option)
{
	struct stat fileStat;
	if (stat(path, &fileStat) == -1)
	{
		perror("File error");
		return -1;
	}
	if (S_ISREG(fileStat.st_mode) == 0)//checks if its a regular file
	{
		return -1;
	}
	else
	{

		if (std::find(option.begin(), option->end(), 'B') != option.end() || std::find(option.begin(), option->end(), 'b') != option.end() || std::find(option.begin(), option->end(), 'm') != option.end())
		{
			return statbuf.st_size;
		}
		else
		{
			return (statbuf.st_blocks / 2);
		}
	}




}

int depthfirstapply(char* path, int pathfun(char* path1, std::vector<int>* option))
{

	struct stat fileStat;
	int totalSize = 0; //size
	struct dirent* ent; //directory entry at the current position in directory stream
	char* filePath;
	DIR* dir = opendir(path); //open directory

	if (!dir)//unable to open directory
	{
		return -1;
	}

	while ((ent = readdir(dir)) != NULL)//traverses directory until dir is not a directory
	{
		filePath = ent->d_name; // current file path

	}
	closedir(dir);//close directory
	return count;//returns the size of the files in this directory(path)
}

int main(int argc, char* argv[])
{
	std::vector<int> option;
	int input;


	while (input = getopt(argc, argv, "hLHbacsB:md:")) != -1)
	{
		switch (input)
		{
			case 'a':
				option.push_back('a');
				break;
			case 'B':
				option.push_back('B');
				break;
			case 'b':
				option.push_back('b');
				break;
			case 'c':
				option.push_back('c');
				break;
			case 'd':
				option.push_back('d');
				break;
			case 'h':
				printf("commands:\n");
				printf("mydu [-a] [-B M | -b | -m] [-c] [-d N] [-H] [-L] [-s] <dir1> <dir2>\n");
				printf("\n");
				printf("-a = Write count for all files, not just directories.\n");
				printf("-B M = Scale sizes by M before printing; for example, -BM prints size in units of 1,048,576 bytes.\n");
				printf("-b = Print size in bytes.\n");
				printf("-c = Print a grand total.\n");
				printf("-d N = Print the total for a directory only if it is N or fewer levels below the command line argument.\n");
				printf("-h = Print a help message or usage, and exit\n");
				printf("-H = Human readable; print size in human readable format, for example, 1K, 234M, 2G.\n");
				printf("-L = Dereference all symbolic links. By default, you will not dereference symbolic links.\n");
				printf("-m = Same as -B 1048576.\n");
				printf("-s = Display only a total for each argument.\n");
				return EXIT_SUCCESS;

			case 'H':
				option.push_back('H');
				break;
			case 'L':
				option.push_back('L');
				break;
			case 'm':
				option.push_back('m');
				break;
			case 's':
				option.push_back('s');
				break;
			case 'd':
				option.push_back('d');
				break;
			default:
				perror("invalid command line arguments");
		}
	}


}