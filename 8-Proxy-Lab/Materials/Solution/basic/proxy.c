#include <stdio.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void build_reqheader(rio_t *rp, char *newreq, char *method, char *hostname, char *port, char *path);


int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage :%s <port> \n", argv[0]);
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);   // 防止收到SIGPIPE信号而过早终止

    // 打开监听端口
    listenfd = Open_listenfd(argv[1]);
    // 死循环监听端口，如果有请求进入就提供服务。
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);   // 提供服务的函数
        Close(connfd);
    }

    printf("%s", user_agent_hdr);
    return 0;
}

void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    int serverfd;
    int n;
    rio_t client_rio, server_rio;

    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char newreq[MAXLINE];

    // 面对客户端，自己是服务器，接收客户端发送的请求
    Rio_readinitb(&client_rio, fd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) {
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("client uri is: %s\n", uri);
    parse_uri(uri, hostname, port, path);   // 解析uri中元素
    //printf("hostname: %s\nport: %s\npath: %s\n", hostname, port, path);
    build_reqheader(&client_rio, newreq, method, hostname, port, path); //构造新的请求头
    //pintf("the new req is: \n");
    //printf("%s", newreq);

    // 与目标服务器建立连接
    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        fprintf(stderr, "connect to real server err");
        return;
    }

    // 发送请求报文
    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, newreq, strlen(newreq));

    // 将目标服务器的内容接收后原封不动转发给客户端
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE))!=0) {
        printf("get %d bytes from server\n", n);
        Rio_writen(fd, buf, n);
    }

    Close(serverfd);    // 别忘了关闭文件描述符
}

void parse_uri(char *uri, char *hostname, char *port, char *path) {
    // 假设uri: http://localhost:12345/home.html
    char *hostpos = strstr(uri, "//");
    if (hostpos==NULL) {
        // 此种情况下，uri为 /home.html
        char *pathpos = strstr(uri, "/");
        if (pathpos != NULL) {
            strcpy(path, pathpos);
            strcpy(port, "80"); //没有包含端口信息，则默认端口为80
            return;
        }
        hostpos = uri;
    } else {
        hostpos += 2;
    }
    char *portpos = strstr(hostpos, ":");
    // hostpos为：//localhost:12345/home.html
    // portpos为：:12345/home.html
    if (portpos != NULL) {
        int portnum;
        char *pathpos = strstr(portpos, "/");
        if (pathpos!=NULL) {
            sscanf(portpos+1, "%d%s", &portnum, path);
        } else {
            sscanf(portpos+1, "%d", &portnum);
        }
        sprintf(port, "%d", portnum);
        *portpos = '\0';
    } else {
        char *pathpos = strstr(hostpos, "/");
        if (pathpos != NULL) {
            strcpy(path, pathpos);
            *pathpos = '\0';
        }
        strcpy(port, "80");
    }
    strcpy(hostname, hostpos);
    return;
}

void build_reqheader(rio_t *rp, char *newreq, char *method, char *hostname, char *port, char *path) {
    sprintf(newreq, "%s %s HTTP/1.0\r\n", method, path);

    char buf[MAXLINE];
    // 循环从客户端输入中读取行
    while (Rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n")) break;    // 空行，表示请求头已经结束
        if (strstr(buf, "Host:")!=NULL) continue; // Host由我们自己设置
        if (strstr(buf, "User-Agent:")!=NULL) continue; // User-Agent由我们自己设置
        if (strstr(buf, "Connection:")!=NULL) continue; // Connection由我们自己设置
        if (strstr(buf, "Proxy-Connection:")!=NULL) continue; // Proxy-Connection由我们自己设置

        sprintf(newreq, "%s%s", newreq, buf);   // 其他请求头直接原封不动加入请求中
    }
    // 添加上请求的必要信息
    sprintf(newreq, "%sHost: %s:%s\r\n", newreq, hostname, port);
    sprintf(newreq, "%s%s", newreq, user_agent_hdr);
    sprintf(newreq, "%s%s", newreq, conn_hdr);
    sprintf(newreq, "%s%s", newreq, proxy_conn_hdr);
    sprintf(newreq, "%s\r\n", newreq);
}

