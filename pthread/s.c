

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
void *thread(void *arg)
{
    int n= 0;
    while (n< 5)
    {
        n++;
        printf("%u run \n",(unsigned int)pthread_self());
    }
}

void main()
{
    pthread_t tid[10];
    int i = 0;
    for (; i < 5; i++)
    {
        pthread_create(&tid[i], NULL, thread, NULL);
    }
    for(i = 0; i < 10; i++)
    {
        pthread_join(tid[i], NULL);
    }
}
