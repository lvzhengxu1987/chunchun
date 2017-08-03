#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Tcp.h"
#define MAX_THREADS 50 		//最大线程数
#define SUCCESS                     0
#define FAIL                       -1

int igSocket;
long igConnindex;
int accept_id;	//通讯池下标  accept_proc用
int read_id;	//req_proc用


char CFGNAME[24];

int cgDebug = 5;
pthread_t check_timeout_thread;
pthread_t req_thread[MAX_THREADS],resp_thread[MAX_THREADS],accept_thread;
pthread_mutex_t connsock_mutex;	//通讯连接池互斥锁
int g_pipe[MAX_THREADS][2];
int g_shutdown_flag;

void *check_timeout(void *);
void *accept_proc(void *);
void *req_proc(void *);
void *resp_proc(void *);

int MsgChange(char *msgbuf);       //请求转应答改报文
int PtTcpCreateServ(int  );  //与客户端建立SOCKET连接,并侦听端口
void  InitStruct();		//初始化通讯连接池
int iAddConnSockList(CONNINFO *slTermbuf);
int iDelConnSockList(CONNINFO *slTermBuf);
int waiteforsock();
int iCheckConnList(CONNINFO *slTermBuf);
ssize_t  readn(int fd, void *vptr, size_t n);
ssize_t tcp_snd_len(int sockfd, const char *buf, size_t len);
void DebugPool();
void PbWbQuit(int sig);

int  Read_socket(int nSocketId, char *spBuf, int nLen);
int  Write_socket(int Socket_id, char *Buf, int Len, int iHead);
int nMMsqSpeakThd( int   nMsgType,
                int   nMsqId,
                int   nSrcMod,
                int   nSrcQue,
                int   nSrcMsgType,
                void* vvpData,
                int   vnDataL );
main(argc, argv)
int argc;
char *argv[];
{
	int i,j;
	int rc;
	pthread_attr_t reqattr[MAX_THREADS];
	pthread_attr_t resqattr[MAX_THREADS];
	pthread_attr_t tmattr;

	char aTimeout[32],aThreadnum[8];
	
	
	    int                 iChildpid, iRet;
    int                 iSockfd, iNewSockfd;
    int                 nPort;
    unsigned long       lCliAddr;
    socklen_t           iAddrLen;
    struct sockaddr_in  sClientAddr;
    register int worker_pointer = 0;
    struct sockaddr_in cli_addr;
		int ilCliaddrlen;
    char ip_buf[256] = { 0 };
    int client_fd=0;
	if (argc <2 )
	{
		printf("usage: name ");
		exit (-1);
	}
 	setbuf(stdout, NULL);
  setbuf(stderr, NULL);
 	memset(CFGNAME,0x00,sizeof(CFGNAME));
 	strcpy(CFGNAME,argv[1]);
 	g_shutdown_flag = 0;
 	if ( (iRet=Init("EBankSSrv", argv[1]))!=0 )
  {
      printf("%s: Init Error[%d].",argv[0], iRet );
      exit(0);
  }
	AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "EBankSSrv Begin Start!");
	clearsig();
	/*建立服务端连接并侦听客户端*/
	igSocket = PtTcpCreateServ(giSrvPort);
	if (igSocket < 0) {
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:建立服务端侦听套接字失败!giSrvPort[%d]",giSrvPort);
		exit(-1);
	}

	signal(SIGTERM,PbWbQuit);
	signal(SIGINT, PbWbQuit);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGCLD, SIG_IGN);
