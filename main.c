// SimpleHTTPServer.cpp
// ���ܣ�ʵ�ּ򵥵�web���������ܣ���ͬʱ��Ӧ��������������
//       1��������ļ����ڣ��������������ʾ���ļ���
//       2������ļ������ڣ��򷵻�404-file not foundҳ��
//       3��ֻ֧��GET��HEAD����
// HTTP1.1 �� 1.0��ͬ��Ĭ���ǳ������ӵ�(keep-alive)

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
"<span style=\"font-size:50px;color:CC9999\">�Է����㷢����һ��404������������һֻɵ��</span>"
"<br><br><br>"
"<img src=\"http://oygwu7lhj.bkt.clouddn.com/dog.JPEG\" alt=\"dog\">"
"</center></body>"
"</html>"
;
const char *DefaultPage=
"<html>"
"<head><title>Default</title><link rel='icon' href='http://oygwu7lhj.bkt.clouddn.com/favicon.ico' type='image/x-ico' /> </head>"
"<body bgcolor=\"#70000\"><br><br><br><br><br><br><br>"
"<center><span style=\"font-size:50px;color:A0A0A0;font-family:century\">����������</span><hr></center>"
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
"<span style=\"font-size:30px;color:000000;font-family:century\">��ϲ�㷢��һֻ����ѧϰ��ɵ��</span>"
"<br>"
"<img src=\"http://oygwu7lhj.bkt.clouddn.com/ql.jpg\" alt=\"ql\">"
"</center></body>"
"</html>"
;

void ProcessRequst(SOCKET sAccept)
{
    int i, j;
    //example:GET /su?wd=ww&action=opensearch&ie=UTF-8 HTTP/1.1\r\n
    //�����������
    i = 0; j = 0;
    // ȡ����һ�����ʣ�һ��ΪHEAD��GET��POST
    while (!(' ' == recv_buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = recv_buf[j];
        i++; j++;
    }
    method[i] = '\0';   // ������������Ҳ�ǳ�ѧ�ߺ����׺��ӵĵط�

    printf("method: %s\n", method);
    // �������GET��HEAD��������ֱ�ӶϿ���������
    // ��������Ĺ淶Щ���Է��������һ��501δʵ�ֵı�ͷ��ҳ��
    if (stricmp(method, "GET") && stricmp(method, "HEAD"))//�Ƚϣ���ȷ���0
    {
        printf("not get or head method.\n");
        printf("***********************\n\n\n\n");
        return;
    }

    // ��ȡ���ڶ�������(url�ļ�·�����ո����)������'/'��Ϊwindows�µ�·���ָ���'\'
    // ����ֻ���Ǿ�̬����(����url�г���'?'��ʾ�Ǿ�̬����Ҫ����CGI�ű���'?'������ַ�����ʾ���������������'+'����
    // ���磺www.csr.com/cgi_bin/cgi?arg1+arg2 �÷�����ʱҲ�в�ѯ�����ڳ���������)
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

    if(1==strlen(url))//Ĭ������
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

    // �������url·��ת��Ϊ����·��
    _getcwd(path,_MAX_PATH);//��ȡ��ǰ����·��
    strcat(path,url);//�����ַ�������
    printf("path: %s\n",path);


    // �򿪱���·���µ��ļ������紫������r�ı���ʽ�򿪻����
    FILE *resource = fopen(path,"rb");

    // û�и��ļ�����һ���򵥵�404-file not found��htmlҳ�棬���Ͽ���������
    if(resource==NULL)
    {
        file_not_found(sAccept);
        // ���method��GET�������Զ����file not foundҳ��
        if(0 == stricmp(method, "GET"))
            send_not_found(sAccept);

        printf("file not found.\n");
        printf("***********************\n\n\n\n");
        return;
    }
    /*
    SEEK_SET	������ͷ
    SEEK_CUR	�ļ�ָ��ĵ�ǰλ��
    SEEK_END	�ļ���β*
    */

    // ����ļ����ȣ��ǵ������ļ�ָ�뵽�ļ�ͷ
    fseek(resource,0,SEEK_END);
    long flen=ftell(resource);//���ص�ǰ�ļ���дλ��
    printf("file length: %ld\n", flen);
    fseek(resource,0,SEEK_SET);

    // ����200 OK HEAD
    file_ok(sAccept, flen);

    // �����GET���������������Դ
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
// ����200 ok��ͷ
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

// ����404 file_not_found��ͷ
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

// �����Զ����file_not_foundҳ��
int send_not_found(SOCKET sAccept)
{
    send(sAccept, FileNotFoundPage, strlen(FileNotFoundPage), 0);
    return 0;
}

// �����������Դ
int send_file(SOCKET sAccept, FILE *resource)
{
    char send_buf[BUF_LENGTH];
    while (1)
    {
        memset(send_buf,0,sizeof(send_buf));       //������0
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
    SOCKET sListen,sAccept;        //�����������׽��֣������׽���
    int serverport=DEFAULT_PORT;   //�������˿ں�
    struct sockaddr_in ser,cli;   //��������ַ���ͻ��˵�ַ
    int iLen;
    int iResult;

    printf("-----------------------\n");
    printf("Server waiting\n");
    printf("-----------------------\n");

    //��һ��������Э��ջ
    if (WSAStartup(MAKEWORD(2,2),&wsaData) !=0)
    {
        printf("Failed to load Winsock.\n");
        return USER_ERROR;
    }

    //�ڶ��������������׽��֣����ڼ����ͻ�����
    sListen =socket(AF_INET,SOCK_STREAM,0);
    if (sListen == INVALID_SOCKET)
    {
        printf("socket() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //������������ַ��IP+�˿ں�
    ser.sin_family=AF_INET;
    ser.sin_port=htons(serverport);               //�������˿ں�
    ser.sin_addr.s_addr=htonl(INADDR_ANY);   //������IP��ַ��Ĭ��ʹ�ñ���IP
    memset(&(ser.sin_zero), 0, 8);
    //���������󶨼����׽��ֺͷ�������ַ
    if (bind(sListen,(LPSOCKADDR)&ser,sizeof(ser))==SOCKET_ERROR)
    {
        printf("blind() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //���岽��ͨ�������׽��ֽ��м���
    if (listen(sListen,BACKLOG)==SOCKET_ERROR)
    {
        printf("listen() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }
    //�����������ܿͻ��˵��������󣬷�����ÿͻ������������׽���
    iLen=sizeof(cli);
    while (1)  //ѭ���ȴ��ͻ�������
    {
        sAccept=accept(sListen,(struct sockaddr*)&cli,&iLen);
        if (sAccept==INVALID_SOCKET)
        {
            printf("accept() Failed:%d\n",WSAGetLastError());
            break;
        }
        printf( "Client IP��%s \n", inet_ntoa(cli.sin_addr));
        printf( "Client PORT��%d \n", ntohs(cli.sin_port));
        while(1)
        {
            memset(recv_buf,0,sizeof(recv_buf));
            printf("wait to rec...\n");
            if ((iResult=recv(sAccept,recv_buf,sizeof(recv_buf),0))<0)   //���մ���
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
                printf("recv data from client:%s\n",recv_buf); //���ճɹ�����ӡ������
                ProcessRequst(sAccept);
            }
        }
    }
    closesocket(sListen);
    WSACleanup();
    return 0;
}
