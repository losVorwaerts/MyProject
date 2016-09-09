#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "make_log.h"
#include "cJSON.h"
#include "redis_op.h"
#include "fcgi_stdio.h"


#define DATA_LOG_MODULE     "cgi"
#define DATA_LOG_PROC       "data"

#define FILE_NAME_LEN       (256)
#define HOST_NAME_LEN       (30)
#define PIC_URL_LEN         (256)
#define PIC_NAME_LEN        (10)

#define HOST_IP             "http://192.168.21.200"

char g_host_name[HOST_NAME_LEN];


void increase_file_pv(char *file_id)
{
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd("127.0.0.1", "6379");
    if (redis_conn == NULL) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        return;
    }


    rop_zset_increment(redis_conn, FILE_HOT_ZSET, file_id);


    rop_disconnect(redis_conn);
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
/*
   void cgi_init(void)
   {
   int fd = 0;
   struct stat st_buf;
   int len = 0;

   fd = open("./conf/HOST_NAME", O_RDONLY);
   if (fd < 0) {
//LOG(UTIL_LOG_MODULE, UTIL_LOG_PROC, "open ./conf/HOSTNAME error");
return ;
}

fstat(fd, &st_buf);
len = st_buf.st_size;

read(fd, g_host_name, len);
trim_space(g_host_name);
close(fd);

}
*/

/* -------------------------------------------*/
/**
 * @brief  得到本行的指定字段数据 字段号从1开始
 *
 * @param line_head        本行首地址
 * @param col            字段号 
 * @param value            得到的数据
 * @param max_len        数据最大长度
 * @param with_quote    该字段是否有“”包括 1 是 其他否
 *
 * @returns   
 *        得到的数据
 */
/* -------------------------------------------*/
char* get_value_by_col(char *line_head, int col, char *value, int max_len, char with_quote)
{
    char *pos = NULL;        
    char *value_head = NULL;
    int tmp_col = 0;
    int value_len = 0;


    if (col < 1) {
        fprintf(stderr, "col must >= 1\n");
        goto END;
    }

    /* 预留 \0 空间 */
    --max_len;

    /* 第一个字段 */
    if (col == 1) {
        for (pos = line_head, value_head = line_head; *pos != '|' && *(pos-1) != '\\'; ++pos);
        if (with_quote == 1) {
            value_len = ((pos - line_head - 2) > max_len) ? max_len: (pos-line_head-2);
            memcpy(value, line_head+1, value_len);    
        }
        else {
            value_len = ((pos - line_head) > max_len) ? max_len: (pos-line_head);
            memcpy(value, line_head, value_len);    
        }
        value[value_len] = '\0';
        goto END;
    }

    /* 二到最后字段 */
    for (pos = line_head, value_head = line_head; ; ++pos) {
        if ((*pos == '|' && *(pos-1) != '\\') || *pos == '\n') {
            ++tmp_col;
            if (tmp_col == col) {
                if (with_quote == 1) {
                    value_len = ((pos-value_head-2)>max_len) ? max_len: (pos-value_head-2);
                    memcpy(value, value_head+1, value_len);
                }
                else {
                    value_len = ((pos-value_head)>max_len) ? max_len: (pos-value_head);
                    memcpy(value, value_head, value_len);
                }
                value[value_len] = '\0';
                goto END;
            }
            value_head = pos+1;
            if (*pos == '\n') {
                break;
            }
        }
    }

    /* col 超出字段范围 */
    fprintf(stderr, "expend colum !\n");
    value = NULL;

END:
    return value;
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
void print_file_list_json(int fromId, int count, char *cmd, char *kind)
{


    int i = 0;


    cJSON *root = NULL; 
    cJSON *array =NULL;
    char *out;
    char filename[FILE_NAME_LEN] = {0};
    char create_time[FIELD_ID_SIZE] ={0};
    char picurl[PIC_URL_LEN] = {0};
    char suffix[8] = {0};
    char pic_name[PIC_NAME_LEN] = {0};
    char file_url[FILE_NAME_LEN] = {0};
    char file_id[FILE_NAME_LEN] = {0};
    char user[128] = {0};
    int retn = 0;
    int endId = fromId + count - 1;
    int score = 0;

    RVALUES file_list_values = NULL;
    int value_num;
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd("127.0.0.1", "6379");
    if (redis_conn == NULL) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        return;
    }

    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "fromId:%d, count:%d",fromId, count);
    file_list_values = malloc(count*VALUES_ID_SIZE);

    retn = rop_range_list(redis_conn, FILE_INFO_LIST, fromId, endId, file_list_values, &value_num);
    if (retn < 0) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis range list error");
        rop_disconnect(redis_conn);
        return;
    }
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "value_num=%d\n", value_num);


    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    for (i = 0;i < value_num;i ++) {
        //array[i]:
        cJSON* item = cJSON_CreateObject();

        //id
        //cJSON_AddNumberToObject(item, "id", i);
        get_value_by_col(file_list_values[i], 1, file_id, VALUES_ID_SIZE-1, 0);
        cJSON_AddStringToObject(item, "id", file_id);

        //kind
        cJSON_AddNumberToObject(item, "kind", 2);

        //title_m(filename)
        get_value_by_col(file_list_values[i], 3, filename, VALUES_ID_SIZE-1, 0);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "filename=%s\n", filename);

        cJSON_AddStringToObject(item, "title_m", filename);

        //title_s
        get_value_by_col(file_list_values[i], 5, user, VALUES_ID_SIZE-1, 0);
        cJSON_AddStringToObject(item, "title_s", user);

        //time
        get_value_by_col(file_list_values[i], 4, create_time, VALUES_ID_SIZE-1, 0);
        cJSON_AddStringToObject(item, "descrip", create_time);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "create_time=%s\n", create_time);

        //picurl_m
        memset(picurl, 0, PIC_URL_LEN);
        strcat(picurl, g_host_name);
        strcat(picurl, "/static/file_png/");


        get_file_suffix(filename, suffix);
        sprintf(pic_name, "%s.png", suffix);
        strcat(picurl, pic_name);
        cJSON_AddStringToObject(item, "picurl_m", picurl);

        //url
