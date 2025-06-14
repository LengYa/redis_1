#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

int main() 
{
    unsigned int j, isunix = 0;
    redisContext *c;
    redisReply *reply;
    const char *redis_password = "123456";

    // 定义连接超时时间
    struct timeval timeout = { 1, 500000 }; // 1.5 秒

    // 初始化连接选项
    redisOptions options;
    memset(&options, 0, sizeof(redisOptions));

    // 设置 Redis 服务器的主机名和端口
    const char *hostname = "127.0.0.1";
    const int port = 6379;
    REDIS_OPTIONS_SET_TCP(&options, hostname, port);

    // 设置连接超时和命令超时
    options.connect_timeout = &timeout;
    options.command_timeout = &timeout;

    // 尝试建立连接
    c = redisConnectWithOptions(&options);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can not allocate redis context\n");
        }
        exit(1);
    }

    //认证密码，如果有密码的话，需要先认证一下，没有密码那就注释掉这行代码。
    reply = redisCommand(c, "AUTH %s", redis_password);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("Redis认证失败！\n");
    }
    else
    {
        printf("Redis认证成功！\n");
    }

    // 执行 Redis 命令
    int roleid = 10086;

    // 检查键是否存在
    reply = redisCommand(c, "EXISTS role:%d", roleid);
    if (reply == NULL) {
        printf("Command error: %s\n", c->errstr);
        redisFree(c);
        exit(1);
    }

    if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
        printf("Key does not exist.\n");
        freeReplyObject(reply);
    } else {
        freeReplyObject(reply);

        // 执行 hgetall 命令
        reply = redisCommand(c, "hgetall role:%d", roleid);
        if (reply == NULL) {
            printf("Command error: %s\n", c->errstr);
            redisFree(c);
            exit(1);
        }

        // 处理命令回复
        if (reply->type != REDIS_REPLY_ARRAY) {
            if (reply->type == REDIS_REPLY_NIL) {
                printf("Key does not exist or hash is empty.\n");
            } else {
                printf("Reply error: expected array, got %d\n", reply->type);
            }
        } else {
            // 处理数组回复的逻辑
            printf("Reply: number of elements=%lu\n", reply->elements);
            for (size_t i = 0; i < reply->elements; i += 2) {
                printf("\t%s: %s\n", reply->element[i]->str, reply->element[i + 1]->str);
            }
        }

        // 释放回复对象
        freeReplyObject(reply);
    }

    // 释放连接上下文
    redisFree(c);

    return 0;
}