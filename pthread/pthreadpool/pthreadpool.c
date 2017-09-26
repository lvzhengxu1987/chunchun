#include"threadpool.h"

int threadpool_work(struct threadpoo* pool, void * (*callback_function)(void * arg), void * arg)
{
    pthread_mutex_lock(&(pool->mutex));
    /*先判断现在是否满足条件*/
    if((pool->work_cur_num == work_max_num )&& !pool->pool_close || pool->pthread_close)
    {
        /*pool->mutex该参数保证等待时是锁状态*/
        pthread_cont_wait(&(pool->work_not_full),&(pool->mutex));
    }
    if(pool->pool_close || pool->pthread_close)
    {
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }
    struct job *pjob = (struct job*)malloc(sizeof(struct job))
    if(NULL == pjob)
    {
        printf("malloc pjob faile\n");
        ruturn -1;
    }
    pjob->callback_function = callback_function;
    pjob->arg = arg;
    pjob->next = NULL;
    if(pool->head == NULL)
    {
        pool->head = pool->tail = pjob;
    }
    else
    {
        pool->tail->next = pjob;

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
        pool = malloc(sizeof(struct threadpoo));
        if(NULL == poll)
        {
            printf("malloc pthreadpool fail\n");
            break;
        }
        pool->thread_num = thread_num;
        pool->work_max_num = work_max_num;
        pool->work_cur_num = 0;
        pool->pthread_close = 0;
        pool->pool_close = 0;
        if(pthread_mutex_init(&(pool->mutex), NULL) <0)
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
        pool->pthreads = malloc(sizeof(struct threadpoo)*thread_num);
        if(NULL == pool->pthreads)
        {
            printf("maoolc pool->pthreads faile\n");
            break;
        }
        int i;
        for(i = 0; i< thread_num; i++)
        {
            pthread_create(pool->pthreads[i],NULL, threadpool_function, NULL);
        }

    }
    while(0);
    return NULL
}
int threadpool_destroy(struct threadpool *pool)
{

}
void* threadpool_function(void* arg)
{

}

