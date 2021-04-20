#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include<dirent.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30

int Execute(char* arglist[],int);
char** Tokenize(char*);
char* ReadCmd(FILE*);
void ExecutePipe(char* ,int);
void PipeCmd(char**, char**);
void ChildHandler(int);
void SaveCommandToFile(char*);
char* GetCommandFromFile(char*);
int BuiltInCd(char* arglist[]);
void BuiltInJob();
int inp,out;

static int jobs[100];
static int totalJobs=0;

int main(){
	inp=dup(0);
	out=dup(1);
	signal(SIGINT,SIG_IGN);
	char *cmdline;
	char** arglist;   
	while((cmdline = ReadCmd(stdin)) != NULL){		
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
	         		else
						Execute(arglist,background);						
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

void SaveCommandToFile(char* cmdline){
    int linesCount=0;
    char line[500];
    FILE *lineCheck=fopen("history.txt","r");
    if(lineCheck !=NULL)
    {
    	while((fgets(line, 500, lineCheck))!=NULL ){
      		linesCount++;
   		}
        fclose(lineCheck);
    }
    else
        linesCount=0;
    if(linesCount<10)
    {
        FILE *fp=fopen("history.txt","a+");
        fputs(cmdline,fp);
        fputs("\n",fp);
        fclose(fp);     
    }
    else
    {
        FILE *main=fopen("history.txt","r");
        FILE *temp=fopen("temp.txt","w");
        int j=1;
        fgets(line, 500, main);
        while(j<=9)
        {
	        fgets(line, 500, main);
	        fputs(line,temp);
	        j++;
        }
	    fputs(cmdline,temp);
        fputs("\n",temp);
	    fclose(main); 
	    fclose(temp);
	    remove("history.txt");
	    rename("temp.txt","history.txt");
    }
}

char* GetCommandFromFile(char* commandNumber){   
    int linesCount=0;    
    char* line=(char*)malloc(sizeof(char)*500);
    char* command=strtok(commandNumber,"!");
    FILE *commandsHistory=fopen("history.txt","r");
    if(command !=NULL)
    {
    	while((fgets(line, 500, commandsHistory))!=NULL ){
      		linesCount++;
      		//printf("command count %d Line: %s ", linesCount,line);
      		if(linesCount==atoi(command))
	        {   
	            command=line;
	            memset(command+strlen(command)-1,'\0',1);
	            return command;
	        }
   		}
   		if(strcmp(command,"-1")==0)
	    {
	        command=line;
	        memset(command+strlen(command)-1,'\0',1);
	        return command;
	    }
        fclose(commandsHistory);
    }    
    return "event not found";
}

int Execute(char* arglist[],int background){
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
       			printf("[%d] %d\n",totalJobs+1,cpid);
				signal(SIGCHLD,ChildHandler);	 
				jobs[totalJobs]=cpid;
       			totalJobs++;      	
       		}
		return 0;
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

int BuiltInCd(char** arglist){
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