/*
	rc = pthread_attr_init(&attr);
	if ( rc==-1 ){
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:pthread_attr_init error![%d]",errno);
		exit(-1);
	}

	rc = pthread_attr_setstacksize(&attr,384*1024);
	if ( rc == -1 ){
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:pthread_attr_setstacksize error![%d]",errno);
		exit(-1);
	}

	pthread_setconcurrency(51*giThreadNum +2);*/
	/*创建共享锁*/
	
	rc = pthread_mutex_init(&connsock_mutex,NULL);
	if ( rc==-1 ){
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:设置conn_mutex共享锁失败![%d]",errno);
		exit(-1);
	}
	
	rc = pthread_getconcurrency();
	AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"pthread_getconcurrency1[%d]",rc);
	rc = pthread_setconcurrency(2*giThreadNum +2);
	if ( rc != 0 ){
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:pthread_attr_setstacksize error![%d]",errno);
		exit(1);
	}
	rc = pthread_getconcurrency();
	AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"pthread_getconcurrency2[%d]",rc);
	
	/*创建超时检测线程*/
		rc = pthread_attr_init(&tmattr);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:timeout pthread_attr_init error![%d]",errno);
			exit(1);
	  }
	  rc = pthread_attr_setstacksize(&tmattr,128*1024);
		if ( rc == -1 ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:timeout pthread_attr_setstacksize error![%d]",errno);
			exit(1);
		}
		rc = pthread_attr_setdetachstate(&tmattr, PTHREAD_CREATE_JOINABLE);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:timeout pthread_attr_setdetachstate error![%d]",errno);
			exit(1);
	  }
		rc = pthread_create(&check_timeout_thread,&tmattr,check_timeout,NULL);
		if (rc ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:timeout 创建超时检测线程失败[%d]",rc);
			exit(1);
		}
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"创建超时检测线程成功[%d]",check_timeout_thread);
	/*初始化通讯连接池*/
	InitStruct();
	accept_id=0;
	read_id=0;
	igConnindex=0;

	/*创建socket回执线程*/
	for (j=0;j<  giThreadNum * 2 ;j++){
		rc = pthread_attr_init(resqattr+j);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:resp pthread_attr_init error![%d]",errno);
			exit(1);
	  }
	  rc = pthread_attr_setstacksize(resqattr+j,128*1024);
		if ( rc == -1 ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:resp pthread_attr_setstacksize error![%d]",errno);
			exit(1);
		}
		rc = pthread_attr_setdetachstate(resqattr+j, PTHREAD_CREATE_JOINABLE);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:resp pthread_attr_setdetachstate error![%d]",errno);
			exit(1);
	  }
		rc = pthread_create(&resp_thread[j],resqattr+j,resp_proc,NULL);
		if (rc ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:创建交易接收线程失败[%d]",rc);
			exit(1);
		}
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"创建交易接收线程成功[%d]",resp_thread[j]);
	}
	
	for(i=0;i<giThreadNum;i++)
	{
		if(pipe(g_pipe[i])<0)
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"failed to create pipe[%d]",g_pipe[i]);	
			exit(1);
		}
	}
	
	/*创建socket接收处理线程*/
	for (j=0;j<giThreadNum; j++){
		rc = pthread_attr_init(reqattr+j);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:req pthread_attr_init error![%d]",errno);
			exit(1);
	  }
	  rc = pthread_attr_setstacksize(reqattr+j,128*1024);
		if ( rc == -1 ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:req pthread_attr_setstacksize error![%d]",errno);
			exit(1);
		}
		rc = pthread_attr_setdetachstate(reqattr+j, PTHREAD_CREATE_JOINABLE);
		if ( rc==-1 )
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:req pthread_attr_setdetachstate error![%d]",errno);
			exit(1);
	  }
		rc = pthread_create(&req_thread[j],reqattr+j,req_proc,(void *)&g_pipe[j][0]);
		if (rc ){
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:req 创建交易发送线程失败[%d]",rc);
			exit(1);
		}
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"创建交易发送线程成功[%d]",req_thread[j]);
	}
	
	
	ilCliaddrlen = sizeof(struct sockaddr_in);
	while(1)
	{  
     memset((char *)&cli_addr, 0x00, sizeof(struct sockaddr_in));
     if ((client_fd = accept(igSocket, (struct sockaddr *)&cli_addr, &ilCliaddrlen)) > 0) {
             if(write(g_pipe[worker_pointer][1],(char*)&client_fd,4)<0){
                     AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"failed to write pipe");
                     exit(1);
             }
             inet_ntop(AF_INET, &cli_addr.sin_addr, ip_buf, sizeof(ip_buf));
             AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"[CONN]Connection from [%s] client_fd=[%d] ", ip_buf,client_fd);
             worker_pointer++;
             if(worker_pointer == giThreadNum) worker_pointer=0;
     }
     else if(errno == EBADF && g_shutdown_flag){
             break;
     }
     else
		{
				if(0 == g_shutdown_flag)
				{
				  AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"please check ulimit -n");
				  sleep(1);   
				}
    }
	}
	if(client_fd<0 && 0==g_shutdown_flag)
	{
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"Accep failed, try ulimit -n");
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"[ERROR]too many fds open, try ulimit -n");
		g_shutdown_flag = 1;
	}		
	
	//pthread_join(accept_thread,NULL);
	pthread_join(check_timeout_thread,NULL);
	for (j=0;j<giThreadNum ;j++)
		pthread_join(resp_thread[j],NULL);
	for (j=0;j<giThreadNum ;j++)
		pthread_join(req_thread[j],NULL);
		
	/*线程等待*/
	while(1) pause();
}
/**************************************************************
 ** 函数名      :   iAddConnSockList()
 ** 功  能      :   在内部结构中记录CLIENT连接
 ** 作  者      :
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   slTermbuf:  输入：内部结构
 ** 返回值      :   0 SUCCESS, <0 FAIL
***************************************************************/

int iAddConnSockList(CONNINFO *slconn)
{
	int i, j, iTmp;
  pthread_mutex_lock(&connsock_mutex);//add 20140529
	//查找空闲空间
	for (i = (accept_id+1)%giMaxConn, j=0 ;j < giMaxConn; ++i, ++j)
	{
		i= i %giMaxConn;
		if (0 == sConnSock.ci[i].flag)
		{
			accept_id = i;
			break;
		}
	}
	//判断是否找到空间位置
	if ( j == giMaxConn ){
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"WARNING:通讯连接池已满");
		pthread_mutex_unlock(&connsock_mutex);//add 20140529
		return(-1);
	}
	/*put in mutual mode*/
	//pthread_mutex_lock(&connsock_mutex);   del 20140529

	if (igConnindex < 99999)
	{
		igConnindex++;
	}else
	{
		igConnindex = 1;
	}
	sConnSock.ci[i].sockfd = slconn->sockfd;
	sConnSock.ci[i].flag = 1;
	memcpy(sConnSock.ci[i].ipaddr,slconn->ipaddr,sizeof(slconn->ipaddr));

	sConnSock.ci[i].index = igConnindex;
	slconn->index = igConnindex;
	sConnSock.ci[i].starttime = time(NULL);

	sConnSock.total++;
	pthread_mutex_unlock(&connsock_mutex);
	if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"AddSockList:add ok,index[%ld] <--> sockfd[%d]", igConnindex,slconn->sockfd);
	return(0);
}
/**************************************************************
 ** 函数名      :   serverproc()
 ** 功  能      :   SERVER线程处理函数
 ** 作  者      :
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   Conn:       输入：内部结构
 ** 返回值      :   无
***************************************************************/
 void *req_proc(void *args)
 {
 	/*char   alCliaddr[21]; */
 	pthread_t my_pthread_id;
 	struct timeval rwto;
 	struct timeval Btime;
 	int rc, ilRn;
    int     nMsgLen, nLen, iHead, iRet, nRet = -1;
    long    lPid;
    char    * sMsgHead = NULL;
    char    sTransId[5];
    char    * sCommBuf = NULL;
    char    sCommLineBuf[80+1];
		
		int   pipefd;
    int * socketfd = (int *)malloc(sizeof(int));

 	/*线程分离*/
	my_pthread_id = pthread_self();
	/*pthread_detach(pthread_self());*/
	pipefd = *(int * )args;
	CONNINFO * conn = (CONNINFO *)malloc(sizeof(CONNINFO));
	if(!conn)
	{
		AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "req_proc malloc CONNINFO Error!");
		return (void *)1;
	}
	sMsgHead = (char *)malloc(sizeof(char)*100);
  sCommBuf  = (char *)malloc(sizeof(char)*(BUFFER_SIZE + 1));
	
	RouteDef * iRoute  = (RouteDef *)malloc(sizeof(RouteDef));
	if(!iRoute)
	{
		AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "req_proc malloc RouteDef Error!");
		return (void *)1;
	}
