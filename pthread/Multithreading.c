#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>

#define BUFFER_SIZE 16
#define OVER (-1)
#define BUF_SIZE 512
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
    if((p->writepos+1)%BUFFER_SIZE == p->readpos)
    {
        pthread_cond_wait(&p->notfull,&p->lock);
    }
    p->buf[p->writepos] = *(void *)message;
    p->writepos++;
    if(p->writepos >= BUFFER_SIZE)
        p->writepos = 0;
    pthread_cond_signal(&p->notempty);
    pthread_mutex_unlock(&p->lock);

}
char * get(struct prodcons *b)
{
    char date[BUF_SIZE];
	memset(date, 0x00, BUF_SIZE);
    pthread_mutex_lock(&b->lock);
    //缓冲区为空等待
    if(b->writepos == b->readpos)
    {
        pthread_cond_wait(&b->notempty,&b->lock);
    }
    *date = b->buf[b->readpos];
    b->readpos++;
    if(b->readpos >=BUFFER_SIZE)
        b->readpos = 0;
    pthread_cond_signal(&b->notfull);
    pthread_mutex_unlock(&b->lock);
    return date;
}

void * product(void * arg)
{ 
    put(&buffer,arg);
}

void *consumer()
{
    char * date ;
	
    while(1)
    {
        date  = get(&buffer);
		
    }
}

int main()
{
    pthread_t pthread_id[num];
    pthread_t pthread_id3;
    pthread_t pthread_id2;
    pthread_t pthread_id4;
    int ret;
    int i;

    msgs  msg;
    
    key_t key;
    int pid;
    msgget(key,IPC_CREATE|0666);
    msgrcv(key,(void *)&msg, BUF_SIZE,0,0);
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
     for(i = 0; i < num; i++)
    {
        pthread_join(pthread_id2[i], NULL);
    }

    return 0;
}
