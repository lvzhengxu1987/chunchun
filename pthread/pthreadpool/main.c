#include "pthreadpool.h"


#define SERVER_PORT 8880
#define MSG_LEN 2014
#define SERCER_IP "127.0.0.1"


void * work(void * arg)
{
    printf("The server received message  is :%s\n",(char *)arg);
    //往下可以延伸往业务模块发送消息，进行业务模块处理
    //接受业务模块发送的消息

}

int main()
{
    int ret;

    int serverid;
    struct sockaddr_in client_addr;
    int newserverid;
    int msg_len= 0;
    char sCommBuf[MSG_LEN+1];


    struct threadpool *pool;

//    ret = module_init();//创建所需队列
    pool = threadpool_init(10,20);
 //   for(num = 0; num < 8; num++)
 //调试用
 /*
        threadpool_work(pool, work, "1");
        threadpool_work(pool, work, "3");
        threadpool_work(pool, work, "4");

        threadpool_work(pool, work, "5");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
        threadpool_work(pool, work, "6");
*/
    serverid = create_socket(SERCER_IP, SERVER_PORT);
    if(serverid < 0)
    {
        printf("create_socket error \n");
        return -2;
    }
    msg_len = sizeof(client_addr);
    while(1)
    {
        memset(sCommBuf, 0x00, sizeof(sCommBuf));
        newserverid = accept(serverid, (struct sockaddr*)&client_addr, &msg_len);
        if(newserverid < 0)
        {
            printf("accept error \n");    
            socket_close(serverid);
            return -1;
        }
        ret = read(newserverid, sCommBuf, MSG_LEN);
        threadpool_work(pool, work, (void *)sCommBuf);
//        memset(sCommBuf, 0x00, sizeof(sCommBuf));
        //memcpy(sCommBuf,"hello server i am client", 24);
       // ret = write(newserverid,sCommBuf,sizeof(sCommBuf));
        socket_close(newserverid);
    }
    socket_close(serverid);
    sleep(4);
    threadpool_destroy(pool);
    return 0;
}
