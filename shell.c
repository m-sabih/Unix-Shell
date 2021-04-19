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

int execute(char* arglist[],int background);
char** tokenize(char* cmdline);
char* read_cmd(FILE*);
void handler(int n);
int pipeIndex(char*);
void execute_pipe(char* ,int);
void pipe_cmd(char** cmd1, char**);
void child_handler(int);
int inp,out;

int main(){
	inp=dup(0);
	out=dup(1);
	signal(SIGINT,SIG_IGN);
	char *cmdline;
	char** arglist;   
	while((cmdline = read_cmd(stdin)) != NULL){		
		int m=0,colon_count=0;
	    
	    char **total_commands=(char**)malloc(sizeof(char*)*MAXARGS+1);
   		for (int t=0;t<colon_count;t++){
      		total_commands[t]=(char*)malloc(sizeof(char)*ARGLEN);
      		bzero(total_commands[t],ARGLEN);
   		}
		char* colon_token;
      	int commands_count=0;
      	colon_token=strtok(cmdline,";");
      	while(colon_token!=NULL){
        	total_commands[commands_count]=colon_token;
        	commands_count++;
        	colon_token=strtok(NULL,";");
      	}
      	total_commands[commands_count]='\0';

      	for(int com=0;com<commands_count;com++){
      		int background=0;
      		int len=strlen(total_commands[com]);
	   		char* ncmdline=(char*) malloc(sizeof(char*)*len);
	   		bzero(ncmdline,len);
	      	char *loc;
	      	int pipeIndex=0;
	     	
			if(total_commands[com][len-1]=='&'){
	     	background=1;
	     	for(int p=0;p<len-2;p++){
	     		ncmdline[p]=total_commands[com][p];
	     	}
	     }
	     else{
	     	int cmd_ind=0;
	     	while(cmd_ind<len){
	     		ncmdline[cmd_ind]=total_commands[com][cmd_ind];
	     		cmd_ind++;
	     	}
	     	ncmdline[cmd_ind]='\0';
	     }	     	
	     	loc = strchr(ncmdline, '|');
	     	pipeIndex=loc-ncmdline;
			if(loc != NULL){
				execute_pipe(ncmdline,pipeIndex);
				wait(NULL);
			}
			else{
				if((arglist = tokenize(ncmdline)) != NULL){
					execute(arglist,background);						
					free(arglist);
					free(ncmdline);
				}
			}
		}
  	   	free(total_commands);
	}
	printf("\n");
	return 0;
}

int execute(char* arglist[],int background){
	int status;
	int cpid = fork();
	switch(cpid){
		case -1:
			perror("fork failed");
			exit(1);
		case 0:
			if(background==1)
      	 		setpgrp();
			signal(SIGINT,SIG_DFL);
			execvp(arglist[0], arglist);
			perror("Command not found...");
			exit(1);
		default:
			if (background == 0){
	        	waitpid(cpid, &status, 0);         	
	         	dup2(inp,0);
	         	dup2(out,1);
    		}
    		else{
       			printf("Process created with PID: %d\n",cpid);
				signal(SIGCHLD,child_handler);	       	
       	}
		return 0;
	}
}

void child_handler(int n){
	while(waitpid(-1,NULL,WNOHANG)>0){}
	printf("\n");
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
