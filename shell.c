#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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
int pipeIndex(char*);
void execute_pipe(char* ,int);
void pipe_cmd(char** cmd1, char** );
int inp,out;

int main(){
	inp=dup(0);
	out=dup(1);
	signal(SIGINT,SIG_IGN);
	char *cmdline;
	char** arglist;   
	while((cmdline = read_cmd(stdin)) != NULL){
		int pipeIndex=0;
		int m=0;	

		char *loc;
		char* pipeToken;
		pipeToken=strtok(cmdline,"\0");
		loc = strchr(pipeToken, '|');
		pipeIndex=loc-pipeToken;

		if(loc != NULL){
			execute_pipe(cmdline,pipeIndex);
			wait(NULL);
		}
		else{
			if((arglist = tokenize(cmdline)) != NULL){
				execute(arglist);						
				free(arglist);
				free(cmdline);
			}
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
			dup2(inp,0);
			dup2(out,1);
			printf("child exited with status %d \n", status >> 8);
		return 0;
	}
}

char** tokenize(char* cmdline){
	char** arglist = (char**)malloc(sizeof(char*)* (MAXARGS+1));
	for(int j=0; j < MAXARGS+1; j++){
		arglist[j] = (char*)malloc(sizeof(char)* ARGLEN);
		bzero(arglist[j],ARGLEN);
	}
	char* token;
	int argnum=0;
	int in,out;
	token=strtok(cmdline," ");
	while(token!=NULL){
		if(strstr(token,"<")){
			token = strtok (NULL, " ");
			in = open(token, O_RDONLY);
			dup2(in, STDIN_FILENO);
			close(in);
			token = strtok (NULL, " ");
			continue;
		}
		if(strstr(token,">")){
			token = strtok (NULL, " ");
			out = open(token, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out, STDOUT_FILENO);
			close(out);
			token = strtok (NULL, " ");
			continue;
		}
		arglist[argnum]=token;
		argnum++;
		token=strtok(NULL," ");
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

void execute_pipe(char* cmdline,int index){
	char **arg1=(char**)malloc(sizeof(char*)*MAXARGS+1);
	for (int i=0;i<MAXARGS;i++){
		arg1[i]=(char*)malloc(sizeof(char)*ARGLEN);
		bzero(arg1[i],ARGLEN);
	}
	char **arg2=(char**)malloc(sizeof(char*)*MAXARGS+1);
	for (int k=0;k<MAXARGS;k++){
		arg2[k]=(char*)malloc(sizeof(char)*ARGLEN);
		bzero(arg2[k],ARGLEN);
	}
	char* token;
	int len,ind=0,j=0;
	token=strtok(cmdline," ");
	arg1[ind]=token;
	len=strlen(token);
	j=j+len+1;ind++;
	while(token!=NULL){
		if(j==index){
			arg1[ind]=NULL;
			break;
		}
		token=strtok(NULL," ");
		arg1[ind]=token;
		len=strlen(token);
		j=j+len+1;ind++;
	}
	ind=0;
	token=strtok(NULL," ");
	while(token!=NULL){
		token=strtok(NULL," ");
		if(token==NULL)
			break;
		arg2[ind]=token;
		len=strlen(token);
		ind++;
	}
	arg2[ind]=NULL;
	int pid=fork();
	if(pid==0)
		pipe_cmd(arg1,arg2);
}

void pipe_cmd(char** cmd1, char** cmd2) {
	int fds[2]; 
	int status;
	pipe(fds);
	pid_t pid;
	pid = fork();
	if (pid == 0)
	{ 
		close(fds[1]); 
		dup2(fds[0], STDIN_FILENO); 
		close(fds[0]); 
		execvp(cmd2[0], cmd2);
	}
	else
	{
		close(fds[0]); 
		dup2(fds[1], STDOUT_FILENO); 
		close(fds[1]);
		execvp(cmd1[0], cmd1);
	}
}
