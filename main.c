// SimpleHTTPServer.cpp
// 功能：实现简单的web服务器功能，能同时响应多个浏览器的请求：
//       1、如果该文件存在，则在浏览器上显示该文件；
//       2、如果文件不存在，则返回404-file not found页面
//       3、只支持GET、HEAD方法
// HTTP1.1 与 1.0不同，默认是持续连接的(keep-alive)

#include <Winsock2.h>
#include <stdio.h>
#include <string.h>
_CRTIMP char* __cdecl __MINGW_NOTHROW _getcwd (char*, int);

#define BACKLOG 10
#define DEFAULT_PORT 8080
#define BUF_LENGTH 1024
#define MIN_BUF 128
#define _MAX_PATH MAX_PATH
#define USER_ERROR -1
#define SERVER "Server: weidongxu\r\n"

char recv_buf[BUF_LENGTH];
char method[MIN_BUF];
char url[MIN_BUF];
char path[_MAX_PATH];

void ProcessRequst(SOCKET sAccept);
int file_not_found(SOCKET sAccept);
int file_ok(SOCKET sAccept, long flen);
int send_file(SOCKET sAccept, FILE *resource);
int send_not_found(SOCKET sAccept);

const char *FileNotFoundPage=
"<html>"
"<head><title>404 not found</title><link rel='icon' href='http://oygwu7lhj.bkt.clouddn.com/favicon.ico' type='image/x-ico' /> </head>"
"<body bgcolor=\"#FFFFCC\"><br><br><center>"
"<span style=\"font-size:50px;color:CC9999\">对方给你发送了一个404，并向你扔了一只傻亮</span>"
"<br><br><br>"
"<img src=\"http://oygwu7lhj.bkt.clouddn.com/dog.JPEG\" alt=\"dog\">"
"</center></body>"
"</html>"
;
const char *DefaultPage=
"<html>"
"<head><title>Default</title><link rel='icon' href='http://oygwu7lhj.bkt.clouddn.com/favicon.ico' type='image/x-ico' /> </head>"
"<body bgcolor=\"#70000\"><br><br><br><br><br><br><br>"
"<center><span style=\"font-size:50px;color:A0A0A0;font-family:century\">你是卿亮吗</span><hr></center>"
"<br>"
"<table width=100% height=10%><tr><td><center>"
"<a href=\"yes\"><button type=\"button\" style=\"height:30px;width:100px;\">yes</button></a>"
"&nbsp&nbsp&nbsp&nbsp"
"<a href=\"no\"><button type=\"button\" style=\"height:30px;width:100px;\">no</button></a>"
"</center></td></tr></table>"
"</body>"
"</html>"
;
const char *ConfirmPage=
"<html>"
"<head><title>Default</title><link rel='icon' href='http://oygwu7lhj.bkt.clouddn.com/favicon.ico' type='image/x-ico' /> </head>"
"<body bgcolor=\"#FFFFCC\"><center>"
"<span style=\"font-size:30px;color:000000;font-family:century\">恭喜你发现一只认真学习的傻亮</span>"
"<br>"
"<img src=\"http://oygwu7lhj.bkt.clouddn.com/ql.jpg\" alt=\"ql\">"
"</center></body>"
"</html>"
;

