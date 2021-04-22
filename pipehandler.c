#include "pipehandler.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30

//functions for handling pipe
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
  //separating command based on spacing/tokens
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
  
    free(arg1);
    free(arg2);
}

void PipeCmd(char** cmd1, char** cmd2) {
  int fds[2]; 
  int status;
  pipe(fds);
  pid_t pid;
  pid = fork();
  //single way communication of pipe for two processes
  if (pid == 0)
  { 
    //child process
    close(fds[1]); 
    dup2(fds[0], STDIN_FILENO); 
    close(fds[0]); 
    execvp(cmd2[0], cmd2);
  }
  else
  {
    //parent process
    close(fds[0]); 
    dup2(fds[1], STDOUT_FILENO); 
    close(fds[1]);
    execvp(cmd1[0], cmd1);
  }
}