/*从客户端接收报文*/
 	for(;;)
 	{
        
 		/*接收SOCKET连接*/
 				    nMsgLen = nLen = -1;
        memset( sMsgHead,     0x00, sizeof(sMsgHead) );
        memset( sCommBuf,     0x00, sizeof(sCommBuf) );
        memset( sCommLineBuf, 0x00, sizeof(sCommLineBuf) );
        /*------------------ 先 读 4 字 节 报 文 长 度 ------------------*/
		if(read(pipefd,socketfd,4)==-1)
		{
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"faild to read pipe");
			exit(1);
		}
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "my_pthread_id[%d] read socket[%d]\n", my_pthread_id, *socketfd );
        nMsgLen = Read_socket(*socketfd, sMsgHead, PKGLEN_LEN);
        if ( nMsgLen == 0 )    /* closed by peer */
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "Warning: read socket[%d], [%s]\n", *socketfd, strerror(errno) );
            close(*socketfd);
            exit(0);
        }
        else if (nMsgLen < 0)
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "my_pthread_id[%d] Error: read socket[%d], [%s]\n", my_pthread_id, *socketfd, strerror(errno) );
            close(*socketfd);
            exit(1);
        }

        nLen = atoi(sMsgHead);
        if ( nLen > BUFFER_SIZE )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,-1, "Error: Len is too long [%d]... we EXIT", nLen);
            close(*socketfd);
            exit(1);
        }
        if ( nLen == 0 )
            continue;

        nMsgLen = Read_socket(*socketfd, sCommBuf, nLen);
        if ( nMsgLen == 0 )    /* closed by peer */
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "Warning: read socket[%d], [%s]\n", *socketfd, strerror(errno) );
            close(*socketfd);
            exit(0);
        }
        else if (nMsgLen < 0)
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "my_pthread_id[%d] Error: read socket[%d],[%s]\n", my_pthread_id,*socketfd, strerror(errno) );
            close(*socketfd);
            exit(1);
        }

        AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,0, "my_pthread_id[%d] From socket[%d] read msghead[%s] msglen[%d]",my_pthread_id, *socketfd, sMsgHead, nMsgLen); 
        AppDebugBuffer(TRACE_LEVEL_ERRMSG,TRACE_INFO, sCommBuf, nMsgLen ); 

        /* 根据报文第一字节判断是否带报文头 */
        if ( giHeadLen == 0 )
        {
            AppTrace(TRACE_LEVEL_DEBUG,TRACE_INFO,0, "No Head.");
            iHead = 0;
        }
        else if ( giHeadLen != 0 && (unsigned char)*sCommBuf != giHeadLen )
        {
            memcpy( sCommLineBuf, sCommBuf, 65 );
            AppTrace(TRACE_LEVEL_ERRMSG, TRACE_INFO, 65, "FIX Msg KeyInf[%s]", sCommLineBuf);
            iHead = 0;
        }
        else
        {
            memcpy( sCommLineBuf, sCommBuf+61, 75 );
            AppTrace(TRACE_LEVEL_ERRMSG, TRACE_INFO, 75, "ISO Msg KeyInf[%s]", sCommLineBuf);
            iHead = giHeadLen;
        }
        if ( nMsgLen <= iHead )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,0, "Error: Len is too short [%d]... we EXIT", nLen);
            close(*socketfd);
            exit(1);
        }
		/*获得客户端主机地址
		sprintf(alCliaddr, "%s", inet_ntoa(cli_addr.sin_addr) );*/
		
		/*strcpy(conn->ipaddr,alCliaddr);*/
		conn->sockfd = *socketfd;
 		
		rwto.tv_sec = giReadTimeOut;
		rwto.tv_usec= 0;
		rc = setsockopt(*socketfd,SOL_SOCKET,SO_RCVTIMEO,&rwto,sizeof(rwto));
		if ( rc < 0)
			 AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"Thread [%d] setsockopt SO_RCVTIMEO error,ilRc=%d",my_pthread_id,rc);

	 	if( sConnSock.total >= (giMaxConn - giMaxConn / 50 ) )    /** TOO MUCH TELLER'S CONNECT **/
    {
        AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,0, "Reach MaxConn = %d my_pthread_id[%d]" , sConnSock.total,my_pthread_id);
        /* 请求转应答 
				shutdown(*socketfd,2);*/
				nLen = Write_socket(*socketfd, sCommBuf, nMsgLen, iHead);/* wrong */
        if( nLen <= 0 )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nLen, "Write socket Error1! write len=[%d]\n", nLen );
            close(*socketfd);
            exit(2);
        }
      	close(*socketfd);
        continue;
    }
    
    AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "current giConn = %d my_pthread_id[%d]" , sConnSock.total ,my_pthread_id);
		/***********超过线程池强求转应答 beg***********************/
    rc = iAddConnSockList(conn);
		if (rc < 0)
		{
			AppTrace(TRACE_LEVEL_ERRMSG, TRACE_INFO, 75, "iAddConnSockList no positions my_pthread_id[%d]" ,my_pthread_id);
			/* 请求转应答 
			shutdown(*socketfd,2);*/
			nLen = Write_socket(*socketfd, sCommBuf, nMsgLen, iHead);/* wrong */
      if( nLen <= 0 )
      {
          AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nLen, "Write socket Error2! write len=[%d]\n", nLen );
          close(*socketfd);
          exit(2);
      }
      close(*socketfd);
			continue;
		}
		
		giaModMsgType[0] = conn->index;

        /* 取得路由, 发送消息到目标服务的消息队列 */
        memset( sTransId, 0, sizeof(sTransId) );
        memset( iRoute,  0, sizeof(RouteDef) );
        memcpy( sTransId, sCommBuf+iHead, sizeof(sTransId)-1 );
        iRoute->trans_id = atoi(sTransId);
        iRoute->rq_flag[0] = RQ_FLAG_REQ;
        iRoute->trans_type[0] = TRANS_TYPE_ALL;
        nRet = GetRoute( &gInRoutes, iRoute );
        if ( nRet != 0 )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet, "GetRoute Error.[%d],[%s],[%s]", iRoute->trans_id, iRoute->rq_flag, iRoute->trans_type );
            exit(1);
        }

        nRet = nMMsqSpeakThd( iRoute->des_mod_que,
                           iRoute->des_msg_type,
                           giaModId[0],
                           giaModQue[0],
                           giaModMsgType[0],
                           sCommBuf+iHead,
                           nMsgLen-iHead );
        if ( nRet == 0 )
        {
            AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "Write to msgque: len=%d", nMsgLen-iHead);
        }
        else
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet, "Write to msgque Error.");
            exit(1);
        }

	}
}
/**************************************************************
 ** 函数名      :   iCheckConnList()
 ** 功  能      :   检查一个连接
 ** 作  者      :
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   slTermBuf:  输入：内部结构
 ** 返回值      :   0 SUCCESS, <0 FAIL
***************************************************************/

