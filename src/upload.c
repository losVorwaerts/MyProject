#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include <string.h>

#include "make_log.h"


int upload(char *filename,char *fileid)
{
    //int n=0;
    pid_t pid;
    int fd[2];
    
  
    if(pipe(fd)<0)
    {
       // perror("pipe");
        LOG("fast","upload","pipe err");
        exit(1);
    }

    if((pid = fork())<0)
    {
        //perror("fork");
        LOG("fast","upload","fork err");
        exit(1);
    }
    if(pid==0)
    {
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        
        execl("/usr/bin/fdfs_upload_file","fdfs_upload_file","/etc/fdfs/client.conf",filename,NULL);
        LOG("fast","upload","exec err");
    }
    else{
        close(fd[1]);
        read(fd[0],fileid,256);
        fileid[strlen(fileid)-1] = '\0';
        LOG("fast","upload","%s",fileid);

        wait(NULL);
        close(fd[0]);
    }

    return 0;
}
