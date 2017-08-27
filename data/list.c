#include<stdio.h>
#include<stdlib.h>

struct grade {
        int score;
            struct grade *next;
};
typedef struct grade NODE;


struct grade * create();
void display(struct grade *);
void myfree(struct grade *);
void insert(struct grade *head, struct grade *p,int dateu);
void deletes(struct grade *head, int date);


int main()
{
    int ret;
    struct grade *head,*pnew;
    head = create();
    if(head == NULL)
    {
        printf("list create head fail\n");
        return -2;
    }
    display(head);
    pnew = (NODE *)malloc(sizeof(NODE));
    if(pnew == NULL)
    {
        printf("pnew malloc fail\n");
        myfree(head);
        return -1;
    }
    pnew->score = 88;
    insert(head, pnew,4);
    display(head);
    deletes(head,3);
    display(head);

    myfree(head);
    return 0;
}
void insert(struct grade *head, struct grade *p,int date)
{
    NODE * q;
    int j;
    q = head;
    for(j = 0; j<date && q!=NULL;j++)
        q = q->next;
     p->next = q->next;
     q->next = p;
}
void deletes(struct grade *head, int date)
{
    NODE *p,*q;
    int i;
    p = head;
    for(i = 0; i< date && p!=NULL ; i++)
        p = p->next;
    q = p->next;
    p->next = q->next;
    free(q);


}
void display(struct grade *head)
{
    struct grade *p;
    for(p = head->next; p !=NULL; p = p->next)
    {
        printf("%d\n",p->score);
    }
}
struct grade * create()
{
    int date;
    struct grade *head,*tail, *pnew;
    head = (NODE *)malloc(sizeof(NODE));
    if(head == NULL )
    {
        printf("fail create\n");
        return NULL;
    }
    head->next = NULL;
    tail = head;
    while(1)
    {
        scanf("%d",&date);
        if(date < 0)
            break;
        pnew = (NODE *)malloc(sizeof(NODE));
        if(pnew == NULL)
        {
            printf("malloc fail\n");
            return NULL;
        }
        pnew->score = date;
        pnew->next = NULL;
        tail->next = pnew;
        tail = pnew;
    }
    return head;

}
void myfree(struct grade * head)
{
    NODE *p,*q;
    p = head;
    while(p->next !=NULL)
    {
        q = p->next;
        p->next = q->next;
        free(q);
    }
    free(head);

}