int iCheckConnList(CONNINFO *slTermBuf)
{
	int i;

	if(cgDebug>4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"iCheckConnList:index[%d]", slTermBuf->index);


	pthread_mutex_lock(&connsock_mutex); 
	for (i = 0;i < giMaxConn;++i){
		if (sConnSock.ci[i].index == slTermBuf->index)
			break;
	}

	if ( i != giMaxConn){
		slTermBuf->sockfd = sConnSock.ci[i].sockfd;
		if(cgDebug>4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"CheckConnList:Found sock[%d] index[%d]",\
      slTermBuf->sockfd,slTermBuf->index);
    pthread_mutex_unlock(&connsock_mutex);
		return 0;
	}
	else{
		pthread_mutex_unlock(&connsock_mutex);
		if(cgDebug>4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"CheckConnList:No.  sock[%d]",\
                 slTermBuf->sockfd);
		return(-1);
	}
}
/**************************************************************
 ** 函数名:   resp_proc()
 ** 功能:     应答线程处理函数
 ** 作者:
 ** 建立日期:   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   args:       输入：内部结构
 ** 返回值      :   无
***************************************************************/
void *resp_proc(void *args)
{
	int	 ilMsglen;                      /* 报文长度     */
	char	 alMsgbuf[BUFFER_SIZE];        /* 报文         */
	char  head[6];

	
	pthread_t my_pthread_id;
	int	sockfd;
	long llindex;
	struct timeval Etime; /*终止时间*/
	//int waste;
		MSGDEF  *tagMsgOut;
	 	struct sockaddr_in cli_addr;
 	int ilCliaddrlen;
 	int rc,ilRn;
    int     nMsgLen, nLen, iHead, iRet, nRet = -1;
    long    lPid;
    char    sMsgHead[100];
    char    sTransId[5];
    char    sCommBuf[BUFFER_SIZE+1];
    char    sCommLineBuf[80+1];
    
	my_pthread_id = pthread_self();
	/*pthread_detach(pthread_self());*/
  CONNINFO * conn = (CONNINFO *)malloc(sizeof(CONNINFO));
  tagMsgOut = (MSGDEF *)malloc(sizeof(MSGDEF));

	if(!conn)
	{
		AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "req_proc malloc CONNINFO Error!");
		return (void *)1;
	}
	for(;;) {

	 			memset(tagMsgOut, 0x00, sizeof(MSGDEF));
        nMsgLen = msgrcv( giaMsgNum[giaModQue[0]],
                          tagMsgOut,
                          sizeof(MSGDEF)-sizeof(long),
                          0,
                          0 );/* wrong */
        if( nMsgLen < 0 )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "msgrcv Error[%s]", strerror(errno) );
            exit(1);
        }
		    
        AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "received respone message.my_pthread_id=[%d]" ,my_pthread_id);

		/* 检查响应是否为超时应答*/
		conn->index = tagMsgOut->lMsgType;
		if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "conn->index[%d] tagMsgOut->lMsgType[%d]", conn->index, tagMsgOut->lMsgType);	

		ilRn = iCheckConnList(conn);/* wrong */
		if ( ilRn < 0 ) {
			//ilRn = iDelConnSockList(conn);	已经被timeout线程清理
			AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:Response timeout timeout , index[%d]",tagMsgOut->lMsgType);			
			continue;
		}
    /* socket 回执入口 */
		sockfd = conn->sockfd;
		llindex=conn->index;
		
        /* 组装报文头 */
        memset( sCommBuf, 0x00, sizeof(sCommBuf) );
        memset( sMsgHead, 0, sizeof(sMsgHead) );
        nMsgLen = tagMsgOut->nMsgLen + iHead;
        if ( iHead > 0 )
        {
            cmBuildHdr( sMsgHead, nMsgLen);
            memcpy( sCommBuf, sMsgHead, iHead );
        }

        memcpy( &sCommBuf[iHead], tagMsgOut->sMsgBuf, tagMsgOut->nMsgLen );
        nLen = Write_socket(sockfd, sCommBuf, nMsgLen, iHead);/* wrong */
        if( nLen <= 0 )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nLen, "Write socket Error! write len=[%d]\n", nLen );
            close(sockfd);
            exit(2);
        }
        AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,0, "write [%d] bytes through socket to EBankSys", nLen);
    conn->sockfd = sockfd;
		conn->index = llindex;
		
		rc = iDelConnSockList(conn);/* wrong */
		gettimeofday(&Etime,NULL);
		if(cgDebug>=1) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"发送返回成功 sock[%d].",sockfd);
	}
}

