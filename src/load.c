#ifndef lint
static const char rcsid[] = "$Id: echo.c,v 1.5 1999/07/28 00:29:37 roberts Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "fcgi_stdio.h"
#include "make_log.h"
#include "upload.h"
#include "redis_op.h"


#define LOAD_LOG_MODULE "cgi"
#define LOAD_LOG_PROC   "load"

#define	TEMP_BUF_MAX_LEN		256
#define	HOST_NAME_LEN		30

#define REDIS_SERVER_IP     "127.0.0.1"
#define REDIS_SERVER_PORT   "6379"

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
/**
 * @brief  得到文件后缀字符串 如果非法文件后缀,返回"null"
 */
int get_file_suffix(const char *file_name, char *suffix)
{
    const char *p = file_name;
    int len = 0;
    const char *q=NULL;
    const char *k= NULL;

    if (p == NULL) {
        return -1;
    }

    q = p;

    //asdsd.doc.png
    //             ↑

    while (*q != '\0') {
        q++;
    }

    k = q;
    while (*k != '.' && k != p) {
        k--;
    }

    if (*k == '.') {
        k++;
        len = q - k;

        if (len != 0) {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        }
        else {
            strncpy(suffix, "null", 5);
        }
    }
    else {
        strncpy(suffix, "null", 5);
    }

    return 0;
}     


/**
 * @brief  解析url query 类似 abc=123&bbb=456 字符串
 *          传入一个key,得到相应的value
 * @returns   
 *          0 成功, -1 失败
 */
int query_parse_key_value(const char *query, const char *key, char *value, int *value_len_p)
{
    char *temp = NULL;
    char *end = NULL;
    int value_len =0;


    //找到是否有key
    temp = strstr(query, key);
    if (temp == NULL) {
        //LOG(UTIL_LOG_MODULE, UTIL_LOG_PROC, "Can not find key %s in query\n", key);

        return -1;
    }

    temp+=strlen(key);//=
    temp++;//value


    //get value
    end = temp;

    while ('\0' != *end && '#' != *end && '&' != *end ) {
        end++;
    }

    value_len = end-temp;

    strncpy(value, temp, value_len);
    value[value_len] ='\0';

    if (value_len_p != NULL) {
        *value_len_p = value_len;
    }

    return 0;
}


int make_file_url(char *fileid, char *fdfs_file_url)
{
	int		ret = 0;
	
	char	*p = NULL;
	char	*q = NULL;
	char	*k = NULL;
	
	char fdfs_file_stat_buf[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_host_name[HOST_NAME_LEN] = {0};

    int pfd[2];
    pid_t pid;

    pipe(pfd);
    pid = fork();

    if(pid == 0)
    {
        close(pfd[0]);
        dup2(pfd[1],STDOUT_FILENO);
        LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,"[%s,%d]",fileid,strlen(fileid));

        execl("/usr/bin/fdfs_file_info","fdfs_file_info","/etc/fdfs/client.conf",fileid,NULL);
        LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,"execl err");
    }

    close(pfd[1]);

    read(pfd[0],fdfs_file_stat_buf,TEMP_BUF_MAX_LEN);
    LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,"get file_ip [%s] succ", fdfs_file_stat_buf);

    wait(NULL);

    p = strstr(fdfs_file_stat_buf, "source ip address: ");

    q = p + strlen("source ip address: ");
    k = strstr(q,"\n");
    strncpy(fdfs_file_host_name, q ,k-q);

    fdfs_file_host_name[k-q] = '\0';

    strcat(fdfs_file_url,"http://");
    strcat(fdfs_file_url, fdfs_file_host_name);
    strcat(fdfs_file_url,"/");
    strcat(fdfs_file_url, fileid);

    LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,"[%s]",fdfs_file_url);

    return ret;
}
int get_username(char *user)
{
    char *query_string = getenv("QUERY_STRING");

    query_parse_key_value(query_string, "user", user, NULL);
    if(strlen(user) == 0)
    {
        LOG(LOAD_LOG_MODULE, LOAD_LOG_PROC,"get user has no value!");
        return -1;
    }
    else{
        LOG(LOAD_LOG_MODULE, LOAD_LOG_PROC,"get user = [%s]",user);
    }

    return 0;
}

int store_file_to_redis(char *fileid, char *fdfs_file_url, char *filename, char *user)
{
    int     ret = 0;
    redisContext *redis_conn = NULL;

    char *redis_value_buf = NULL;
    time_t now;
    char create_time[25] = {0};
    char suffix[8];

    redis_conn = rop_connectdb_nopwd(REDIS_SERVER_IP, REDIS_SERVER_PORT);
    if(redis_conn == NULL)
        return ret;

    redis_value_buf = malloc(VALUES_ID_SIZE);
    if(redis_value_buf == NULL)
    {
        LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,"malloc redis value error");
        ret = -1;
        goto END;
    }

    memset(redis_value_buf, 0, VALUES_ID_SIZE);

    strcat(redis_value_buf, fileid);
    strcat(redis_value_buf,"|");

    strcat(redis_value_buf, fdfs_file_url);
    strcat(redis_value_buf,"|");

    strcat(redis_value_buf, filename);
    strcat(redis_value_buf,"|");

    now = time(NULL);
    strftime(create_time, 24,"%Y-%m-%d %H:%M:%S", localtime(&now));
    strcat(redis_value_buf, create_time);
    strcat(redis_value_buf,"|");
    strcat(redis_value_buf,user);
    strcat(redis_value_buf,"|");

    get_file_suffix(filename, suffix);

    strcat(redis_value_buf, suffix);

    rop_list_push(redis_conn,"FILE_INFO_LIST",redis_value_buf);

    rop_zset_increment(redis_conn, "FILE_HOT_ZSET",fileid);

    free(redis_value_buf);

END:
    rop_disconnect(redis_conn);
    return ret;
}

int store_file(char *fileid, char *fdfs_file_url, char *filename, char *user)
{
    return store_file_to_redis(fileid, fdfs_file_url, filename, user);
}


int main ()
{	
	char *buf = NULL;
    char str[1024] = {0};
    int i,ch;
    
    char *boundary = NULL;
    char *filenamestr = NULL;
	char filename[1024]={0};

    char fileid[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_url[512] = {0};
    char user[128] = {0};
	
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
		LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,filename);
        char *start=memstr(buf,len,"\r\n\r\n");
        start+=4;
        
        int newlen =len-(int)(start-buf);
        char *end = memstr(start,newlen,boundary);
        end-=2;
		
		//char	path[
        int fd = open(filename,O_RDWR|O_CREAT|O_TRUNC,0664);
        len =(int) (end-start);

        write(fd,start,len);
        
        if(buf!=NULL)
        {
            free(buf);
        }
        close(fd);
        upload(filename,fileid);
        LOG(LOAD_LOG_MODULE,LOAD_LOG_PROC,fileid);

        make_file_url(fileid, fdfs_file_url);

        unlink(filename);
        if(store_file(fileid, fdfs_file_url, filename, user) < 0)
        {
            goto END;
        }
END:
        memset(filename,0,sizeof(filename));
        memset(fileid, 0, sizeof(fileid));
        memset(fdfs_file_url, 0, sizeof(fdfs_file_url));
        memset(user, 0,sizeof(user));

    } 

    return 0;
}
