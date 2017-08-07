#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 16
#define OVER (-1)
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
void put(struct prodcons *p,int num)
{
    //给互斥变量加锁
    pthread_mutex_lock(&p->lock);
    //缓冲区已满等待
    if((p->writepos+1)%BUFFER_SIZE == p->readpos)
    {
        pthread_cond_wait(&p->notfull,&p->lock);
    }
    p->buf[p->writepos] = num;
    p->writepos++;
    if(p->writepos >= BUFFER_SIZE)
        p->writepos = 0;
    pthread_cond_signal(&p->notempty);
    pthread_mutex_unlock(&p->lock);

}
int get(struct prodcons *b)
{
    int date;
    pthread_mutex_lock(&b->lock);
    //缓冲区为空等待
    if(b->writepos == b->readpos)
    {
        pthread_cond_wait(&b->notempty,&b->lock);
    }
    date = b->buf[b->readpos];
    b->readpos++;
    if(b->readpos >=BUFFER_SIZE)
        b->readpos = 0;
    pthread_cond_signal(&b->notfull);
    pthread_mutex_unlock(&b->lock);
    return date;
}

void * product()
{
    int i;
    for(i = 0; i<100; i++)
    {
        printf("%d------>\n",i);
        put(&buffer,i);
    }
    put(&buffer,OVER);
}

void *consumer()
{
    int num = 0;
    while(1)
    {
        num = get(&buffer);
        if(num == OVER)
            break;
        printf("------->%d\n",num);
    }
}
int main()
{
    pthread_t pthread_id;
    pthread_t pthread_id2;
    int ret;

    init(&buffer);

    ret = pthread_create(&pthread_id, NULL,  (void*)product,NULL);
    if(ret != 0 )
    {
        printf("pthread_create error\n");
        return -1;
    }
    ret = pthread_create(&pthread_id2, NULL,  (void*)consumer,NULL);
    if(ret != 0 )
    {
        printf("pthread_create error\n");
        return -1;
    }
    pthread_join(pthread_id2, NULL);
    pthread_join(pthread_id, NULL);
    return 0;
}
