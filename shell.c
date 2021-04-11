#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30

int execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(FILE*);
void handler(int n);

int main(){
	signal(SIGINT,SIG_IGN);
	char *cmdline;
	char** arglist;   
	while((cmdline = read_cmd(stdin)) != NULL){
		if((arglist = tokenize(cmdline)) != NULL){
			execute(arglist);
			for(int j=0; j < MAXARGS+1; j++)
				free(arglist[j]);
			free(arglist);
			free(cmdline);
		}
	}
	printf("\n");
	return 0;
}

int execute(char* arglist[]){
	int status;
	int cpid = fork();
	switch(cpid){
		case -1:
			perror("fork failed");
			exit(1);
		case 0:
			signal(SIGINT,SIG_DFL);
			execvp(arglist[0], arglist);
			perror("Command not found...");
			exit(1);
		default:
			waitpid(cpid, &status, 0);
			signal(SIGCHLD,handler);
			printf("child exited with status %d \n", status >> 8);
		return 0;
	}
}

void handler(int n){
	while(waitpid(-1,NULL,WNOHANG)>0){}
		printf("\n");
}

char** tokenize(char* cmdline){
	char** arglist = (char**)malloc(sizeof(char*)* (MAXARGS+1));
	for(int j=0; j < MAXARGS+1; j++){
		arglist[j] = (char*)malloc(sizeof(char)* ARGLEN);
		bzero(arglist[j],ARGLEN);
	}
	if(cmdline[0] == '\0')
		return NULL;
	int argnum = 0;
	char*cp = cmdline;
	char*start;
	int len;
	while(*cp != '\0'){
		while(*cp == ' ' || *cp == '\t')
			cp++;
		start = cp;
		len = 1;
		while(*++cp != '\0' && !(*cp ==' ' || *cp == '\t'))
			len++;
		strncpy(arglist[argnum], start, len);
		arglist[argnum][len] = '\0';
		argnum++;
	}
	arglist[argnum] = NULL;
	return arglist;
}      

char* read_cmd(FILE* fp){
	char prompt[1024];
	uid_t uid = getuid();
	struct passwd *psw = getpwuid(uid);
	getcwd(prompt,1024);
	printf("\033[1;32m%s@\033[1;34m%s\033[0m$",psw->pw_name, prompt);
	int c;
	int pos = 0;
	char* cmdline = (char*) malloc(sizeof(char)*MAX_LEN);
	while((c = getc(fp)) != EOF){
		if(c == '\n')
			break;
		cmdline[pos++] = c;
	}
	if(c == EOF && pos == 0) 
		return NULL;
	cmdline[pos] = '\0';
	return cmdline;
}
