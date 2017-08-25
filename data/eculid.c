#include<stdio.h>
#include <stdio.h>


int fun1(int  , int  );
int main(int argc , char ** argv)
{
    int num1,num2;
    int ret = 0;
    scanf("%d\n",&num1);
    scanf("%d",&num2);
    ret = fun1(num1,num2);
    printf("%d and %d greatest commin divisor is :%d\n",num1,num2,ret);
    return 0;
}

int fun1(int M , int N)
{
    int Rem = 0;
    while(N > 0)
    {
        Rem = M%N;
        M = N;
        N = Rem;
    printf("%d\n",Rem);
    }
    printf("%d\n",Rem);
    return M;
}
