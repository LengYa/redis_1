
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/sds.h>

#include "reactor.h"
#include "adapter_async.h"

static reactor_t *R;

char *rtype[] = {
    "^o^",
    "STRING",
    "ARRAY",
    "INTEGER",
    "NIL",
    "STATUS",
    "ERROR",
    "DOUBLE",
    "BOOL",
    "MAP",
    "SET",
    "ATTR",
    "PUSH",
    "BIGNUM",
    "VERB",
};

void dumpReply(struct redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = (redisReply *)r;
    switch (reply->type)
    {
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_STRING:
        printf("[req = %s]reply:(%s)%s\n", (char *)privdata, rtype[reply->type], reply->str);
        break;
    case REDIS_REPLY_NIL:
        printf("[req = %s]reply:(%s)nil\n", (char *)privdata, rtype[reply->type]);
        break;
    case REDIS_REPLY_INTEGER:
        printf("[req = %s]reply:(%s)%lld\n", (char *)privdata, rtype[reply->type], reply->integer);
        break;
    case REDIS_REPLY_ARRAY:
        printf("[req = %s]reply(%s):number of elements=%lu\n", (char *)privdata, rtype[reply->type], reply->elements);
        for (size_t i = 0; i < reply->elements; i++)
        {
            printf("\t %lu : %s\n", i, reply->element[i]->str);
        }
        break;
    case REDIS_REPLY_ERROR:
        printf("[req = %s]reply(%s):err=%s\n", (char *)privdata, rtype[reply->type], reply->str);
        break;
    default:
        printf("[req = %s]reply(%s)\n", (char *)privdata, rtype[reply->type]);
        break;
    }
}

void authCallback(redisAsyncContext *c, void *r, void *privdata)
{

    redisReply *reply = (redisReply *)r;
    if(reply == NULL){
        printf("Redis认证失败！ reply 为 NULL.\n");
        stop_eventloop(R);
    }else if (reply->type == REDIS_REPLY_ERROR)
    {
        printf("Redis认证失败！\n");
        stop_eventloop(R);
    }else{
        printf("Redis认证成功！\n");
    }
}
 
void connectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("Error: %s\n", c->errstr);
        stop_eventloop(R);
        return;
    }
    printf("Connected...\n");

    // 异步认证密码
    const char *redis_password = "123456";
    redisAsyncCommand((redisAsyncContext *)c, authCallback, NULL, "AUTH %s", redis_password);

    redisAsyncCommand((redisAsyncContext *)c, dumpReply, "hset role:10001", "hset role:10001 name %s age %d sex %s", "mark", 31, "male");

    redisAsyncCommand((redisAsyncContext *)c, dumpReply, "hgetall role:10001","hgetall role:10001");
}

void disconnectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("Error: %s\n", c->errstr);
        stop_eventloop(R);
        return;
    }

    printf("Disconnected...\n");
    stop_eventloop(R);
}

int main(int argc, char **argv)
{
    // 1. 创建 event loop
    R = create_reactor();

    // 2. 连接redis
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err)
    {
        /* Let *c leak for now... */
        redisAsyncFree(c);
        printf("Error: %s\n", c->errstr);
        return 1;
    }
    // 3. 将 redisAsyncContext 附加到 event loop 上，主要是注册回调函数，填写相关事件
    redisAttach(R, c);

    // 4.设置连接成功/断开回调函数
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);

    eventloop(R);

    release_reactor(R);
    return 0;
}