/**************************************************************
 ** 函数名      :   check_timeout()
 ** 功  能      :   超时检查线程处理函数
 ** 作  者      :
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   args:       输入：内部结构
 ** 返回值      :   无
***************************************************************/
void * check_timeout(void *args)
{
	int i;
	time_t curtime,newcurtime;

	while(1){ /*begin while loop*/
		if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"超时检测开始，当前总连接数：[%d]",sConnSock.total);
		if(sConnSock.total==0)
		{
			sleep(giRcvTimeOut);
			continue;	//为空则不用检测
		}
		curtime = time(NULL);
		/*put in mutual mode*/
		pthread_mutex_lock(&connsock_mutex);
		for ( i=0;i<giMaxConn;++i){/*begin for loop*/
			if ( sConnSock.ci[i].sockfd != 0 && sConnSock.ci[i].flag!=0 ) {
				if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"i=[%d],sock=[%d],index=[%d],start=[%ld]-[%ld],flag[%d]",\
				    i,sConnSock.ci[i].sockfd,sConnSock.ci[i].index,sConnSock.ci[i].starttime,curtime,sConnSock.ci[i].flag);
			}
			if ( (sConnSock.ci[i].sockfd != 0) && (sConnSock.ci[i].flag!=0) && (curtime-sConnSock.ci[i].starttime) >  giRcvTimeOut) { /*begin if cond*/
	    if (cgDebug >= 3)AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"检测到超时连接：i=[%d],sock=[%d],end-start=[%ld] > [%ld], timeout Close!!!flag[%d] ", \
	    	i,sConnSock.ci[i].sockfd,curtime-sConnSock.ci[i].starttime,giRcvTimeOut,sConnSock.ci[i].flag);
				sConnSock.ci[i].flag=0;
				close(sConnSock.ci[i].sockfd);
				--sConnSock.total;
				sConnSock.ci[i].sockfd=0;
				if(cgDebug>=1) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"检测到超时连接：i=[%d],sock=[%d],start=[%ld], cliendip:[%s] timeout Close!!!flag[%d]",	\
	            i,sConnSock.ci[i].sockfd,sConnSock.ci[i].starttime, sConnSock.ci[i].ipaddr, sConnSock.ci[i].flag);
			} /* end if cond */
		}/* end for loop */

		/*leave mutex mode*/
		pthread_mutex_unlock(&connsock_mutex);
		if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"超时检测结束，当前总连接数：[%d]",sConnSock.total);
		DebugPool();
		newcurtime = time(NULL);
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"超时检测结束，检查时间 [%ld] ,超时进程sleep时间：[%d]",newcurtime -  curtime,giRcvTimeOut - ( newcurtime -  curtime ));
		sleep(giRcvTimeOut - ( newcurtime -  curtime ) );
	} /* end while loop */
}

/****************************************************************/
/* 函数编号    ：PtTcpCreateServ                                */
/* 函数名称    ：绑定通讯端口                                   */
/* 作    者    : PC                                            */
/* 建立日期    ：2005-01-10                                     */
/* 最后修改日期：                                               */
/* 函数用途    : 与客户端建立SOCKET连接,并侦听端口              */
/* 函数返回值  : 成功   0                                       */
/*               失败   1                                       */
/****************************************************************/
int PtTcpCreateServ(int ilPort )
{
  int sockid;
  struct sockaddr_in serv_addr;
  int opt;
/**
  struct linger Linger;
**/

  memset((char *)&serv_addr,0x00,sizeof(struct sockaddr_in));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(ilPort);
  sockid=socket(AF_INET,SOCK_STREAM,0);
  if (sockid < 0) {
    AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "PtTcpconnect创建SOCKET错误!!--Retcode =%d",sockid);
    return(-1);
  }
  AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "PtTcpconnect创建SOCKET成功!!");
  /* Set socket reuse address option */
  opt = 1;
  if (setsockopt(sockid,SOL_SOCKET,SO_REUSEADDR,(char *) &opt,sizeof(opt))<0) {
      close(sockid);
      return(-2);
  }
  /* set linger */
  /****
  Linger.l_onoff = 1;
  Linger.l_linger = 0;
  if (setsockopt(sockid,SOL_SOCKET,SO_LINGER, \
		(char *)&Linger,sizeof(Linger)) != 0)
    AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "setsockopt() set linger Error[%d]", errno);
  ****/

  if (bind(sockid,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in)) < 0)
  {
    AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "PtTcpconnect绑定SOCKET错误[%d]!!", errno);
    close(sockid);
    return(-1);
  }

  if (listen(sockid,SOMAXCONN) < 0) 
  {
    AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "PtTcpconnect侦听连接错误[%d]!!", errno);
    close(sockid);
    return(-1);
  }
  if(cgDebug>=1) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "**************listen [%d]*****SOMAXCONN=[%d]*...",ilPort,SOMAXCONN);
  return(sockid);
}

