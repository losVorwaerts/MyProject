#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fcgi_stdio.h"
int main(int argc, char *argv[])
{
    int count = 0;
    while (FCGI_Accept() >= 0) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");
        printf("<title>Fast CGI Hello!</title>");
        printf("<h1>Fast CGI Hello!</h1>");
        printf("Request number %d running on host <i>%s</i>\n", ++count, getenv("SERVER_NAME"));
        printf("</br>");
        printf("QUERY_STRING:%s",getenv("QUERY_STRING"));
        printf("</br>");
        printf("REMOTE_ADDR:%s",getenv("REMOTE_ADDR"));
        printf("REMOTE_PORT:%s",getenv("REMOTE_PORT"));
        printf("</br>");
        printf("SCRIPT_FILENAME:%s",getenv("SCRIPT_FILENAME"));
    }
    return 0;
}
