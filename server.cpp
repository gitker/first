
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <map>
#include <pthread.h>

using namespace std;

typedef struct
{
    unsigned int magic;
    unsigned int ver;
    unsigned int cmd;
    unsigned char uid[64];
    unsigned int sid;
    unsigned int code;
    unsigned int msglen;
    unsigned int ext_data[16];
} msg_head;

#define get_attr_cmd 1
#define access_cmd 2
#define read_dir_cmd 3
#define open_cmd 4
#define read_cmd 5
#define release_cmd 6
#define write_cmd 7
#define make_rm_cmd 8
#define truncate_cmd 9
#define rename_cmd 10

typedef struct
{
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_mode;
    uint64_t st_nlink;
    uint64_t st_uid;
    uint64_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint64_t st_atime;
    uint64_t st_mtime;
    uint64_t st_ctime;
} statx;

#define MAGIC 0x123456

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

int readn(int fd, char *data, int n)
{
    int readed = 0;
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

int read_msg(msg_head *phd, int fd, char *buffer, int maxlen, unsigned int cmd)
{
    msg_head head;

    if (readn(fd, (char *)&head, sizeof(head)) < 0)
    {
        return -1;
    }

    if (head.magic != MAGIC || head.cmd != cmd)
    {
        printf("%u %u\n", head.magic, head.cmd);
        return -3;
    }
    if (head.msglen > (unsigned int)maxlen)
    {
        return -4;
    }
    if (readn(fd, buffer, head.msglen) < 0)
    {
        return -5;
    }
    if (phd != NULL)
    {
        memcpy(phd, &head, sizeof(msg_head));
    }
    return head.msglen;
}

int write_msg(int fd, char *resbuf, int len, unsigned char *uid, unsigned int cmd)
{
    msg_head head;

    memcpy(head.uid, uid, 64);
    head.msglen = len;
    head.magic = MAGIC;
    head.cmd = cmd;
    head.code = 220;

    if (writen(fd, (char *)&head, sizeof(head)) < 0)
    {
        return -1;
    }

    if (resbuf == NULL || len == 0)
    {
        return 0;
    }

    if (writen(fd, resbuf, len) < 0)
    {
        return -1;
    }

    return 0;
}

const char *get_path(char *gpath, const char *path)
{
    memset(gpath, 0, 512);
    // gpath[0] = '.';
    // gpath[1] = '/';
    strcpy(gpath, path);
    return gpath;
}

static int mknod_wrapper(int dirfd, const char *path, const char *link,
                         int mode, dev_t rdev)
{
    int res;

    if (S_ISREG(mode))
    {
        //  res = openat(dirfd, path, O_CREAT | O_EXCL | O_WRONLY, mode);
        //  if (res >= 0)
        //     res = close(res);
    }
    else if (S_ISDIR(mode))
    {
        // res = mkdirat(dirfd, path, mode);
    }
    else if (S_ISLNK(mode) && link != NULL)
    {
        // res = symlinkat(link, dirfd, path);
    }
    else if (S_ISFIFO(mode))
    {
        // res = mkfifoat(dirfd, path, mode);
    }
    else
    {
        // res = mknodat(dirfd, path, mode, rdev);
    }

    return res;
}

void copy_stat(statx *dst, struct stat *src)
{
    dst->st_atime = src->st_atime;
    dst->st_ctime = src->st_ctime;
    dst->st_mtime = src->st_mtime;
    dst->st_dev = src->st_dev;
    dst->st_mode = src->st_mode;
    dst->st_ino = src->st_ino;
    dst->st_nlink = src->st_nlink;
    dst->st_uid = src->st_uid;
    dst->st_gid = src->st_gid;
    dst->st_rdev = src->st_rdev;
    dst->st_size = src->st_size;
    dst->st_blksize = src->st_blksize;
    dst->st_blocks = src->st_blocks;
}

// in  path
// out  ret+stbuf
static int xmp_getattr(const char *path, statx *stbuf)
{
    int res;
    char gpath[512];
    struct stat ss;

    // printf("get addr %s\n", get_path(path));
    //   fflush(stdout);

    res = lstat(get_path(gpath, path), &ss);

    copy_stat(stbuf, &ss);
    if (res == -1)
        return -errno;
    return 0;
}

// in   mask + path
// out  ret
static int xmp_access(const char *path, int mask)
{
    int res;
    char gpath[512];

    res = access(get_path(gpath, path), mask);
    if (res == -1)
        return -errno;

    return 0;
}

typedef struct
{
    uint64_t d_ino;
    uint64_t d_type;
    uint64_t namelen;
} dir_entry;

typedef struct
{
    char name[512];
    uint64_t flag;
    uint64_t fi;
    uint64_t mode;
    uint64_t type;
} open_t;

typedef struct
{
    int64_t fd;
    uint64_t size;
    uint64_t offset;
} read_t;

typedef struct
{
    int64_t fd;
    uint64_t size;
    uint64_t offset;
} write_t;

typedef struct
{
    char path[512];
    int64_t type;
    uint64_t mode;
    uint64_t rdev;
} mk_rm_t;

typedef struct
{
    char path[512];
    int64_t fd;
    uint64_t size;
} truncate_t;

typedef struct
{
    char from[512];
    char to[512];
} rename_t;

static int xmp_open(const char *path, uint64_t fi, uint64_t flag, int type, uint64_t mode)
{
    int res;
    char gpath[512];

    if (type == 1)
    {
        res = open(get_path(gpath, path), flag, mode);
    }
    else
    {
        res = open(get_path(gpath, path), flag);
    }

    if (res == -1)
        return -errno;

    return res;
}

static int xmp_read(int fd, uint64_t size, uint64_t offset, char *buf)
{
    int res;

    res = pread(fd, buf, size, offset);
    if (res < 0)
    {
        res = -errno;
    }
    return res;
}

static int xmp_release(int fd)
{

    close(fd);
    return 0;
}

struct buffer_ary
{
    char *buf;
    int maxlen;
    int used;
    buffer_ary()
    {
        maxlen = 1024;
        buf = (char *)malloc(maxlen);
        used = 0;
    }
    int push(void *buffer, int len)
    {
        if (len + used > maxlen)
        {
            int new_max = maxlen;
            while (len + used > new_max)
            {
                new_max = 2 * new_max;
            }
            char *c = (char *)malloc(new_max);
            memcpy(c, buf, used);
            free(buf);
            buf = c;
            maxlen = new_max;
        }
        memcpy(buf + used, buffer, len);
        used += len;
        return 0;
    }
};

int process_msg(unsigned int cmd, char *msg, int msglen, char **presbuf, int *reslen)
{
    int outlen = 0;
    int ret = 0;

    char *resbuf = NULL;

    if (cmd == get_attr_cmd)
    {
        statx st;

        resbuf = (char *)malloc(4 + sizeof(st));
        if (msg[msglen - 1] != 0 || msglen < 2)
        {
            ret = -2;
        }
        else
        {
            ret = xmp_getattr(msg, &st);
        }
        memcpy(resbuf, &ret, 4);
        outlen += 4;
        memcpy(resbuf + outlen, &st, sizeof(st));
        outlen += sizeof(st);
        *reslen = outlen;
    }
    if (cmd == access_cmd)
    {
        int mask;
        resbuf = (char *)malloc(4);
        if (msglen < 6 || msg[msglen - 1] != 0)
        {
            ret = -2;
        }
        else
        {
            memcpy(&mask, msg, 4);
            ret = xmp_access(msg + 4, mask);
        }
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }
    if (cmd == read_dir_cmd)
    {
        DIR *dp;
        struct dirent *de;
        dir_entry entry;
        char gpath[512];

        if (msg[msglen - 1] != 0 || msglen < 2)
        {
            *reslen = 0;
            return 0;
        }

        buffer_ary ary;

        dp = opendir(get_path(gpath, msg));
        if (dp == NULL)
        {
            ret = -errno;
        }

        ary.push(&ret, 4);

        while (dp != NULL && ((de = readdir(dp)) != NULL))
        {
            uint64_t d_ino;
            uint64_t d_type;
            int namelen;
            memset(&entry, 0, sizeof(entry));

            entry.d_ino = de->d_ino;
            entry.d_type = de->d_type << 12;
            namelen = strlen(de->d_name);
            entry.namelen = namelen;

            ary.push(&entry, sizeof(entry));
            ary.push(de->d_name, namelen);
        }
        *reslen = ary.used;
        resbuf = ary.buf;
        if (dp != NULL)
        {
            closedir(dp);
        }
    }
    if (cmd == open_cmd)
    {
        open_t op;
        if (msglen != sizeof(op))
        {
            *reslen = 0;
            return 0;
        }
        memcpy(&op, msg, sizeof(op));

        ret = xmp_open(op.name, op.fi, op.flag, op.type, op.mode);

        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }
    if (cmd == read_cmd)
    {
        read_t op;
        memcpy(&op, msg, sizeof(op));

        if (msglen != sizeof(op) || op.size > 0x100000)
        {
            *reslen = 0;
            return 0;
        }
        resbuf = (char *)malloc(4 + op.size);
        ret = xmp_read(op.fd, op.size, op.offset, resbuf + 4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
        if (ret > 0)
        {
            *reslen += ret;
        }
    }
    if (cmd == release_cmd)
    {
        int fd;
        int ret = 0;

        if (msglen != 4)
        {
            *reslen = 0;
            return 0;
        }
        memcpy(&fd, msg, 4);
        xmp_release(fd);

        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }
    if (cmd == write_cmd)
    {
        write_t op;

        if (msglen <= sizeof(op))
        {

            *reslen = 0;
            return 0;
        }
        memcpy(&op, msg, sizeof(op));

        if (msglen != sizeof(op) + op.size)
        {
            *reslen = 0;
            return 0;
        }

        int ret = pwrite(op.fd, msg + sizeof(op), op.size, op.offset);
        if (ret < 0)
        {
            ret = -errno;
        }
        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }
    if (cmd == make_rm_cmd)
    {
        mk_rm_t op;
        char gpath[512];

        if (msglen != sizeof(op))
        {
            *reslen = 0;
            return 0;
        }
        memcpy(&op, msg, sizeof(op));
        get_path(gpath, op.path);
        switch (op.type)
        {
        case 0:
            ret = mknod_wrapper(0, gpath, NULL, op.mode, op.rdev);
            break;

        case 1:
            ret = mkdir(gpath, op.mode);
            break;
        case 2:
            ret = unlink(gpath);
            break;
        case 3:
            ret = rmdir(gpath);
            break;
        case 4:
            ret = chmod(gpath, op.mode);
            break;
        default:
            ret = 0;
        }
        if (ret < 0)
        {
            ret = -errno;
        }
        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }
    if (cmd == truncate_cmd)
    {
        truncate_t op;
        char gpath[512];

        if (msglen != sizeof(op))
        {
            *reslen = 0;
            return 0;
        }

        memcpy(&op, msg, sizeof(op));
        if (op.fd < 0)
        {
            ret = truncate(get_path(gpath, op.path), op.size);
        }
        else
        {
            ret = ftruncate(op.fd, op.size);
        }
        if (ret < 0)
        {
            ret = -errno;
        }
        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }

    if (cmd == rename_cmd)
    {
        rename_t op;
        char f[512];
        char t[512];

        if (msglen != sizeof(op))
        {
            *reslen = 0;
            return 0;
        }

        memcpy(&op, msg, sizeof(op));

        ret = rename(get_path(f, op.from), get_path(t, op.to));
        if (ret < 0)
        {
            ret = -errno;
        }
        resbuf = (char *)malloc(4);
        memcpy(resbuf, &ret, 4);
        *reslen = 4;
    }

    *presbuf = resbuf;

    return 0;
}

#define MAX_MSG_SIZE 0x100000

int process_result(int fd)
{
    char *msg_in = NULL;
    char *msg_out = NULL;

    int ret = 0;

    msg_head head;
    int out_len = 0;

    {
        if (readn(fd, (char *)&head, sizeof(head)) < 0)
        {
            ret = -1;
            goto out;
        }
        if (head.magic != MAGIC)
        {
            printf("%u \n", head.magic);
            ret = -2;
            goto out;
        }
        if (head.msglen > MAX_MSG_SIZE || head.msglen == 0)
        {
            ret = -3;
            goto out;
        }
        msg_in = (char *)malloc(head.msglen);
        if (readn(fd, msg_in, head.msglen) < 0)
        {
            ret = -4;
            goto out;
        }
        if (process_msg(head.cmd, msg_in, head.msglen, &msg_out, &out_len) < 0)
        {
            ret = -5;
            goto out;
        }
        if (write_msg(fd, msg_out, out_len, head.uid, head.cmd) < 0)
        {
            ret = -6;
            goto out;
        }
    }
out:
    if (msg_in)
    {
        free(msg_in);
    }
    if (msg_out)
    {
        free(msg_out);
    }
    return ret;
}

void *active(void *arg)
{
    int *pfd = (int *)arg;

    char opt = 1;
    setsockopt(*pfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(char));

    net_set_io_timeout(*pfd, 10);

    while (process_result(*pfd) == 0)
    {
    }

    close(*pfd);
    free(pfd);

    return NULL;
}

int server(char *port)
{
    int sock_fd, conn_fd;
    struct sockaddr_in server_addr;
    int reuse = 1;

    signal(SIGPIPE, SIG_IGN);

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket error:%s\n\a", strerror(errno));
        exit(1);
    }

    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(port));

    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(int));

    if (bind(sock_fd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
    {
        fprintf(stderr, "Bind error:%s\n\a", strerror(errno));
        exit(1);
    }

    if (listen(sock_fd, 5) == -1)
    {
        fprintf(stderr, "Listen error:%s\n\a", strerror(errno));
        exit(1);
    }
    while (1)
    {

        if ((conn_fd = accept(sock_fd, (struct sockaddr *)NULL, NULL)) == -1)
        {
            printf("accept socket error: %s\n\a", strerror(errno));
            continue;
        }

        int *ff = new int;
        *ff = conn_fd;

        pthread_t myThread1;
        pthread_create(&myThread1, NULL, active, ff);
        pthread_detach(myThread1);
    }
}

int main(int argc, char **argv)
{
    printf("stat size %d\n", (int)sizeof(statx));
    server(argv[1]);
}
