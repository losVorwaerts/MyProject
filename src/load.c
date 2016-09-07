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
	char *buf = NULL;
    char str[1024] = {0};
    int i,ch;
    
    char *boundary = NULL;
    char *filenamestr = NULL;
	char filename[1024]={0};
	
    while (FCGI_Accept() >= 0) {
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;

		printf("Content-type: text/html\r\n"
	    "\r\n");
        if (contentLength != NULL) {
            len = strtol(contentLength, NULL, 10);
        }
        else {
            len = 0;
        }

        if (len <= 0) {
	 
        }
        else {
            			
			buf = malloc(len);
			memset(buf,0,len);
	    	
            for (i = 0; i < len; i++) {
                if ((ch = getchar()) < 0) {
                    break;
		}
                buf[i]=ch;
            }
        }
        strcpy(str,getenv("CONTENT_TYPE"));
  
        boundary=strstr(str,"boundary=");
        boundary+=9;
     
        filenamestr=memstr(buf,len,"filename=");
              
        i=0;
        filenamestr+=10;
        memset(filename,0,sizeof(filename));
        while(*filenamestr!='"')
        {
            filename[i] = *filenamestr;
            filenamestr++;
            i++;
        }
		LOG("LOAD","MAIN",filename);
        char *start=memstr(buf,len,"\r\n\r\n");
        start+=4;
        
        int newlen =len-(int)(start-buf);
        char *end = memstr(start,newlen,boundary);
        end-=2;

        int fd = open(filename,O_RDWR|O_CREAT|O_TRUNC,0664);
        len =(int) (end-start);

        write(fd,start,len);
        if(buf!=NULL)
        {
            free(buf);
        }
        close(fd);
        
    } 

    return 0;
}
