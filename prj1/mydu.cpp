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

int depthfirstapply(char* path, int pathfun(char* path1, std::vector<int>* option));
int sizepathfun(char* path, std::vector<int>* option);


int sizepathfun(char* path, std::vector<int>* option)
{
	struct stat st;
	if (stat(path, &st) == -1)
	{
		perror("File error");
		return -1;
	}
	if (S_ISREG(st.st_mode) == 0)
	{
		return -1;
	}

}

int depthfirstapply(char* path, int pathfun(char* path1, std::vector<int>* option))
{

	struct stat st;
	int totalSize = 0; //size
	struct dirent* ent; //directory entry at the current position in directory stream
	DIR* dir = opendir(path); //open directory

	if (!dir)//unable to open directory
	{
		return -1;
	}

	while ((ent = readdir(dir)) != NULL)//traverses directory until dir is not a directory
	{

	}
	closedir(dir);//close directory
	return count;//returns the size of the files in this directory(path)
}

int main(int argc, char* argv[])
{



}