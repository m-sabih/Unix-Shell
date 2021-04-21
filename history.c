#include "history.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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