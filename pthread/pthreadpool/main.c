#include "threadpool.h"

void * work(void * arg)
{
    char *p = (char *)arg;
    printf("The num is :%s\n",p);
}

int main()
{
    struct threadpool *pool;
    pool = threadpool_init(10,20);
    int num = 0 ;
    for(num = 0; num < 40; num++)
    {
        threadpool_work(pool, work, num);
    }
    return 0;
}
