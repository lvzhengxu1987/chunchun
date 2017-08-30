
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
#define SERVER_PORT   8880 
#define BUFFER_SIZE 1024
#define BUF_SIZE 512
#define MAX_EVENTS 2048

 int  client_conect(char * buf, char * buffer);
 int num = 0;

int main(int argc, char **argv)

{
    int pid;
    int new_server_socket;
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr)); //把一段内存区的内容全部设置为0
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    //创建用于internet的流协议(TCP)socket,用server_socket代表服务器socket

    int server_socket = socket(AF_INET,SOCK_STREAM,0);
    if( server_socket < 0)
    {
        printf("Create Socket Failed!");
        exit(1);
    }

    int opt =1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    //把socket和socket地址结构联系起来
    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        printf("Server Bind Port : %d Failed!", SERVER_PORT);
        exit(1);
    }

    //server_socket用于监听
    if ( listen(server_socket, BUFFER_SIZE) )
    {
        printf("Server Listen Failed!");
        exit(1);
    }
    int epoll_fd;
    epoll_fd = epoll_create(MAX_EVENTS);
    if(epoll_fd == -1)
    {
        printf("epoll_create error\n");
        exit(-1);
    }
    struct epoll_event ev;// epoll事件结构体  
    struct epoll_event events[MAX_EVENTS];// 事件监听队列  
    ev.events=EPOLLIN;  
    ev.data.fd=server_socket;  
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_socket,&ev)==-1)  
    {  
        perror("epll_ctl:server_sockfd register failed");  
        exit(EXIT_FAILURE);  
    }  
    int nfds;// epoll监听事件发生的个数 
    while (1) //服务器端要一直运行
    {
        //等待事件的发生
        nfds=epoll_wait(epoll_fd,events,MAX_EVENTS,-1);  
        if(nfds==-1)  
        {  
            perror("start epoll_wait failed");  
            exit(EXIT_FAILURE);  
        } 
        int i;
        for(i = 0; i < nfds ; i++)
        {
            if(events[i].data.fd==server_socket)
            {
                //定义客户端的socket地址结构client_addr
                struct sockaddr_in client_addr;
                socklen_t length = sizeof(client_addr);
                //接受一个到server_socket代表的socket的一个连接
                //如果没有连接请求,就等待到有连接请求--这是accept函数的特性
                //accept函数返回一个新的socket,这个socket(new_server_socket)用于同连接到的客户的通信
                //new_server_socket代表了服务器和客户端之间的一个通信通道
                //accept函数把连接到的客户端信息填写到客户端的socket地址结构client_addr中
                new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
                if ( new_server_socket < 0)
                {
                    printf("Server Accept Failed!\n");
                    break;
                }
                ev.events=EPOLLIN|EPOLLET;  
                printf("XXXXXXXXXXXX\n");
                ev.data.fd=new_server_socket;  
                if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,new_server_socket,&ev)==-1)  
                {  
                    perror("epoll_ctl:client_sockfd register failed");  
                    exit(EXIT_FAILURE);  
                } 
            }
            else  
            {
                printf("XXXXXXXXXXXX\n");
                num++;
                char buffer[BUFFER_SIZE];
                bzero(buffer, BUFFER_SIZE);
                int length;
                length = recv(new_server_socket,buffer,BUFFER_SIZE,0);
                if (length < 0)
                {
                    printf("Server Recieve Data Failed!\n");
                    break;
                }
                //向B转发消息		
                char buf[255];
                memset(buf , 0x00, sizeof(buf));
                //client_conect(buffer, buf); 

                //发送buffer中的字符串到new_server_socket,实际是给客户端
                memcpy(buf , "My name is C ",14);
                if(send(new_server_socket,buf,12,0)<0)
                {
                    printf("Send File\n");
                    break;
                }
            }
            //关闭与客户端的连接==

            close(new_server_socket);
        }
    }
    close(server_socket);
    return 0;
}
int  client_conect(char * buf, char * buffer)
{
    int client_sockfd;
    int len;
    struct sockaddr_in remote_addr; // 服务器端网络地址结构体     
    memset(&remote_addr,0,sizeof(remote_addr)); // 数据初始化--清零     
    remote_addr.sin_family=AF_INET; // 设置为IP通信     
    remote_addr.sin_addr.s_addr=inet_addr("127.0.0.1");// 服务器IP地址     
    remote_addr.sin_port=htons(8888); // 服务器端口号     
    // 创建客户端套接字--IPv4协议，面向连接通信，TCP协议   
    if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        perror("client socket creation failed");
        exit(EXIT_FAILURE);
    }
    // 将套接字绑定到服务器的网络地址上   
    if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
    {
        perror("connect to server failed");
        exit(EXIT_FAILURE);
    }
    // 循环监听服务器请求      
    send(client_sockfd,buf,BUFFER_SIZE,0);
    // 接收服务器端信息   
    char buf1[255];
    len=recv(client_sockfd,buffer,BUFFER_SIZE,0);
    printf("receive from server:%s\n",buffer);
    if(len<0)
    {
        perror("receive from server failed");
        exit(EXIT_FAILURE);
    }
    close(client_sockfd);// 关闭套接字     
    return 0;

}