void ProcessRequst(SOCKET sAccept)
{
    int i, j;
    //example:GET /su?wd=ww&action=opensearch&ie=UTF-8 HTTP/1.1\r\n
    //处理接收数据
    i = 0; j = 0;
    // 取出第一个单词，一般为HEAD、GET、POST
    while (!(' ' == recv_buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = recv_buf[j];
        i++; j++;
    }
    method[i] = '\0';   // 结束符，这里也是初学者很容易忽视的地方

    printf("method: %s\n", method);
    // 如果不是GET或HEAD方法，则直接断开本次连接
    // 如果想做的规范些可以返回浏览器一个501未实现的报头和页面
    if (stricmp(method, "GET") && stricmp(method, "HEAD"))//比较，相等返回0
    {
        printf("not get or head method.\n");
        printf("***********************\n\n\n\n");
        return;
    }

    // 提取出第二个单词(url文件路径，空格结束)，并把'/'改为windows下的路径分隔符'\'
    // 这里只考虑静态请求(比如url中出现'?'表示非静态，需要调用CGI脚本，'?'后面的字符串表示参数，多个参数用'+'隔开
    // 例如：www.csr.com/cgi_bin/cgi?arg1+arg2 该方法有时也叫查询，早期常用于搜索)
    i = 0;
    while ((' ' == recv_buf[j]) && (j < sizeof(recv_buf)))
        j++;
    while (!(' ' == recv_buf[j]) && (i < sizeof(recv_buf) - 1) && (j < sizeof(recv_buf)))
    {
        if (recv_buf[j] == '/')
            url[i] = '\\';
        else
            url[i] = recv_buf[j];
        i++; j++;
    }
    url[i] = '\0';
    printf("url: %s\n",url);

    if(1==strlen(url))//默认请求
    {
        printf("send default page\n");
        file_ok(sAccept, strlen(DefaultPage));
        send(sAccept, DefaultPage, strlen(DefaultPage), 0);
        printf("***********************\n\n\n\n");
        return;
    }
    else if(0 == stricmp(url, "\\yes"))
    {
        printf("send confirm page\n");
        file_ok(sAccept, strlen(ConfirmPage));
        send(sAccept, ConfirmPage, strlen(ConfirmPage), 0);
        printf("***********************\n\n\n\n");
        return;
    }

    // 将请求的url路径转换为本地路径
    _getcwd(path,_MAX_PATH);//获取当前工作路径
    strcat(path,url);//将两字符串连接
    printf("path: %s\n",path);


    // 打开本地路径下的文件，网络传输中用r文本方式打开会出错
    FILE *resource = fopen(path,"rb");

    // 没有该文件则发送一个简单的404-file not found的html页面，并断开本次连接
    if(resource==NULL)
    {
        file_not_found(sAccept);
        // 如果method是GET，则发送自定义的file not found页面
        if(0 == stricmp(method, "GET"))
            send_not_found(sAccept);

        printf("file not found.\n");
        printf("***********************\n\n\n\n");
        return;
    }
    /*
    SEEK_SET	档案开头
    SEEK_CUR	文件指针的当前位置
    SEEK_END	文件结尾*
    */

    // 求出文件长度，记得重置文件指针到文件头
    fseek(resource,0,SEEK_END);
    long flen=ftell(resource);//返回当前文件读写位置
    printf("file length: %ld\n", flen);
    fseek(resource,0,SEEK_SET);

    // 发送200 OK HEAD
    file_ok(sAccept, flen);

    // 如果是GET方法则发送请求的资源
    if(0 == stricmp(method, "GET"))
    {
        if(0 == send_file(sAccept, resource))
            printf("file send ok.\n");
        else
            printf("file send fail.\n");
    }

    printf("***********************\n\n\n\n");

    return;
}
// 发送200 ok报头
int file_ok(SOCKET sAccept, long flen)
{
    char send_buf[MIN_BUF];
//  time_t timep;
//  time(&timep);
    sprintf(send_buf, "HTTP/1.1 200 OK\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
//  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
//  send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, SERVER);
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Content-Length: %ld\r\n", flen);
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Content-Type: text/html\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

// 发送404 file_not_found报头
int file_not_found(SOCKET sAccept)
{
    char send_buf[MIN_BUF];
//  time_t timep;
//  time(&timep);
    sprintf(send_buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
//  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
//  send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, SERVER);
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Content-Type: text/html\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

// 发送自定义的file_not_found页面
int send_not_found(SOCKET sAccept)
{
    send(sAccept, FileNotFoundPage, strlen(FileNotFoundPage), 0);
    return 0;
}

// 发送请求的资源
int send_file(SOCKET sAccept, FILE *resource)
{
    char send_buf[BUF_LENGTH];
    while (1)
    {
        memset(send_buf,0,sizeof(send_buf));       //缓存清0
        fgets(send_buf, sizeof(send_buf), resource);
    //  printf("send_buf: %s\n",send_buf);
        if (SOCKET_ERROR == send(sAccept, send_buf, strlen(send_buf), 0))
        {
            printf("send() Failed:%d\n",WSAGetLastError());
            return USER_ERROR;
        }
        if(feof(resource))
        return 0;
    }
}

int main()
{
    WSADATA wsaData;
    SOCKET sListen,sAccept;        //服务器监听套接字，连接套接字
    int serverport=DEFAULT_PORT;   //服务器端口号
    struct sockaddr_in ser,cli;   //服务器地址，客户端地址
    int iLen;
    int iResult;

    printf("-----------------------\n");
    printf("Server waiting\n");
    printf("-----------------------\n");

    //第一步：加载协议栈
    if (WSAStartup(MAKEWORD(2,2),&wsaData) !=0)
    {
        printf("Failed to load Winsock.\n");
        return USER_ERROR;
    }

    //第二步：创建监听套接字，用于监听客户请求
    sListen =socket(AF_INET,SOCK_STREAM,0);
    if (sListen == INVALID_SOCKET)
    {
        printf("socket() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //创建服务器地址：IP+端口号
    ser.sin_family=AF_INET;
    ser.sin_port=htons(serverport);               //服务器端口号
    ser.sin_addr.s_addr=htonl(INADDR_ANY);   //服务器IP地址，默认使用本机IP
    memset(&(ser.sin_zero), 0, 8);
    //第三步：绑定监听套接字和服务器地址
    if (bind(sListen,(LPSOCKADDR)&ser,sizeof(ser))==SOCKET_ERROR)
    {
        printf("blind() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //第五步：通过监听套接字进行监听
    if (listen(sListen,BACKLOG)==SOCKET_ERROR)
    {
        printf("listen() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }
    //第六步：接受客户端的连接请求，返回与该客户建立的连接套接字
    iLen=sizeof(cli);
    while (1)  //循环等待客户的请求
    {
        sAccept=accept(sListen,(struct sockaddr*)&cli,&iLen);
        if (sAccept==INVALID_SOCKET)
        {
            printf("accept() Failed:%d\n",WSAGetLastError());
            break;
        }
        printf( "Client IP：%s \n", inet_ntoa(cli.sin_addr));
        printf( "Client PORT：%d \n", ntohs(cli.sin_port));
        while(1)
        {
            memset(recv_buf,0,sizeof(recv_buf));
            printf("wait to rec...\n");
            if ((iResult=recv(sAccept,recv_buf,sizeof(recv_buf),0))<0)   //接收错误
            {
                printf("recv() Failed:%d\n",WSAGetLastError());
                printf("***********************\n\n\n\n");
                closesocket(sAccept);
                break;
            }
            else if(iResult==0)
            {
                printf("connection has been closed!\n");
                printf("***********************\n\n\n\n");
                printf("waiting for next connection!\n");
                break;
            }
            else
            {
                printf("recv data from client:%s\n",recv_buf); //接收成功，打印请求报文
                ProcessRequst(sAccept);
            }
        }
    }
    closesocket(sListen);
    WSACleanup();
    return 0;
}
