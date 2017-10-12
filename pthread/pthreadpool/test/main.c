#include "threadpool.h"


#define SERVER_PORT 8880
#define MSG_LEN 2014
#define SERCER_IP "127.0.0.1"

void* work(void* arg)
{
    char *p = (char*) arg;
    printf("threadpool callback fuction : %s.\n", p);
    sleep(1);
}

int main(void)
{
    int ret;

    int serverid;
    struct sockaddr_in client_addr;
    int newserverid;
    int msg_len= 0;
    char sCommBuf[MSG_LEN+1];
    struct threadpool *pool = threadpool_init(10, 20);
    /*
       threadpool_add_job(pool, work, "1");
       threadpool_add_job(pool, work, "2");
       threadpool_add_job(pool, work, "3");
       threadpool_add_job(pool, work, "4");
       threadpool_add_job(pool, work, "5");
       threadpool_add_job(pool, work, "6");
       threadpool_add_job(pool, work, "7");
       threadpool_add_job(pool, work, "8");
       threadpool_add_job(pool, work, "9");
       threadpool_add_job(pool, work, "10");
       threadpool_add_job(pool, work, "11");
       threadpool_add_job(pool, work, "12");
       threadpool_add_job(pool, work, "13");
       threadpool_add_job(pool, work, "14");
       threadpool_add_job(pool, work, "15");
       threadpool_add_job(pool, work, "16");
       threadpool_add_job(pool, work, "17");
       threadpool_add_job(pool, work, "18");
       threadpool_add_job(pool, work, "19");
       threadpool_add_job(pool, work, "20");
       threadpool_add_job(pool, work, "21");
       threadpool_add_job(pool, work, "22");
       threadpool_add_job(pool, work, "23");
       threadpool_add_job(pool, work, "24");
       threadpool_add_job(pool, work, "25");
       threadpool_add_job(pool, work, "26");
       threadpool_add_job(pool, work, "27");
       threadpool_add_job(pool, work, "28");
       threadpool_add_job(pool, work, "29");
       threadpool_add_job(pool, work, "30");
       threadpool_add_job(pool, work, "31");
       threadpool_add_job(pool, work, "32");
       threadpool_add_job(pool, work, "33");
       threadpool_add_job(pool, work, "34");
       threadpool_add_job(pool, work, "35");
       threadpool_add_job(pool, work, "36");
       threadpool_add_job(pool, work, "37");
       threadpool_add_job(pool, work, "38");
       threadpool_add_job(pool, work, "39");
       threadpool_add_job(pool, work, "40");
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
        printf("%s\n",sCommBuf);
        threadpool_add_job(pool, work, sCommBuf);
    }
    sleep(5);
    threadpool_destroy(pool);
    return 0;
}
