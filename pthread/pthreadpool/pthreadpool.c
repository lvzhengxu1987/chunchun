#include"pthreadpool.h"
#include <pthread.h>

int threadpool_work(struct threadpool* pool, void * (*callback_function)(void * arg), void * arg)
{
    printf("begin malloc\n");
    pthread_mutex_trylock(&(pool->mutex));
    printf("begin malloc111111\n");
    pthread_mutex_lock(&(pool->mutex));
    /*先判断现在是否满足条件*/
    printf("begin malloc111111\n");
    if((pool->work_cur_num == pool->work_max_num)&& !pool->pool_close || pool->pthread_close)
    {
        printf("begin malloc\n");
        /*pool->mutex该参数保证等待时是锁状态*/
        pthread_cond_wait(&(pool->work_not_full),&(pool->mutex));
    }
    if(pool->pool_close || pool->pthread_close)
    {
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }
    printf("begin malloc\n");
    struct job *pjob = (struct job*)malloc(sizeof(struct job));
    if(NULL == pjob)
    {
        printf("malloc pjob faile\n");
        return  -1;
    }
    pjob->callback_function = callback_function;
    pjob->arg = arg;
    pjob->next = NULL;
    if(pool->head == NULL)
    {
        pool->head = pool->tail = pjob;
        /*通知线程池有任务到来*/
        pthread_cond_broadcast(&(pool->work_not_empty));
        printf("等待任务\n");
    }
    else
    {
        pool->head->next = pjob;
        pool->tail = pjob;

    }
    pool->work_cur_num ++;
    pthread_mutex_unlock(&(pool->mutex));
    return 0;

}
struct threadpool * threadpool_init(int thread_num, int work_max_num)
{
    struct threadpool *pool = NULL;
    do
    {
        pool = malloc(sizeof(struct threadpool));
        if(NULL == pool)
        {
            printf("malloc pthreadpool fail\n");
            break;
        }
        pool->thread_num = thread_num;
        pool->work_max_num = work_max_num;
        pool->work_cur_num = 0;
        pool->pthread_close = 0;
        pool->pool_close = 0;
        pool->head = NULL;
        pool->tail = NULL;
        if (pthread_mutex_init(&(pool->mutex), NULL))
        {
            printf("pthread_mutex_init faile\n");
            break;
        }
        if(pthread_cond_init(&(pool->work_empty), NULL))
        {
            printf("pthread_cond_init work_empty faile\n");    
            break;
        }
        if(pthread_cond_init(&(pool->work_not_empty), NULL))
        {
            printf("pthread_cond_initwork_not_empty faile\n");    
            break;
        }
        if(pthread_cond_init(&(pool->work_not_full), NULL))
        {
            printf("pthread_cond_init work_not_fullfaile\n");    
            break;
        }
        pool->pthreads = malloc(sizeof(struct threadpool)*thread_num);
        if(NULL == pool->pthreads)
        {
            printf("maoolc pool->pthreads faile\n");
            break;
        }
        int i;
        for(i = 0; i< thread_num; i++)
        {
            pthread_create(&(pool->pthreads[i]),NULL, threadpool_function, (void *)pool);
        }

    }
    while(0);
    return NULL;
}
int threadpool_destroy(struct threadpool *pool)
{
    pthread_mutex_lock(&(pool->mutex));
    if(pool->pool_close || pool->pthread_close)
    {
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }
    pool->pthread_close = 1;/*关闭标志*/
    while(1)
    {
        pthread_cond_wait(&(pool->work_not_empty), &(pool->mutex));    
    }
    pool->pool_close = 1;      //置线程池关闭标志
    pthread_mutex_unlock(&(pool->mutex));
    /*一下两行不理解*/
    pthread_cond_broadcast(&(pool->work_not_empty));
    pthread_cond_broadcast(&(pool->work_not_full));
    int i;
    for(i =0; i < pool->thread_num; i++)
    {
        pthread_join(pool->pthreads[i], NULL);
    }
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->work_empty));
    pthread_cond_destroy(&(pool->work_not_empty));
    pthread_cond_destroy(&(pool->work_not_full));
    struct job *p;
    while(pool->head!=NULL)
    {
        p = pool->head;
        pool->head = p->next;
        free(p);
    }
    free(pool);
    return 0;

}
void* threadpool_function(void* arg)
{
    struct threadpool * pool = (struct threadpool *)arg;
    struct job *pjob = NULL;
    while(1)
    {
        pthread_mutex_trylock(&(pool->mutex));
        pthread_mutex_lock(&(pool->mutex));
        /*线程池关闭或者也没有需要处理的任务*/
        while((pool->work_cur_num ==0) && !pool->pool_close)
        {
            pthread_cond_wait(&(pool->work_not_empty),&(pool->mutex));
        }
        if(pool->pool_close)
        {
            pthread_mutex_unlock(&(pool->mutex));    
            pthread_exit(NULL);
        }
        pjob = pool->head;
        if(pool->work_cur_num == 0)
        {
            pool->head = pool->tail = NULL;
        }
        else
        {
            pool->head = pjob->next;
        }
        if(pool->work_cur_num == 0)
        {
            /*没有任务需要处理时如何做*/
            pthread_cond_signal(&(pool->work_empty));

        }
        if(pool->work_cur_num == pool->work_max_num - 1)
        {
            /*当前任务达到最大时，如何控制*/
            pthread_cond_broadcast(&(pool->work_not_full));    
        }
        pool->work_cur_num--;
        pthread_mutex_unlock(&(pool->mutex));
        (*(pjob->callback_function))(pjob->arg);
        free(pjob);
        pjob = NULL;
    }

}

