for( ; ; )
{
    nfds = epoll_wait(epfd, events, 20, 500); //等待epoll事件的发生
    for(i = 0; i < nfds; ++i)                //处理所发生的所有事件
    {
        if(events[i].data.fd == listenfd)      //监听事件
        {
            connfd = accept(listenfd, (sockaddr *)&clientaddr, &clilen);
            if(connfd < 0)
            {
                perror("connfd<0");
                exit(1);
            }
            setnonblocking(connfd);          //把客户端的socket设置为非阻塞方式
            char *str = inet_ntoa(clientaddr.sin_addr);
            std::cout << "connect from " << str  <<std::endl;
            ev.data.fd=connfd;                //设置用于读操作的文件描述符
            ev.events=EPOLLIN | EPOLLET;      //设置用于注测的读操作事件
            epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            //注册ev事件
        }
        else if(events[i].events&EPOLLIN)      //读事件
        {
            if ( (sockfd = events[i].data.fd) < 0)
            {
                continue;
            }
            if ( (n = read(sockfd, line, MAXLINE)) < 0) // 这里和IOCP不同
            {
                if (errno == ECONNRESET)
                {
                    close(sockfd);
                    events[i].data.fd = -1;
                }
                else
                {
                    std::cout<<"readline error"<<std::endl;
                }
            }
            else if (n == 0)
            {
                close(sockfd);
                events[i].data.fd = -1;
            }
            ev.data.fd=sockfd;              //设置用于写操作的文件描述符
            ev.events=EPOLLOUT | EPOLLET;  //设置用于注测的写操作事件
            //修改sockfd上要处理的事件为EPOLLOUT
            epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
        }
        else if(events[i].events&EPOLLOUT)//写事件
        {
            sockfd = events[i].data.fd;
            write(sockfd, line, n);
            ev.data.fd = sockfd;              //设置用于读操作的文件描述符
            ev.events = EPOLLIN | EPOLLET;    //设置用于注册的读操作事件
            //修改sockfd上要处理的事件为EPOLIN
            epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
        }
    }
}
