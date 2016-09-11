#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "fcgi_stdio.h"
#include "dao_mysql.h"
#include "cJSON.h"
#include "make_log.h"

#define HOST_NAME_LEN   (30)
#define HOST_IP         "192.168.21.200"

#define REG_LOG_MODULE  "cgi"
#define REG_LOG_PROC    "reg"

//#define MYSQL_USER  "root"
//#define MYSQL_PWD   "123456"
//#define MYSQL_DATABASE "dstorage" 

char g_host_name[HOST_NAME_LEN];

int query_parse_key_value(const char *query, const char *key, char *value, int *value_len_p)
{
    char *temp = NULL;
    char *end = NULL;
    int value_len =0;


    //ÕÒµ½ÊÇ·ñÓÐkey
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
void deal_reg_query()
{
    char *query_string = getenv("QUERY_STRING");
    char user[128];
    char pwd[128];
    char flower_name[128];
    char tel[128];
    char email[128];
    
    char *out;
    char buffer[SQL_MAX_LEN] = {0};

    struct timeval tv;
    struct tm*  ptm;
    char    time_str[128];

    gettimeofday(&tv,NULL);
    ptm = localtime(&tv.tv_sec);
    strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",ptm);

    query_parse_key_value(query_string, "user", user,NULL);
    query_parse_key_value(query_string, "pwd", pwd,NULL);
    query_parse_key_value(query_string, "flower_name",flower_name,NULL);
    query_parse_key_value(query_string, "tel", tel,NULL);
    query_parse_key_value(query_string, "email",email,NULL);

    MYSQL *conn = msql_conn(MYSQL_USER, MYSQL_PWD, MYSQL_DATABASE);
    if(conn == NULL)
    {
        LOG(REG_LOG_MODULE,REG_LOG_PROC,"conn err");
        return;
    }
    sprintf(buffer, "insert into %s (u_name,nicheng,password,phone,createtime,email) value ('%s','%s','%s','%s','%s','%s')","user",user,flower_name,pwd,tel,time_str,email);
    LOG(REG_LOG_MODULE,REG_LOG_PROC,"%s",buffer);

    if(mysql_query(conn,buffer))
    {
        print_error(conn,"²åÈëÊ§°Ü");
        LOG(REG_LOG_MODULE,REG_LOG_PROC,"²åÈëÊ§°Ü");
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"code","000");
    out = cJSON_Print(root);

    printf(out);
    free(out);

}
int main(int argc,char *argv[])
{
    while(FCGI_Accept() >= 0)
    {
        memset(g_host_name,0,HOST_NAME_LEN);
        strcpy(g_host_name,HOST_IP);

        printf("Content-type: text/html\r\n");
        printf("\r\n");
        deal_reg_query();
    }
    return 0;
}
