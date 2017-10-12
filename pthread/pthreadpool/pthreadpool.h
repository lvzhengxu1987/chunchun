#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <error.h>
#include <errno.h>
#include <sys/epoll.h>
#include<pthread.h>



struct job{
    void* (*callback_function)(void *arg);    //线程回调函数
    void *arg;                                //回调函数参数
    struct job *next;
};

struct threadpool{
    int thread_num;  //线程的个数
    int work_cur_num;  //线程当前的个数
    int work_max_num;   //线程最大个数
    int pthread_close;   //线程是否已经关闭
    int pool_close;     //线程池是否已经关闭
    pthread_mutex_t mutex;            //互斥信号量
    pthread_t  *pthreads;  //线程池中所有的线程pthread_t
    struct job *head;
    struct job *tail;
    pthread_cond_t work_empty;  //线程为空
    pthread_cond_t work_not_empty; //线程不为空
    pthread_cond_t work_not_full;  //线程为满
};

int threadpool_work(struct threadpool* pool, void * (*callback_function)(void * arg), void * arg);
struct threadpool * threadpool_init(int thread_num, int work_max_num);
int threadpool_destroy(struct threadpool *pool);
void* threadpool_function(void* arg);
int create_socket(char * host_addr, int host_port);
int socket_close(int socketid);
