#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <error.h>
#include <errno.h>
#include <semaphore.h>

#define BUFFER_SIZE 512
#define OVER (-1)
#define NUM 1
#define BUF_SIZE 512
#define MSGKEY 1024
sem_t sem;//信号量 

struct msgs{
    long msgtype;
    char msg_text[BUF_SIZE];	
};
static int num =  2;
struct prodcons
{
    char buf[BUFFER_SIZE];
    pthread_mutex_t lock; /* 互斥体lock 用于对缓冲区的互斥操作 */
    int readpos, writepos; /* 读写指针*/
    pthread_cond_t notempty; /* 缓冲区非空的条件变量 */
    pthread_cond_t notfull; /* 缓冲区未满的条件变量 */

};
struct prodcons buffer;
void init(struct prodcons * b)
{
    pthread_mutex_init(&b->lock,NULL);
    pthread_cond_init(&b->notempty,NULL);
    pthread_cond_init(&b->notfull, NULL);
    b->readpos = 0;
    b->writepos = 0;
}
void put(struct prodcons *p,void * message)
{
	char date[BUF_SIZE];
	memset(date, 0x00, BUF_SIZE);
	
	memcpy(date,(char *)message, sizeof(message));
    //给互斥变量加锁
    pthread_mutex_lock(&p->lock);
    //缓冲区已满等待
    if((p->writepos-NUM) == p->readpos)
    {
        pthread_cond_wait(&p->notfull,&p->lock);
    }
//    p->buf[p->writepos] = (void *)message;
    memcpy(&p->buf[p->writepos] , (char *)message, strlen((char *)message));
    printf("message is put   : %s \n",&p->buf[p->writepos]);
    p->writepos++;
    if(p->writepos >= NUM)
        p->writepos = 0;
    pthread_cond_signal(&p->notempty);
    pthread_mutex_unlock(&p->lock);

}
void * get(struct prodcons *b )
{
	char date[BUF_SIZE];
	memset(date, 0x00, BUF_SIZE);
	memset(date, 0x00, BUF_SIZE);
    pthread_mutex_lock(&b->lock);
    //缓冲区为空等待
    if(b->writepos == b->readpos)
    {
        pthread_cond_wait(&b->notempty,&b->lock);
    }
    strcpy(date, &(b->buf[b->readpos]));
    printf("message is get  : %s \n",(char *)date);
    b->readpos++;
    if(b->readpos >=NUM)
        b->readpos = 0;
    pthread_cond_signal(&b->notfull);
    pthread_mutex_unlock(&b->lock);
}

void * product(void * arg)
{ 
    sem_wait(&sem); 
    while(1)
    {
        printf("message is product  : %s \n",(char *)arg);
        put(&buffer,arg);
        sem_wait(&sem);
    }
}

void *consumer()
{

    while(1)
    {
        get(&buffer);

    }
}

int main()
{
    pthread_t pthread_id[num];
    int ret;
    int i;

    struct msgs  msg;

    key_t key;
    ret = sem_init(&sem, 0, 0);
    if(ret == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }
    int pid;
    init(&buffer);
    for(i = 0; i < num; i++) 
    {

        ret = pthread_create(&pthread_id[i], NULL,  (void*)product,(void *)msg.msg_text);
        if(ret != 0 )
        {
            printf("pthread_create error\n");
            return -1;
        }
    }

    for(i = 0; i < num ;i++)
    {
        ret = pthread_create(&pthread_id[i], NULL,  (void*)consumer,NULL);
        if(ret != 0 )
        {
            printf("pthread_create error\n");
            return -1;
        }
    }
    while(1)
    {
        pid = msgget(MSGKEY,IPC_CREAT | 0666);
        if(msgrcv(pid,(void *)&msg, BUF_SIZE,0,0) < 0)
        {
            printf(" msg failed,errno=%d[%s]\n",errno,strerror(errno)); 
        }
        sem_post(&sem);
    }
    /*
       for(i = 0; i < num; i++)
       {
       pthread_join(pthread_id[i], NULL);
       }
     */

    return 0;
}
