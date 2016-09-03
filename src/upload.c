#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>

#include "make_log.h"

#define MAXLINE 1024

int main(int argc,char *argv[])
{
    //int n=0;
    pid_t pid;
    int fd[2];
    char line[MAXLINE];
    
    if(argc<2)
    {
        printf("usage ./upload filename\n");
        exit(1);
    }
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
        
        execl("/usr/bin/fdfs_upload_file","fdfs_upload_file","/etc/fdfs/client.conf",argv[1],NULL);
        LOG("fast","upload","exec err");
    }
    else{
        close(fd[1]);
        read(fd[0],line,MAXLINE);
        LOG("fast","upload","%s",line);
        wait(NULL);
    }

    return 0;
}
