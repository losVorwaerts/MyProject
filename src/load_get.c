/*
 * echo.c --
 *
 *	Produce a page containing all FastCGI inputs
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
#ifndef lint
static const char rcsid[] = "$Id: echo.c,v 1.5 1999/07/28 00:29:37 roberts Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#else
extern char **environ;
#endif

#include "fcgi_stdio.h"
#include "make_log.h"


static void PrintEnv(char *label, char **envp)
{
    printf("%s:<br>\n<pre>\n", label);
    for ( ; *envp != NULL; envp++) {
        printf("%s\n", *envp);
    }
    printf("</pre><p>\n");
}

//find 'substr' from a fixed-length buffer   
//('full_data' will be treated as binary data buffer)  
//return NULL if not found  
char* memstr(char* full_data, int full_data_len, char* substr) 
{ 
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) { 
        return NULL; 
    } 

    if (*substr == '\0') { 
        return NULL; 
    } 

    int sublen = strlen(substr); 

    int i; 
    char* cur = full_data; 
    int last_possible = full_data_len - sublen + 1; 
    for (i = 0; i < last_possible; i++) { 
        if (*cur == *substr) { 
            //assert(full_data_len - i >= sublen);  
            if (memcmp(cur, substr, sublen) == 0) { 
                //found  
                return cur; 
            } 
        } 
        cur++; 
    } 

    return NULL; 
} 

int main ()
{
    char **initialEnv = environ;
    int count = 0;
	
	char *buf = NULL;
    char str[1024] = {0};
    int i,ch;
	
    while (FCGI_Accept() >= 0) {
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;

	printf("Content-type: text/html\r\n"
	    "\r\n"
	    "<title>FastCGI echo</title>"
	    "<h1>FastCGI echo</h1>\n"
            "Request number %d,  Process ID: %d<p>\n", ++count, getpid());

        if (contentLength != NULL) {
            len = strtol(contentLength, NULL, 10);
        }
        else {
            len = 0;
        }

        if (len <= 0) {
	    printf("No data from standard input.<p>\n");
        }
        else {
            			
			buf = malloc(len);
			memset(buf,0,len);
	    	printf("Standard input:<br>\n<pre>\n");
            for (i = 0; i < len; i++) {
                if ((ch = getchar()) < 0) {
                    printf("Error: Not enough bytes received on standard input<p>\n");
                    break;
		}
                //putchar(ch);
                buf[i]=ch;
                printf("%c",buf[i]);
            }
            //printf("%s",buf);
             
            printf("\n</pre><p>\n");
        }
        strcpy(str,getenv("CONTENT_TYPE"));
        LOG("111","222",str);
        char *boundary=strstr(str,"boundary=");
        boundary+=9;
        LOG("111","222",boundary);
        //char *p = strstr(str,"boundary");
        char *q=memstr(buf,len,"filename=");
        LOG("111","222",q);
        char filename[1024]={0};
        i=0;
        q+=10;
        while(*q!='"')
        {
            filename[i] = *q;
            q++;
            i++;
        }
        LOG("111","222",filename);

        char *start=memstr(buf,len,"\r\n\r\n");
        start+=4;
        int newlen =len-(int)(start-buf);
        char *end = memstr(start,newlen,boundary);
        end-=2;

        int fd = open(filename,O_RDWR|O_CREAT|O_TRUNC,0664);
        LOG("111","222","fd=%d",fd);
        len =(int) (end-start);

        write(fd,start,len);
        LOG("111","222",start);
        LOG("111","222","end%s",end);
        //printf("%s",getenv("ONTENT_TYPE"));
        if(buf!=NULL)
        {
            free(buf);
        }
        close(fd);
        PrintEnv("Request environment", environ);
        PrintEnv("Initial environment", initialEnv);
    } /* while */

    return 0;
}
