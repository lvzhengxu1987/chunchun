/************************************************************************* 
  > File Name: pc.c 
  > Author: gwq 
  > Mail: 457781132@qq.com  
  > Created Time: 2014年11月03日 星期一 17时07分01秒 
 ************************************************************************/  

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/ipc.h>  
#include <sys/sem.h>  
#include <pthread.h>  
#include <unistd.h>  
#include <signal.h>  

#define BUFFER_SIZE 10  
#define SEM_KEY     1234        //信号量Key值  

//定义循环缓冲队列及对其的一组操作  
struct circle_buf {         //循环缓冲队列结构  
    int r;              //读指针  
    int w;              //写指针  
    int buf[BUFFER_SIZE];       //缓冲区  
};  

int semid;              //信号量id  
struct sembuf semaphore;        //定义一个信号量  
struct circle_buf cbuf;  

void writecbuf(struct circle_buf *cbuf, int val)    //向缓冲区写一个值  
{  
    cbuf->buf[cbuf->w] = val;  
    cbuf->w = (cbuf->w + 1) % BUFFER_SIZE;        //写过后指针+1  
}  

int readcbuf(struct circle_buf *pcbuf)          //从当前指针读一个值，返回value  
{  
    int value = pcbuf->buf[pcbuf->r];  
    pcbuf->buf[pcbuf->r] = -1;            //读过后置-1  
    pcbuf->r = (pcbuf->r + 1) % BUFFER_SIZE;  //读过后read+1  

    return value;  
}  

void outcbuf(struct circle_buf *pcbuf)  
{  
    int i = 0;  
    printf("缓冲区各单元的值：");  
    for (i = 0; i < BUFFER_SIZE; ++i) {  
        printf("%d%c", pcbuf->buf[i],  
                (i == BUFFER_SIZE - 1) ? '\n' : ' ');  
    }  
}  

int initsembuf(void)            //创建信号量集，并初始化  
{  
    int sem = 0;  
    //创建3个信号量  
    if ((semid = semget(SEM_KEY, 3, IPC_CREAT | 0666)) >= 0) {  
        sem = 1;  
        //第0个信号量为mutex互斥信号量，初值为1，缓冲区互斥使用(mutex)  
        semctl(semid, 0, SETVAL, sem);  
        sem = BUFFER_SIZE;  
        //第1个信号量为empty同步信号量，初值为10，当前空缓冲区数(empty)  
        semctl(semid, 1, SETVAL, sem);  
        sem = 0;  
        //第2个信号量为full同步信号量，初值为0，当前慢缓冲区数(full)  
        semctl(semid, 2, SETVAL, sem);  
        return 1;  
    } else {  
        return 0;  
    }  
}  

//对信号量的PV操作  
void pmutex(void)  
{  
    semaphore.sem_num = 0;      //信号量索引为0，即第一个信号量  
    semaphore.sem_op = -1;      //减1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void vmutex(void)  
{  
    semaphore.sem_num = 0;      //信号量索引为0，即第一个信号量  
    semaphore.sem_op = 1;       //加1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void pempty(void)  
{  
    semaphore.sem_num = 1;      //信号量索引为1，即第二个信号量  
    semaphore.sem_op = -1;      //减1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void vempty(void)  
{  
    semaphore.sem_num = 1;      //信号量索引为1，即第二个信号量  
    semaphore.sem_op = 1;       //加1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void pfull(void)  
{  
    semaphore.sem_num = 2;      //信号量索引为2，即第三个信号量  
    semaphore.sem_op = -1;      //减1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void vfull(void)  
{  
    semaphore.sem_num = 2;      //信号量索引为2，即第三个信号量  
    semaphore.sem_op = 1;       //加1  
    semaphore.sem_flg = SEM_UNDO;  
    semop(semid, &semaphore, 1);  
}  

void sigend(int sig)  
{  
    semctl(semid, 3, IPC_RMID);  
    exit(0);  
}  

void *productthread(void *arg)  
{  
    int val = *(int *)arg;  
    while (1) {  
        pempty();  
        pmutex();  
        writecbuf(&cbuf, val);  
        printf("生产者%d写入缓冲区的值=%d.\n", val, val);  
        outcbuf(&cbuf);  
        vmutex();  
        vfull();  
        //sleep(1);  
    }  
    return NULL;  
}  

void *consumerthread(void *arg)  
{  
    int cid = *(int *)arg;  
    int val = 0;  
    while (1) {  
        pfull();  
        pmutex();  
        val = readcbuf(&cbuf);  
        printf("消费者%d取走的产品的值=%d.\n", cid, val);  
        outcbuf(&cbuf);  
        vmutex();  
        vempty();  
        //sleep(1);  
    }  
    return NULL;  
}  

int main(int argc, char *argv[])  
{  
    //初始化信号量集  
    while (!initsembuf()) {  
        ;  
    }  
    //收到信号结束程序  
    signal(SIGINT, sigend);  
    signal(SIGTERM, sigend);  
    int i = 0;  
    int ret = 0;  
    //初始化生产者消费者数量  
    int consnum = 0;  
    int prodnum = 0;  

    //初始化循环缓冲队列  
    cbuf.r = 0;  
    cbuf.w = 0;  
    memset(cbuf.buf, 0, BUFFER_SIZE);  

    printf("请输入生产者进程的数目：");  
    scanf("%d", &prodnum);  
    int *prosarg = (int *)malloc(prodnum * sizeof(int));  
    pthread_t *prosid = (pthread_t *)malloc(prodnum * sizeof(pthread_t));  
    printf("请输入消费者进程的数目：");  
    scanf("%d", &consnum);  
    int *consarg = (int *)malloc(consnum * sizeof(int));  
    pthread_t *consid = (pthread_t *)malloc(consnum * sizeof(pthread_t));  

    //启动生产者  
    for (i = 0; i < prodnum; ++i) {  
        prosarg[i] = i + 1;  
        ret = pthread_create(&prosid[i], NULL, productthread,  
                (void *)&prosarg[i]);  
        printf("消费者prosid[%d] = %lu\n", i + 1, prosid[i]);  
        if (ret != 0) {  
            printf("创建生产者线程失败！");  
            exit(EXIT_FAILURE);  
        }  
    }  

    for (i = 0; i < consnum; ++i) {  
        consarg[i] = i + 1;  
        ret = pthread_create(&consid[i], NULL, consumerthread,  
                (void *)&consarg[i]);  
        printf("生产者consid[%d] = %lu\n", i + 1, consid[i]);  
        if (ret != 0) {  
            printf("创建消费者线程失败！");  
            exit(EXIT_FAILURE);  
        }  
    }  

    sleep(10);  

    return 0;  
}
