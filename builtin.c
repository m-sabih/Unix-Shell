#include "builtin.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <dirent.h>

extern int jobs[];
extern int totalJobs;

//Funtions related to Internal shell commands
int BuiltInCd(char** arglist){
	//change directory based on path received. if no path then change to home folder
	if (arglist[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	else{ 
		if (chdir(arglist[1]) == -1) {
			printf(" %s: no such directory\n", arglist[1]);
            return -1;
		}
	}
	return 0;
}

void BuiltInJob() {	
	//get the information from the jobs array which contains pid, used that pid to access proc directory
	char dirName[100];
	char name[100];
	char state;
	long pid;
	FILE * fp = NULL;  
	struct dirent * entry;

	int jobsCount=0;
	for(int i=0;i<totalJobs;i++){
		snprintf(dirName, sizeof(dirName), "/proc/%d/stat", jobs[i]); 
		DIR* dp = opendir(dirName);
		fp = fopen(dirName, "r");

		if (fp) {
			jobsCount++;
			fscanf(fp, "%ld %s %c", &pid, name, &state);
			switch (state) {
				case 'R':
				case 'S':
				case 'D':
				case 'I':
					printf("[%d]     Running		%d\n",jobsCount,jobs[i]);
					break;				
				case 'T':
					printf("[%d]     Stopped 		%d\n",jobsCount,jobs[i]);
					break;
				default:
					break;
				}
			fclose(fp);
		}
		closedir(dp);
	}		
}

void BuiltInKill(char* pid){
	//kill the process
	int n=atoi(pid);
	kill(n, SIGKILL);
}

void BuiltInHelp(char *arg){
	// show help based on arguments recieved
	if(arg == NULL)
		printf("Shell built in commands include \tkill, jobs, cd, exit. \nUse help [command] to get more details \n");
	else if (strcmp(arg,"cd") == 0) 
		printf("cd command is used to change directories: cd [path] \n");
	else if (strcmp(arg,"jobs") == 0) 
		printf("jobs command is used to tell about the background processes: jobs \n");
	else if (strcmp(arg,"exit") == 0) 
		printf("exit is used to exit shell: exit \n");
	else if (strcmp(arg,"kill") == 0) 
		printf("kill command is used to kill a process with a specific id: kill[pid] \n");	
	else
		printf("No related command found, try help \n");
}