#if GET_URL_DYNAMIC
        get_file_url_dynamic(file_id, file_url);
#else
        get_value_by_col(file_list_values[i], 2, file_url, VALUES_ID_SIZE-1, 0);
#endif
        cJSON_AddStringToObject(item, "url", file_url);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "file_url=%s\n", file_url);

        //pv
        score = rop_zset_get_score(redis_conn, FILE_HOT_ZSET, file_id);
        cJSON_AddNumberToObject(item, "pv", score-1);

        //hot 
        //cJSON_AddNumberToObject(item, "hot", i%2);
        cJSON_AddNumberToObject(item, "hot", 0);


        cJSON_AddItemToArray(array, item);

    }



    cJSON_AddItemToObject(root, "games", array);

    out = cJSON_Print(root);

    LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"%s", out);
    printf("%s\n", out);

    free(file_list_values);
    free(out);

    rop_disconnect(redis_conn);
}

void str_replace(char* strSrc, char* strFind, char* strReplace)
{
    while (*strSrc != '\0')
    {
        if (*strSrc == *strFind)
        {
            if (strncmp(strSrc, strFind, strlen(strFind)) == 0)
            {
                int i = 0;
                char *q = NULL;
                char *p = NULL;
                char *repl = NULL;
                int lastLen = 0;

                i = strlen(strFind);
                q = strSrc+i;
                p = q;//p、q均指向剩余字符串的首地址
                repl = strReplace;

                while (*q++ != '\0')
                    lastLen++;
                char* temp = malloc(lastLen+1); //临时开辟一段内存保存剩下的字符串,防止内存覆盖
                int k = 0;
                for (k = 0; k < lastLen; k++)
                {
                    *(temp+k) = *(p+k);
                }
                *(temp+lastLen) = '\0';
                while (*repl != '\0')
                {
                    *strSrc++ = *repl++;
                }
                p = strSrc;
                char* pTemp = temp;//回收动态开辟内存
                while (*pTemp != '\0')
                {
                    *p++ = *pTemp++;
                }
                free(temp);
                *p = '\0';
            }
            else
                strSrc++;
        }
        else
            strSrc++;
    }
}

int main(int argc, char *argv[])
{
    char    fromId[5];
    char    count[5];
    char    cmd[20];
    char    kind[10];
    char    fileId[FILE_NAME_LEN];


    while(FCGI_Accept() >= 0)
    {
        char    *query = getenv("QUERY_STRING");
        memset(fromId,0,5);
        memset(count,0,5);
        memset(cmd,0,20);
        memset(kind,0,10);
        memset(fileId,0,FILE_NAME_LEN);

        query_parse_key_value(query,"cmd",cmd,NULL);
        if(strcmp(cmd,"newFile") == 0)
        {
            query_parse_key_value(query,"fromId",fromId,NULL);
            query_parse_key_value(query,"count",count,NULL);
            query_parse_key_value(query,"kind",kind,NULL);

            LOG(DATA_LOG_MODULE,DATA_LOG_PROC, "fromId:%s, count:%s, cmd:%s, kind:%s",fromId,count, cmd,kind);

            memset(g_host_name,0,sizeof(g_host_name));
            strcpy(g_host_name,HOST_IP);
            LOG(DATA_LOG_MODULE,DATA_LOG_PROC,g_host_name);

            printf("Content-type: text/html\r\n");
            printf("\r\n");

            print_file_list_json(atoi(fromId), atoi(count), cmd, kind);
        }
        else if(strcmp(cmd, "increase") == 0)
        {
            query_parse_key_value(query,"fileId", fileId, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"fileId:%s,cmd:%s",fileId,cmd);
            str_replace(fileId,"%2F","/");

            increase_file_pv(fileId);

            printf("Content-type: text/html\r\n");
            printf("\r\n");
        }

    }


    return 0;
}