/**************************************************************
 ** 函数名      :   InitStruct()
 ** 功  能      :   初始化内部结构
 ** 作  者      :   
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   无
 ** 返回值      :   无
***************************************************************/

void  InitStruct()
{
	int i;
	for(i=0;i<SYS_MAX_SYNCONNCOUNT;i++){
		memset((char *)&sConnSock.ci[i],0,sizeof(CONNINFO));
	}
	sConnSock.total=0;
}

/* Read "n" bytes from a descriptor. */
ssize_t  readn(int fd, void *vptr, size_t n)  
{
  size_t  nleft;
  ssize_t nread;
  char  *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ( (nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;    /* and call read() again */
      else
        return(-1);
    } else if (nread == 0)
      break;        /* EOF */

    nleft -= nread;
    ptr   += nread;
  }
  return(n - nleft);    /* return >= 0 */
}
//通用发送函数
ssize_t tcp_snd_len(int sockfd, const char *buf, size_t len)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = buf;
    nleft = len;

    while (nleft > 0) {
      if ((nwritten = write(sockfd, ptr, nleft)) <= 0) {
        if (errno == EINTR)
          nwritten = 0;           /* and call write() again */
        else
          return (-1);            /* error */
      }

      nleft -= nwritten;
      ptr += nwritten;
    }
    return nwritten;
}

/**************************************************************
 ** 函数名:   iDelConnSockList()
 ** 功能:     从连接池中删除指定连接
 ** 作者:
 ** 建立日期:   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量:
 ** 参数含义    :   slTermBuf:  输入：内部结构
 ** 返回值      :   0 SUCCESS, <0 FAIL
***************************************************************/

int iDelConnSockList(CONNINFO *slTermBuf)
{
	int i;
  int sockfd;
	/*put in mutual mode*/
	errno=0;
	if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"DelConnSockList index[%d] sockfd[%d]",\
         slTermBuf->index,slTermBuf->sockfd);
	pthread_mutex_lock(&connsock_mutex);
	for (i = 0;i < giMaxConn;++i){
		if (sConnSock.ci[i].index == slTermBuf->index)
			break;
	}
	if (( i != giMaxConn)&&(sConnSock.ci[i].sockfd!=0)){
      sockfd=sConnSock.ci[i].sockfd;
      /*shutdown(sockfd,2);*/
      close(sConnSock.ci[i].sockfd);
      memset(&sConnSock.ci[i],0x00,sizeof(CONNINFO));
      --sConnSock.total;
      if ( sConnSock.total < 0) {
         AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:SYSTEM error,sConnSock.total=[%d]",sConnSock.total);
         sConnSock.total = 0;
         pthread_mutex_unlock(&connsock_mutex);
         return(-1);
      }
      		
			sConnSock.ci[i].flag = 0;
			sConnSock.ci[i].sockfd = 0;
			sConnSock.ci[i].index = 0;
	}
	else
		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"Error index[%d] sockfd[%d],i=[%d]",\
         slTermBuf->index,slTermBuf->sockfd,i);
	/* put out mutual mode*/
	pthread_mutex_unlock(&connsock_mutex);
	return (0);
}

/**************************************************************
 ** 函数名      :   DebugPool()
 ** 功  能      :   打印当前连接池信息
 ** 作  者      :
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :  
 ** 返回值      : 
***************************************************************/
void DebugPool()
{
	int i;
	int iFree,iBusy;
	iFree=0;
	iBusy=0;
	pthread_mutex_lock(&connsock_mutex);
	for (i=0;i<giMaxConn;i++)
	{
		if (sConnSock.ci[i].flag==0) iFree++;
		else 
		{
			iBusy++;
		}
	}
	pthread_mutex_unlock(&connsock_mutex);
	if(cgDebug>=4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"当前连接池空闲[%d] 已使用[%d]",iFree,iBusy);	
	return;
}	

/**************************************************************
 ** 函数名      :   PbWbQuit()
 ** 功  能      :   进程停止时的处理函数
 ** 作  者      :   YZ
 ** 建立日期    :   2009/09
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :   无
 ** 返回值      :   无
***************************************************************/
void PbWbQuit(int sig)
{
	g_shutdown_flag=1;
	AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"ERROR:Exit.......main");
	exit(0);	
}

