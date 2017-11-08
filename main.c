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
FILE * FindFile(char* url);
long SendHead(SOCKET sAccept,FILE *resource,char* url);
int send_file(SOCKET sAccept, FILE *resource,long filelen);

void ProcessRequst(SOCKET sAccept)
{
    long filelen;
    FILE *resource;
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
        sprintf(url,"\\default.html");
    }
    if(resource=FindFile(url))//�ҵ��ļ�
    {
        filelen=SendHead(sAccept,resource,url);
    }
    else
    {
        if(resource=FindFile("\\404.html"))
        {
            printf("send not found page\n");
            filelen=SendHead(sAccept,resource,NULL);
        }
        else
        {
            printf("an err occur!\n");
            exit(0);
        }
    }
    // �����GET���������������Դ
    if(0 == stricmp(method, "GET"))
    {
        if(0 == send_file(sAccept, resource,filelen))
            printf("file send ok.\n");
        else
            printf("file send fail.\n");
    }

    printf("***********************\n\n\n\n");

    return;
}
FILE * FindFile(char* url)
{
    // �������url·��ת��Ϊ����·��
    _getcwd(path,_MAX_PATH);//��ȡ��ǰ����·��
    strcat(path,url);//�����ַ�������
    // �򿪱���·���µ��ļ������紫������r�ı���ʽ�򿪻����
    printf("path: %s\n",path);
    FILE *resource = fopen(path,"rb");
    return resource;
}

long SendHead(SOCKET sAccept,FILE *resource,char* url)
{
    char send_buf[MIN_BUF];
    //  time_t timep;
    //  time(&timep);
    if(url)
    sprintf(send_buf, "HTTP/1.1 200 OK\r\n");
    else
    sprintf(send_buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    //  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
    //  send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, SERVER);
    send(sAccept, send_buf, strlen(send_buf), 0);
    fseek(resource,0,SEEK_END);
    long flen=ftell(resource);//���ص�ǰ�ļ���дλ��
    fseek(resource,0,SEEK_SET);
    printf("file length: %ld\n", flen);
    sprintf(send_buf, "Content-Length: %ld\r\n", flen);
    send(sAccept, send_buf, strlen(send_buf), 0);
    if(url)
    {
        char* type=strchr(url,'.');
        type++;
        printf("file type: %s\n", type);
        if(!stricmp(type, "ico"))
        {
            sprintf(send_buf, "Content-Type: image/vnd.microsoft.icon\r\n");//image/vnd.microsoft.icon\r\n");
        }
        else if((!stricmp(type, "JPEG"))||(!stricmp(type, "jpg")))
        {
            sprintf(send_buf, "Content-Type: image/jpeg\r\n");
        }
        else if(!stricmp(type, "html"))
        {
            sprintf(send_buf, "Content-Type: text/html\r\n");
        }
        else
        {
            printf("unknow type!\n");
            sprintf(send_buf, "Content-Type: text/html\r\n");
        }
    }
    else
    {
        printf("html");
        sprintf(send_buf, "Content-Type: text/html\r\n");
    }

    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return flen;
}

int send_file(SOCKET sAccept, FILE *resource,long filelen)
{
    long result=0;
    char send_buf[BUF_LENGTH];
    char flag=0;
    while (filelen>0)
    {
        if(filelen<BUF_LENGTH) flag=1;
        memset(send_buf,0,BUF_LENGTH);       //������0
        if(flag)
        result = fread (send_buf,1,filelen,resource);
        else
        result = fread (send_buf,1,BUF_LENGTH,resource);
        filelen-=result;
        if (SOCKET_ERROR == send(sAccept, send_buf, result, 0))
        {
            printf("send() Failed:%d\n",WSAGetLastError());
            return USER_ERROR;
        }
        //printf("sending:%ld\n",result);
    }
    fclose(resource);
    return 0;
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
            printf("wait to receive\n");
            memset(recv_buf,0,sizeof(recv_buf));
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
