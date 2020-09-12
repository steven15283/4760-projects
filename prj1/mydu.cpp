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

int depthfirstapply(char* path, int pathfun(char* path1,), int scale, int depth, int ino, int math_depth);
int sizepathfun(char* path);
void printHuman(int size, char* pathname, int scale);

static std::vector<int> option;
int sizepathfun(char* path)
{
	struct stat fileStat;

	if (lstat(path, &fileStat) == -1)//checks if fileStat can be accessed
	{
		perror("File error");
		return -1;
	}
	if (S_ISREG(fileStat.st_mode) == 0)//checks if fileStat a regular file
	{
		return -1;
	}
	else
	{

		if (std::find(option.begin(), option.end(), 'B') != option.end() || std::find(option.begin(), option.end(), 'b') != option.end() || std::find(option.begin(), option.end(), 'm') != option.end())//when the arguments are B,b,m 
		{//it returns it in bytes
			return statbuf.st_size;
		}
		else
		{//it returns it in blocks
			return (statbuf.st_blocks / 2);
		}
	}




}

int depthfirstapply(char* path, int pathfun(char* path1),  int scale, int depth, int max_depth)
{
	struct stat info;
	int totalSize = 0; //size
	struct dirent* ent; //to hold directory entry at the current position in directory stream
	char* filePath;//holds filepath used to 
	DIR* dir = opendir(path); //open directory

	if (!dir)//unable to open directory
	{
		return -1;
	}

	while ((ent = readdir(dir)) != NULL)//traverses directory until dir is not a directory
	{
		char* pathname = ent->d_name; // current name of the path of the directory
		char pathname[4096];//holds current pathname for directory

		sprintf(filePath, "%s/%s", path, pathname);// save pathname
		lstat(filePath, &info);// //gets the attributes of filepath
		mode_t mode = info.st_mode; //gets the file type attribute of filepath
		if (S_ISDIR(mode))//checks if mode is a directory
		{
			int size = depthfirstapply(pathname, pathfun, scale, depth + 1, max_depth);//gets size of directory and goes into directory
			size += info.st_size;
			if (size >= 0) // checks the size of directory
			{
				totalSize += size;//accumulate total size
				if (std::find(option.begin(), option.end(), 's') != option.end())//if argument has s then it skips the printing
				{
					continue;
				}
				
				if ((std::find(option.begin(), option.end(), 'H') != option.end()) || (std::find(option.begin(), option.end(), 'B') != option.end()) || (std::find(option.begin(), option.end(), 'm') != option.end())//check if H,B,m is in arguments
				{
					printHuman(size, pathname, options, scale);//when H,B,m is in arguments go to printHuman for unit conversion
				}
				else if (std::find(option.begin(), option.end(), 'd') != option.end())//check if d is in argument
				{
					if (depth >= max_depth)//check if depth > max_depth
					{
						printHuman(size, pathname, options, scale);//print when max_depth is not below depth
					}
					else
					{

					}

				}
				else
				{
					printf("%-15s %d\n", pathname, size);
				}
			}

		}
		else
		{
			int size = pathfun(pathname, scale);//get size of files
			if (size > 0)//adds file sizes
			{
				totalSize += size;
			}
			if (std::find(option.begin(), option.end(), 's') != option.end())//if arguemnt is s then skip printing
			{
				continue;
			}
			if ((std::find(option.begin(), option.end(), 'B') != option.end()) || (std::find(option.begin(), option.end(), 'm') != option.end())//if B or m is in arguments make the size=1 when size is less than 1
			{
				if (size < 1)
				{
					size = 1;
				}
				
			}

			if (std::find(option.begin(), option.end(), 'a') != option.end())//check if a is in arguments
			{//prints out the files with directories from this point on
				
				
				if ((std::find(option.begin(), option.end(), 'H') != option.end()) || (std::find(option.begin(), option.end(), 'B') != option.end()) || (std::find(option.begin(), option.end(), 'm') != option.end()))//check if H,B,m is in arguments
				{
					printHuman(size, pathname, options, scale);//when H,B,m is in arguments go to printHuman for unit conversion
				}
				else if (std::find(option.begin(), option.end(), 'd') != option.end())//check if d is in argument
				{
					if (depth >= max_depth)//check if depth > max_depth
					{
						printHuman(size, pathname, options, scale);//print when max_depth is not below depth
					}
					else
					{

					}

				}
				else
				{
					printf("%-15s %d\n", pathname, size);
				}
			}
				
		}
	}
	closedir(dir);//close directory
	return totalSize;//returns the size of the files in this directory(path)
}

void printHuman(int size, char* pathname, int scale)
{
	char unit = ' ';
	if (std::find(option.begin(), option.end(), 'H') != option.end()) ) //checks if H is in arguments
	{
		if (size >= 1000000000)// convert to gigabyte if size is over that amount
		{
			size = (long long)(size / 1000000000);
			unit = 'G';
		}
		else if (size >= 1000000)// convert to megabyte if size is over that amount
		{
			size = (long long)(size / 1000000);
			unit = 'M';
		}
		else if (size >= 1000)// convert to kilabyte if size is over that amount
		{
			size = (long long)(size / 1000);
			unit = 'K';
		}
		printf("%s%-15d %c\n", pathname, size, unit );
	}
	if (std::find(option.begin(), option.end(), 'B') != option.end()) )//checks if B is in arguments
	{
		size = size / scale; //scales size by user input
		if (size < 1)//round up
		{
			size = 1;
		}
		printf("%-15s %d\n", pathname, size);

	}
	if (std::find(option.begin(), option.end(), 'm') != option.end())//scale by megabyte
	{
		size = size / 1000000;
		if (size < 1) size = 1;
		printf("%s%-15d %c\n", pathname, size, 'M');
	}
}

int main(int argc, char* argv[])
{
	
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