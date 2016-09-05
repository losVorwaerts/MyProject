#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redis_op.h"
#include "make_log.h"

#define REDIS_MAIN_MODULE    "redis_test"
#define REDIS_MAIN_PROC      "redis"

#define IP      "127.0.0.1"
#define PORT    "6379"

int main(int argc,char *argv[])
{
    int ret = 0;
    redisContext *conn = NULL;

    conn = rop_connectdb_nopwd(IP,PORT);
    if(conn == NULL)
    {
        LOG(REDIS_MAIN_MODULE,REDIS_MAIN_PROC,"conn err");
        ret = -1;
        goto END;
    }

    printf("conn succ\n");

    ret = rop_set_string(conn, "name","zhxp");
    if(ret != 0)
    {
        LOG(REDIS_MAIN_MODULE,REDIS_MAIN_PROC,"set_string err");
        goto END;
    }
    
    char *value = NULL;
    ret = rop_get_string(conn,"name",&value);
    if(ret != 0)
    {
        LOG(REDIS_MAIN_MODULE,REDIS_MAIN_PROC,"get_string err");
        goto END;
    }
    printf("value =%s\n",value);
    if(value != NULL)
    {
        free(value);
    }
END:
    rop_disconnect(conn);
    return ret;
}