/***************
    配置参数读取：
  读取配置参数,如：
	////#1  GET CFGNAME  -- BANK
	////#2  get q_respid(arespid)  -- giaMsgNum[giaModQue[0]]
	////#4  get ilPort    --   SERVER_PORT--giSrvPort
	////#6  get consock_nums --MAX_CONN--giMaxConn
	            igTimeOut  --TIMEOUT_READ--giReadTimeOut
	////#7  get iTimeout    --TIMEOUT_RCV--giRcvTimeOut
	////#5  get thread_nums  --THREAD_NUM--giThreadNum
*/
#define  DELSPACE  while(*p == '\n' || *p==' ' || *p=='	')  p++;
#define  NEXTWORD  while(*pt!=' ' && *pt!='	' && *pt!=0x00 ) pt++;

int MsgChange(char msgbuf[])
{
       int iLen=0;
       int i;
       char *p,*p_beg,*p_end,*p_root;
       char *stdnameB="<std400mgid>";
       char *stdnameE="</std400mgid>";
       char *stdroot ="</ROOT>";
       char *msgid="ESR6043";
			char aTmp[BUFFER_SIZE];
      
      memset(aTmp, 0x00, sizeof(aTmp));//20140702
       iLen=strlen(msgbuf);
			if(cgDebug >= 3)AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"recv msg buf=[%s][%d]",msgbuf,iLen);
       if(iLen > BUFFER_SIZE -33 )
       {
       		AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"连接池已满,请求转应答报文超%d", BUFFER_SIZE);
        	return -1;
       }   
       //char aBegin[BUFFER_SIZE];
       //char aEnd[BUFFER_SIZE]; 
       
       //修改stdmsgtype
       p_beg = strstr(msgbuf, "<stdmsgtype>");
       *(p_beg + 12 + 2)='1';
       //修改std400mgid
       //memset(aBegin,0x00,sizeof(aBegin)); 
       //memset(aEnd,0x00,sizeof(aEnd)); 
       p_root=strstr(msgbuf,stdroot);
       if( (p_beg=strstr(msgbuf,stdnameB)) == 0)
       {
       		/* 匹配<std400mgid/>的情况*/
       		p_beg = strstr(msgbuf, "<std400mgid/>");
       		if ( p_beg == 0)
       		{
       		 	if(cgDebug >= 4) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"原报文中没有std400mgid,新增");   
       		 	p_beg = strstr(msgbuf, stdroot);
       		 	if (p_beg != 0)
       		 	{
       			 //strcat(p_beg,"<std400mgid>ESR6043</std400mgid>"); del 20140529
       			 //strcat(p_beg,"<std400mgid>ESR6043</std400mgid></ROOT>");//add 20140529
       			 memcpy(p_beg,"<std400mgid>ESR6043</std400mgid></ROOT>",39);//20140702
       			 if(cgDebug >= 3) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno," 请求转应答报文 res --msgbuf=[%s]",msgbuf);
       			 return iLen + 32;
       			}
       		}
       		p_end = p_beg + 13;
       		//memcpy(aEnd,p_end,(msgbuf+iLen)-p_end);
					//memcpy(aBegin,msgbuf,p_beg-msgbuf);
       		aTmp[0]=0;
       		//sprintf(aTmp, "%s%s%s", aBegin, "<std400mgid>ESR6043</std400mgid>", aEnd);
       		
       		memcpy(aTmp,msgbuf,p_beg-msgbuf);//20140702
					memcpy(aTmp+strlen(aTmp),"<std400mgid>ESR6043</std400mgid>",32);//20140702
					memcpy(aTmp+strlen(aTmp),p_end,(msgbuf+iLen)-p_end);//20140702
					
       		memcpy(msgbuf,aTmp,strlen(aTmp));
					msgbuf[strlen(aTmp)]='\0';
					if(cgDebug >= 3) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno," 请求转应答报文 res --msgbuf=[%s]",msgbuf);
					iLen+=19;		//32-13
					return iLen;
       }else
       {
					p_end=strstr(msgbuf,stdnameE);
					//memcpy(aEnd,p_end,(msgbuf+iLen)-p_end);
					//memcpy(aBegin,msgbuf,p_beg-msgbuf+12);
					aTmp[0]='\0';
					//sprintf(aTmp,"%s%s%s",aBegin,"ESR6043",aEnd);
					
					memcpy(aTmp,msgbuf,p_beg-msgbuf+12);//20140702
					memcpy(aTmp+strlen(aTmp),"ESR6043",7);//20140702
					memcpy(aTmp+strlen(aTmp),p_end,(msgbuf+iLen)-p_end);//20140702
					
					memcpy(msgbuf,aTmp,strlen(aTmp));
					msgbuf[strlen(aTmp)]='\0';
					if(cgDebug >= 3) AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno," 请求转应答报文 res --msgbuf=[%s]",msgbuf);
					iLen+=7;
					return iLen;   				 
			}      
}

