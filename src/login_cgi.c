#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fcgi_stdio.h"
#include "dao_mysql.h"
#include "cJSON.h"
#include "make_log.h"

#define USER_NAME_LEN   (128)
#define PWD_LEN         (256)

#define LOGIN_LOG_MODULE    "cgi"
#define LOGIN_LOG_PROC      "login"

int query_parse_key_value(const char *query,const char *key, char *value,int *value_len_p)
{
    char *temp = NULL;
    char *end = NULL;
    int value_len = 0;

    temp = strstr(query,key);
    if(temp == NULL)
    {
        return -1;
    }
    temp += strlen(key);
    temp++;

    end = temp;
    while('\0' != *end && '#' != *end && '&' != *end)
    {
        end++;
    }

    value_len = end-temp;

    strncpy(value,temp,value_len);
    value[value_len] = '\0';

    if(value_len_p != NULL)
    {
        *value_len_p = value_len;
    }
    return 0;
}
int check_username(char *username, char *password)
{
    char    sql_cmd[SQL_MAX_LEN] = {0};
    int     ret = 0;
    int     i = 0;

    MYSQL *conn = msql_conn(MYSQL_USER,MYSQL_PWD,MYSQL_DATABASE);
    if(conn == NULL)
    {
        LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"mysql conn fail");
        return -1;
    }

    sprintf(sql_cmd,"select password from user where u_name='%s'",username);
    LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"sql 语句:%s",sql_cmd);

    if(mysql_query(conn,sql_cmd)){
        LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"[-]%s error!",sql_cmd);
        ret = -1;
        goto END;
    }
    else{
        MYSQL_RES *result = NULL;
        result = mysql_store_result(conn);
        if(result == NULL)
        {
            LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"mysql_store_result error!");
            ret = -1;
            goto END;
        }
        int num_fields = mysql_num_fields(result);

        MYSQL_ROW row = NULL;
        while((row = mysql_fetch_row(result)))
        {
            for(i=0;i<num_fields; i++)
            {
                if(row[i] != NULL)
                {
                    LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"%d row is %s!",i,row[i]);
                    if(strcmp(password,row[i])==0)
                    {
                        ret = 0;
                        goto END;
                    }
                }
            }
        }

        ret = -1;
    }
END:
    mysql_close(conn);
    return ret;
}
void return_login_status(char *status_num)
{
    char *out;
    cJSON   *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"code",status_num);

    out = cJSON_Print(root);
    printf(out);

    cJSON_Delete(root);
    free(out);
}

int main(int argc,char *argv[])
{
    int     ret = 0;
    char    login_type[10] = {0};
    char    username[USER_NAME_LEN] = {0};
    char    password[PWD_LEN] = {0};

    while(FCGI_Accept() >= 0)
    {
        printf("Content-type: text/html\r\n");
        printf("\r\n");

        memset(login_type,0,sizeof(login_type));
        memset(username,0,USER_NAME_LEN);
        memset(password,0,PWD_LEN);

        char    *query = getenv("QUERY_STRING");

        query_parse_key_value(query,"type",login_type,NULL);
        query_parse_key_value(query,"user",username,NULL);
        query_parse_key_value(query,"pwd",password,NULL);

        LOG(LOGIN_LOG_MODULE,LOGIN_LOG_PROC,"login:[type=%s,user=%s,pwd=%s",login_type,username,password);

        ret = check_username(username, password);
        if(ret == 0)
        {
            return_login_status("000");
        }
        else{
            return_login_status("001");
        }

    }

    return 0;
}
