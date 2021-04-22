#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <dirent.h>
#include <stdbool.h>

#include "history.h"
#include "builtin.h"

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30

int Execute(char* arglist[],int);
char** Tokenize(char*);
char* ReadCmd(FILE*);
void ExecutePipe(char* ,int);
void PipeCmd(char**, char**);
void ChildHandler(int);
int inp,out;

int jobs[100];
int totalJobs=0;

int main(){
	inp=dup(0);
	out=dup(1);
	signal(SIGINT,SIG_IGN);
	char *cmdline;
	char** arglist;   
	while((cmdline = ReadCmd(stdin)) != NULL){		
		int m=0;
	    
	    char **total_commands=(char**)malloc(sizeof(char*)*MAXARGS+1);
   		for (int t=0;t<MAXARGS;t++){
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

      	int ifCheck=0;
      	int returnStatus = 0;
      	bool skipNext=false;
      	for(int com=0;com<commands_count;com++){      		
      		int background=0;      		
      		int len=strlen(total_commands[com]);
	   		char* ncmdline=(char*) malloc(sizeof(char*)*len);
	   		bzero(ncmdline,len);
	      	char *loc;
	      	int pipeIndex=0;
	     	bool isIf=false;	     	
	     	if(skipNext==true){
	     		skipNext=false;
	     		continue;
	     	}
	     	if(total_commands[com][0]=='!'){	     		
	     		ncmdline = GetCommandFromFile(total_commands[com]);
	     		if(strcmp(ncmdline,"event not found")==0){
	     			printf("-bash: %s: event not found\n",total_commands[com]);
	     			break;
	     		}
	     	}
			else if(total_commands[com][len-1]=='&'){
		     	background=1;
		     	for(int p=0;p<len-1;p++){
		     		ncmdline[p]=total_commands[com][p];
		     	}
	     	}
	     	else if(strstr(total_commands[com],"if")!=NULL){
	     		isIf=true;	     		
	     		int cmd_ind=0;
	     		int cmd_ind2=3;
	     		while(cmd_ind2<len){
	     			ncmdline[cmd_ind]=total_commands[com][cmd_ind2];
	     			cmd_ind++;
	     			cmd_ind2++;
	     		}
	     		if(cmd_ind==0){
	     			printf("Invalid statement if\n");
	     			break;
	     		}
	     		else
	     			ncmdline[cmd_ind]='\0';	     		
	     	}
	     	else if(strstr(total_commands[com],"then")!=NULL){
	     		if(total_commands[com+1]!=NULL){
	     				if(strstr(total_commands[com+1],"fi")==NULL){
			     			if(strstr(total_commands[com+1],"else")==NULL){    	
			     				printf("Invalid syntax for if\n");
			     				break;
			     			}
	     				}
	     				else{
	     					skipNext=true;
	     				}
	     			}
	     			else{
			     		printf("Invalid syntax for if\n");
			     		break;
			    }
	     		if(ifCheck==1){
	     			int t=0;	     					
		     		int cmd_ind=0;
		     		int cmd_ind2=6;
		     		while(cmd_ind2<len){
		     			ncmdline[cmd_ind]=total_commands[com][cmd_ind2];
		     			cmd_ind++;
		     			cmd_ind2++;	
		     		}
		     		if(cmd_ind==0){
		     			printf("Invalid statement then\n");
		     			break;
		     		}
		     		else
		     			ncmdline[cmd_ind]='\0';
	     		}
	     		else
	     			continue;
	     	}
	     	else if(strstr(total_commands[com],"else")!=NULL){		     	
	     		if(total_commands[com+1]!=NULL){
	     				if(strstr(total_commands[com+1],"fi")==NULL){
	     					printf("Invalid syntax for if\n");
	     					break;
	     				}
	     				skipNext=true;
	     			}
		     		else{
	     				printf("Invalid syntax for if\n");
	     				break;
	     		}
	     		if(ifCheck==2){     			     			
		     		int cmd_ind=0;
		     		int cmd_ind2=6;
		     		while(cmd_ind2<len){
		     			ncmdline[cmd_ind]=total_commands[com][cmd_ind2];
		     			cmd_ind++;
		     			cmd_ind2++;	
		     		}
		     		if(cmd_ind==0){
		     			printf("Invalid statement then\n");
		     			break;
		     		}
		     		else
		     			ncmdline[cmd_ind]='\0';
	     		}
	     		else
	     			break;
	     	}
	     	else{
	     		int cmd_ind=0;
	     		while(cmd_ind<len){
	     			ncmdline[cmd_ind]=total_commands[com][cmd_ind];
	     			cmd_ind++;	     			
	     		}
	     		ncmdline[cmd_ind]='\0';
	     	}	     	
	     	SaveCommandToFile(ncmdline);

	     	loc = strchr(ncmdline, '|');
	     	pipeIndex=loc-ncmdline;
			if(loc != NULL){
				ExecutePipe(ncmdline,pipeIndex);
				wait(NULL);
			}
			else{
				if((arglist = Tokenize(ncmdline)) != NULL){
					if (strcmp(arglist[0],"cd") == 0)
	         			BuiltInCd(arglist);
	         		else if (strcmp(arglist[0],"exit") == 0) 
	         			exit(0);
	         		else if (strcmp(arglist[0],"jobs") == 0) 
	         			BuiltInJob();
	         		else if (strcmp(arglist[0],"kill") == 0)	         	
	         			BuiltInKill(arglist[1]);
	         		else if (strcmp(arglist[0],"help") == 0)	         		
	         			BuiltInHelp(arglist[1]);
	         		else{
						returnStatus = Execute(arglist,background);
						if(returnStatus==0 && isIf==true){
							ifCheck=1;	
							isIf=false;
						}
						else if(returnStatus==1 && isIf==true){
							ifCheck=2;	
							isIf=false;
						}						         			
					}
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

int Execute(char* arglist[],int background){
	int status, childStatus=0;
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
	        	if (WIFEXITED(status)) {
				    if(WEXITSTATUS(status)==1)
				    	childStatus=1;
				}        	
	         	dup2(inp,0);
	         	dup2(out,1);
    		}
    		else{
       			printf("[%d] %d\n",totalJobs+1,cpid);
				signal(SIGCHLD,ChildHandler);	 
				jobs[totalJobs]=cpid;
       			totalJobs++;      	
       		}
		return childStatus;
	}
}

void ChildHandler(int n){
	while(waitpid(-1,NULL,WNOHANG)>0){}	
}

char** Tokenize(char* cmdline){
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

char* ReadCmd(FILE* fp){
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

void ExecutePipe(char* cmdline,int index){
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
		PipeCmd(arg1,arg2);
}

void PipeCmd(char** cmd1, char** cmd2) {
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