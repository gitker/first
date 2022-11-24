#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <limits.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/wait.h>

#define PUT 0
#define GET 1

int readn(int fd, void *in, int n)
{
    int readed = 0;
    char *data = (char *)in;
    if (n <= 0)
        return 0;
    while (readed < n)
    {
        int ret = read(fd, data + readed, n - readed);

        if ((ret < 0) && (errno == EINTR))
            continue;
        if (ret <= 0)
            return -1;

        readed += ret;
    }
    return 0;
}

int writen(int fd, void *data, int n)
{
    if (n <= 0)
    {
        return 0;
    }
    while (1)
    {
        int ret = write(fd, data, n);
        if ((ret < 0) && (errno == EINTR))
            continue;
        if (ret != n)
        {
            return -1;
        }
        return 0;
    }
    return 0;
}

static void net_set_io_timeout(int socket, int seconds)
{
    struct timeval tv;

    if (seconds > 0)
    {
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }
}

int sendfile(char *path, int fd)
{
    int len;
    int ret;
    char buf[4096];
    int res=-1;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (writen(fd, &len, 4) != 0)
    {
        return -1;
    }
    while (1)
    {
        ret = fread(buf, 1, 4096, fp);
        if (ret <= 0)
        {
            break;
        }
        if (writen(fd, buf, ret) != 0)
        {
            return -1;
        }
    }
    fclose(fp);
    if (readn(fd, &res, 4) != 0)
    {
        return -1;
    }
    if(res !=0)
    {
        printf("send error %d\n",res);
    }
    return 0;
}

int read_file(char *path, int fd)
{
    int size = 0;
    char buf[4096];
    int ret;
    int res=0;

    FILE *fp;
    if (readn(fd, &size, 4) != 0)
    {
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL)
    {
        return -1;
    }
    while (size > 0)
    {
        int want = 4096;
        if (size < 4096)
        {
            want = size;
        }
        ret = readn(fd, buf, want);
        if (ret < 0)
        {

           return -1;
        }
        fwrite(buf, want, 1, fp);
        size -= want;
    }
    fclose(fp);
    if(writen(fd,&res,4)!=0)
    {
        return -1;
    }
    read(fd,&res,1);
    return 0;
}

void cli(int fd)
{
    char opt = 1;
    int cmd = 0;
    int size = 0;
    char path[256];
    char buf[4096];
    int ret;
    FILE *fp;

    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(char));
    net_set_io_timeout(fd, 20);
    if (readn(fd, &cmd, 4) != 0)
    {
        return;
    }
    if (readn(fd, path, 256) != 0)
    {
        return;
    }
    if (cmd == PUT)
    {

        read_file(path,fd);

    }
    else
    {
        sendfile(path,fd);
    }
}

int main(int argc, char *argv[])
{

    int sockfd, new_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int sin_size, iDataNum;

    signal(SIGPIPE, SIG_IGN);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket error:%s\n\a", strerror(errno));
        return 0;
    }
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    if (bind(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
    {
        fprintf(stderr, "Bind error:%s\n\a", strerror(errno));
        return 0;
    }
    if (listen(sockfd, 5) == -1)
    {
        fprintf(stderr, "Listen error:%s\n\a", strerror(errno));
        return 0;
    }
    int cnt = 0;
    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)(&client_addr), (socklen_t *)&sin_size)) == -1)
        {
            fprintf(stderr, "Accept error:%s\n\a", strerror(errno));
            return 0;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            close(sockfd);
            if (fork() == 0)
            {
                cli(new_fd);
            }
            exit(0);
        }
        else
        {
            int status;
            waitpid(-1, &status, 0);
            close(new_fd);
        }
    }

    return 0;
}
