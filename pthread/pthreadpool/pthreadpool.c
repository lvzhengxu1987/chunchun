#include"pthreadpool.h"
#include <pthread.h>

int threadpool_work(struct threadpool* pool, void * (*callback_function)(void * arg), void * arg)
{
    printf("输入参数为：%s\n",(char *)arg);
    pthread_mutex_lock(&(pool->mutex));
    /*先判断现在是否满足条件*/
    if((pool->work_cur_num == pool->work_max_num)&& !pool->pool_close || pool->pthread_close)
    {
        /*pool->mutex该参数保证等待时是锁状态*/
        pthread_cond_wait(&(pool->work_not_full),&(pool->mutex));
    }
    if(pool->pool_close || pool->pthread_close)
    {
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }
    struct job *pjob = (struct job*)malloc(sizeof(struct job));
    if(NULL == pjob)
    {
        return  -1;
    }
    pjob->callback_function = callback_function;
    pjob->arg = arg;
    printf("message is : %s  \n",(char *)pjob->arg);
    pjob->next = NULL;
    if(pool->head == NULL)
    {
        pool->head = pool->tail = pjob;
        /*通知线程池有任务到来*/
        pthread_cond_broadcast(&(pool->work_not_empty));
    }
    else
    {
        pool->tail->next = pjob;
        pool->tail = pjob;

    }
    pool->work_cur_num ++;
    //printf("已加入线程池\n");
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

        return pool;
    }
    while(0);
    return NULL;
}
int threadpool_destroy(struct threadpool *pool)
{
    printf("run threadpool_destroy \n");
    pthread_mutex_lock(&(pool->mutex));
    printf("pool->pool_close:%d  pool->pthread_clos : %d\n",pool->pool_close,pool->pthread_close);
    if(pool->pool_close || pool->pthread_close)
    {
        printf("已将关闭\n");
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }
    printf("关闭线程标志\n");
    pool->pthread_close = 1;/*关闭标志*/
    while(pool->work_cur_num !=0)
    {
        printf("等待线程为空\n");
        pthread_cond_wait(&(pool->work_not_empty), &(pool->mutex));    
    }
    pool->pool_close = 1;      //置线程池关闭标志
    pthread_mutex_unlock(&(pool->mutex));
    /*一下两行不理解*/
    printf("唤醒线程池中正在阻塞的线程\n");
    pthread_cond_broadcast(&(pool->work_not_empty));
    pthread_cond_broadcast(&(pool->work_not_full));
    int i;
    printf("开始销毁线程\n");
    for(i =0; i < pool->thread_num; i++)
    {
        pthread_join(pool->pthreads[i], NULL);
    }
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->work_empty));
    pthread_cond_destroy(&(pool->work_not_empty));
    pthread_cond_destroy(&(pool->work_not_full));
    free(pool->pthreads);
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
void* threadpool_function(void* ppool)
{
    struct threadpool *pool = (struct threadpool *)ppool;
    struct job *pjob = NULL;
    while(1)
    {
        pthread_mutex_lock(&(pool->mutex));
        while ((pool->work_cur_num == 0) && !pool->pool_close)   //队列为空时，就等待队列非空
        {
            pthread_cond_wait(&(pool->work_not_empty), &(pool->mutex));
        }
        if (pool->pool_close)   //线程池关闭，线程就退出
        {
            pthread_mutex_unlock(&(pool->mutex));
            pthread_exit(NULL);
        }
  //      printf("pool->work_cur_num is :%d\n",pool->work_cur_num);
        if (pool->work_cur_num == 0)
        {
            //printf("pool->head = pool->tail = NULL\n");
            pool->head = pool->tail = NULL;
        }
        /*
        else
        {
            pool->head->next = pjob->next;
        }
        if (pool->work_cur_num == 0)
        {
            printf("销毁线程\n");
            pthread_cond_signal(&(pool->work_empty));        //队列为空，就可以通知threadpool_destroy函数，销毁线程函数
        }
        */
        if (pool->work_cur_num == pool->work_max_num - 1)
        {
            pthread_cond_broadcast(&(pool->work_not_full));  //队列非满，就可以通知threadpool_add_job函数，添加新任务
        }
        pool->work_cur_num--;
        pjob = pool->head;
        pool->head = pjob->next;
        pthread_mutex_unlock(&(pool->mutex));

        printf("pjob->arg : %s",(char *)pjob->arg);
        (*(pjob->callback_function))(pjob->arg);   //线程真正要做的工作，回调函数的调用
        pjob = NULL;
    }
}


int create_socket(char * host_addr, int host_port)
{
    struct sockaddr_in socketaddr_server;
    int ret;
    int socketlistenid;

    memset(&socketaddr_server, 0x00, sizeof(socketaddr_server));

    socketlistenid = socket(AF_INET, SOCK_STREAM, 0);
    if(socketlistenid < 0)
    {
        printf("socket error\n");
        return -1;
    }
    socketaddr_server.sin_family = AF_INET;
    socketaddr_server.sin_port = htons(host_port);
    socketaddr_server.sin_addr.s_addr = htons(INADDR_ANY);
    ret = bind(socketlistenid,(struct sockaddr *)&socketaddr_server, sizeof(socketaddr_server));
    if(ret < 0)
    {
        printf("bind error\n");
        socket_close(socketlistenid);
        return -1;
    }
    ret = listen(socketlistenid, 5);
    if(ret < 0)
    {
        printf("listen error\n");
        socket_close(socketlistenid);
        return -1;
    }
    return socketlistenid;
}

int socket_close(int socketid)
{
    return(close(socketid));
}

int module_init()
{
    
    return 0;
}