/**************************************************************************************************
*  RETURN VALUE:
*    = 0 -- Buffer is NULL
*    > 0 -- successful
*    =-1 -- Error
**************************************************************************************************/
int Write_socket(int nSocketId, char *spBuf, int nLen, int inHead)
{
    int  nNum;
    int  i;
    char sBufhead[PKGLEN_LEN + 1];
    char sPack[BUFFER_SIZE + PKGLEN_LEN + 1];
    char sCommLineRspBuf[80+1];

    if (nLen == 0)
        return(0);

    memset( sBufhead, 0x00, sizeof(sBufhead) );
    memset( sPack,    0x00, sizeof(sPack) );

    ConvCommHeadIToA(nLen, PKGLEN_LEN, sBufhead);

    memcpy(sPack, sBufhead, PKGLEN_LEN);
    memcpy(sPack + PKGLEN_LEN, spBuf, nLen);

    nLen += PKGLEN_LEN;

    AppDebugBuffer(TRACE_LEVEL_NORMAL,TRACE_INFO, (char *)&sPack[0], nLen); 

    memset( sCommLineRspBuf, 0x00, sizeof(sCommLineRspBuf) );

    if( inHead ==0 && giHeadLen != 0 )
    {
        memcpy( sCommLineRspBuf, &sPack[4], 64 );
        AppTrace(TRACE_LEVEL_ERRMSG, TRACE_INFO, 64, "FIX rsp msg KeyInf[%s]", sCommLineRspBuf);
    }
    else
    {
        memcpy( sCommLineRspBuf, &sPack[65], 75 );
        AppTrace(TRACE_LEVEL_ERRMSG, TRACE_INFO, 75, "ISO rsp msg KeyInf[%s]", sCommLineRspBuf);
    }
    errno=0;
    i = 0;
    do
    {
        nNum = write(nSocketId, &sPack[i], nLen - i);
        if (nNum <= 0)
        {
        	  AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nLen-i, "Error errno=[%d],errinf=[%s],nNum=[%d]",errno,strerror(errno),nNum);
        	  if (errno == EINTR)
            	nNum = 0;           /* and call write() again */
            else 
            {
            	return 1;
            }
            
        }
        i += nNum;
    } while (i < nLen);
    return(nLen);
}

/**************************************************************************************************
*  RETURN VALUE:
*    =-1 --  read failed
*    > 0 --  successful
**************************************************************************************************/
int Read_socket(int nSocketId, char *spBuf, int nLen)
{
    int nNum;
    int i = 0;
		errno = 0;
    do
    {
        /* read messages */
        nNum = read(nSocketId, &spBuf[i], nLen - i);
        if (nNum <= 0)
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno, "Read socket [%d] Error.[%s]", nSocketId, strerror(errno) );
            close(nSocketId);
            return(-1);
        }
        i += nNum;
    } while (i < nLen);

    return(i);
}

/**********************************************************************
 *  PROCEDURE: nMMsqSpeakThd
 *  Description:
 *      This is common function of send message to X queue
 *  Input Param:
 *  Output Param:
 *  Return Value:
 *      SUCCESS:  The send is successful.
 *      FAIL:     The send failed.
 *  Notice:
 *  Change History :
 *      Date            Name       Reason
 *      2005/01/31    ShenHui      Initial
 *********************************************************************/
int nMMsqSpeakThd( int   nMsgType,
                int   nMsqId,
                int   nSrcMod,
                int   nSrcQue,
                int   nSrcMsgType,
                void* vvpData,
                int   vnDataL )
{
    int        nRet;
    int        lWriteLen;
    MSGDEF  * tagMsgOut;
		tagMsgOut = (MSGDEF *)malloc(sizeof(MSGDEF));
		if(!tagMsgOut)
		{
			AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO,0, "req_proc malloc MSGDEF Error!");
			return FAIL;
		}

    tagMsgOut->lMsgType = nMsgType;
    tagMsgOut->nSrcMod =  nSrcMod;
    tagMsgOut->nSrcQue =  nSrcQue;
    tagMsgOut->nSrcMsgType = nSrcMsgType;
    tagMsgOut->nMsgLen = vnDataL;
    lWriteLen = vnDataL+sizeof(tagMsgOut->nSrcMod)+sizeof(tagMsgOut->nSrcQue)+sizeof(tagMsgOut->nSrcMsgType)+sizeof(tagMsgOut->nMsgLen);
    memcpy( tagMsgOut->sMsgBuf, vvpData, vnDataL );

    nRet = nMChkMsgLimit(giaMsgNum[nMsqId]);
    if( nRet != 0)
    {
        if ( nRet < 0 ) /* msgctl error, get again */
        {
            if( nRet = CreateMsg(nMsqId) )
            {
                AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet,"msgget giaMsgNum[%d] again Error", nMsqId );
                AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet,"write giaMsgNum[%d] Error", nMsqId);
                return FAIL;
            }
            nRet = nMChkMsgLimit(giaMsgNum[nMsqId]); /* check again */
            if ( nRet != 0 )
            {
                AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO, nRet, "%d Msque over Limit", nMsqId );
                return FAIL;
            }
        }
        else
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO, nRet, "%d Msque over Limit", nMsqId );
            return FAIL;
        }
    }

    nRet = msgsnd( giaMsgNum[nMsqId], tagMsgOut, lWriteLen, 0 );
    if(nRet != -1)
    {
        AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO, 0, "write msqque %d success", giaMsgNum[nMsqId]);
        free(tagMsgOut);
        tagMsgOut=NULL;
        return SUCCESS;
    }
    else
    {
        if( nRet = CreateMsg(nMsqId) )
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet,"msgget giaMsgNum[%d] again Error", nMsqId );
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,nRet,"write giaMsgNum[%d] Error", nMsqId);
            return FAIL;
        }
        nRet = msgsnd( giaMsgNum[nMsqId], tagMsgOut, lWriteLen, 0 );
        if( nRet != -1 )
        {
            AppTrace(TRACE_LEVEL_NORMAL,TRACE_INFO, 0, "write msqque %d success", giaMsgNum[nMsqId]);
            free(tagMsgOut);
        		tagMsgOut=NULL;
            return SUCCESS;
        }
        else
        {
            AppTrace(TRACE_LEVEL_ERRMSG,TRACE_INFO,errno,"write %d Error[%s]", giaMsgNum[nMsqId], strerror(errno) );
            return FAIL;
        }
    }
}