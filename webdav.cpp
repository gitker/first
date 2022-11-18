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
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <iostream>
using namespace std;

static int log = 0;

#define BC_LOG(fmt, arg...)     \
    do                          \
    {                           \
        if (log)                \
        {                       \
            printf(fmt, ##arg); \
        }                       \
    } while (0)

struct file
{
    string name;
    string mtime;
    string etag;
    string len;
    string type;
};

struct dentry
{
    string name;
    string mtime;
};

string get_pair(string name, string value)
{
    char str[512];

    sprintf(str, "<D:%s>%s</D:%s>", name.c_str(), value.c_str(), name.c_str());

    return str;
}

void add_entry(dentry *entry, string shref, string &res)
{
    char href[512];
    sprintf(href, "<D:href>%s</D:href>", shref.c_str());
    res += "<D:response>";
    res += href;
    res += "<D:propstat>";
    res += "<D:prop>";
    res += "<D:resourcetype> <D:collection xmlns:D=\"DAV:\"/></D:resourcetype>";
    res += get_pair("dislpayname", entry->name);
    res += get_pair("getlastmodified", entry->mtime);
    res += "</D:prop>";
    res += "<D:status>HTTP/1.1 200 OK</D:status>";
    res += "</D:propstat>";
    res += "</D:response>";
}

void add_file(file *entry, string shref, string &res)
{
    char href[512];
    sprintf(href, "<D:href>%s</D:href>", shref.c_str());
    res += "<D:response>";
    res += href;
    res += "<D:propstat>";
    res += "<D:prop>";
    res += "<D:resourcetype> </D:resourcetype>";
    res += get_pair("dislpayname", entry->name);
    res += get_pair("getlastmodified", entry->mtime);
    res += get_pair("getetag", entry->etag);
    res += get_pair("getcontentlength", entry->len);
    res += get_pair("getcontenttype", entry->type);
    res += "</D:prop>";
    res += "<D:status>HTTP/1.1 200 OK</D:status>";
    res += "</D:propstat>";
    res += "</D:response>";
}

string get_name(string href)
{
    vector<char> dp;
    string res = "";
    int n = href.size() - 1;
    while (n >= 0 && href[n] == '/')
        n--;
    if (n < 0)
    {
        return res;
    }
    while (n >= 0 && href[n] != '/')
    {
        dp.push_back(href[n]);
        n--;
    }
    for (int i = dp.size() - 1; i >= 0; i--)
    {
        res = res + dp[i];
    }
    return res;
}

int get_val(unsigned char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return 0;
}

unsigned char val(unsigned char *p)
{
    return get_val(p[0]) * 16 + get_val(p[1]);
}

char *urlDecode(const char *str)
{

    int maxlen = strlen(str);
    char *res = (char *)calloc(maxlen + 1, 1);
    unsigned char *p = (unsigned char *)res;
    unsigned char *src = (unsigned char *)str;

    for (int i = 0; i < maxlen;)
    {
        if (src[i] == '%')
        {
            if (i + 2 >= maxlen)
                break;
            *p = val((unsigned char *)src + i + 1);
            p++;
            i += 3;
        }
        else
        {
            *p = src[i];
            p++;
            i += 1;
        }
    }

    return res;
}

void format_file(string href, struct stat stbuf, string name, string &res)
{
    file f;
    char flen[128];
    if(S_ISCHR(stbuf.st_mode))
    {
        stbuf.st_size = 1024;
    }
    sprintf(flen, "%llu", (unsigned long long)stbuf.st_size);
    f.name = name;
    f.etag = "12345";
    f.len = flen;
    f.type = "application/octet-stream";
    add_file(&f, href, res);
}

void write_str(int fd, const char *a)
{
    if(write(fd, a, strlen(a)) != strlen(a))
    {
        exit(0);
    }
}

void write_404(int fd)
{
    write_str(fd, "HTTP/1.1 404 Not Found\r\n");
    write_str(fd, "Content-Length: 9\r\n");
    write_str(fd, "\r\n");
    write_str(fd, "Not Found");
}

void write_errcode(int fd, int err)
{
    char code[128];
    sprintf(code, "HTTP/1.1 %d Conflict\r\n", err);
    write_str(fd, code);
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "\r\n");
}

int is_unreseved(char a)
{
    if (a == '-' || a == '.' || a == '_' || a == '\"')
    {
        return 1;
    }
    if (a >= 'a' && a <= 'z')
        return 1;
    if (a >= 'A' && a <= 'Z')
        return 1;
    if (a >= '0' && a <= '9')
        return 1;

    return 0;
}

string to_url(char *str)
{
    string res = "";
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        char cc[10];
        unsigned char *p = (unsigned char *)str;
        if (!is_unreseved(*str))
        {
            sprintf(cc, "%02x", *p);
            res = res + "%" + cc;
        }
        else
        {
            res = res + *str;
        }
        str++;
    }
    return res;
}

void readdir(string topdir, string href, string rhf, string &res, string depth)
{
    string path = href;

    struct stat stbuf;

    if (lstat(path.c_str(), &stbuf) < 0)
    {
        return;
    }
    BC_LOG("readir %s %s\n", path.c_str(), rhf.c_str());

    if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
    {
        dentry entry;
        entry.name = get_name(href);
        add_entry(&entry, rhf, res);
    }
    else
    {
        format_file(rhf, stbuf, get_name(href), res);
        BC_LOG("format file %s\n", get_name(href).c_str());
        return;
    }

    if (depth[depth.size() - 1] == '0')
    {
        return;
    }

    DIR *dirp = opendir(path.c_str());
    if (dirp == NULL)
    {
        return;
    }
    dirent *dp = NULL;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            string rpath = path + "/" + dp->d_name;
            struct stat stbuf;
            lstat(rpath.c_str(), &stbuf);
            if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
            {
                dentry entry;
                entry.name = dp->d_name;
                add_entry(&entry, rhf + "/" + to_url(dp->d_name), res);
            }
            else
            {
                format_file(rhf + "/" + to_url(dp->d_name), stbuf, dp->d_name, res);
            }
        }
    }
    closedir(dirp);
}

void toxml(string href, string rhf, string &res, string depth)
{
    res = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    res = res + "<D:multistatus xmlns:D=\"DAV:\">";
    readdir(".", href, rhf, res, depth);
    res = res + "</D:multistatus>";
}

void readline(int fd, string &s)
{
    char c;
    while (1)
    {
        if (read(fd, &c, 1) != 1)
        {
            break;
        }
        BC_LOG("%c", c);
        if (c == '\n')
            break;
        if (c != '\r')
        {
            s = s + c;
        }
    }
}

bool head_is(string a, const char *str, int len)
{
    for (int i = 0; i < len && i < a.size(); i++)
    {
        if (a[i] != str[i])
        {
            return false;
        }
    }
    return true;
}

void write_options(int fd)
{
    write_str(fd, "HTTP/1.1 200 OK\r\n");
    write_str(fd, "Allow: OPTIONS, DELETE,PROPPATCH, COPY, MOVE,  PROFIND\r\n");
    write_str(fd, "Dav: 1, 2\r\n");
    write_str(fd, "Ms-Author-Via: DAV\r\n");
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "\r\n");
}

void split(vector<string> &res, string a)
{
    string tmp = "";
    for (int i = 0; i < a.size(); i++)
    {
        if (a[i] == ' ' || a[i] == '\r' || a[i] == '\n')
        {
            if (tmp.size() != 0)
            {
                res.push_back(tmp);
                tmp = "";
            }
        }
        else
        {
            tmp = tmp + a[i];
        }
    }
}

void process_get(int fd, vector<string> &request)
{
    vector<string> cmd;
    char lenstr[128];

    split(cmd, request[0]);

    struct stat stbuf;

    string path = (char *)urlDecode(cmd[1].c_str());
    path = "." + path;

    if (lstat(path.c_str(), &stbuf) < 0 || (stbuf.st_mode & S_IFMT) == S_IFDIR)
    {
        write_404(fd);
        return;
    }
    if(S_ISCHR(stbuf.st_mode))
    {
        stbuf.st_size = 1024;
    }
    sprintf(lenstr, "Content-Length: %llu\r\n", (unsigned long long)stbuf.st_size);
    write_str(fd, "HTTP/1.1 200 OK\r\n");
    write_str(fd, "Content-Type: application/octet-stream\r\n");
    write_str(fd, lenstr);
    write_str(fd, "\r\n");

    FILE *fp = fopen(path.c_str(), "rb");
    while (1)
    {
        char buf[4096];
        int ret = fread(buf, 1, 4096, fp);
        if (ret > 0)
        {
            if(write(fd, buf, ret)!=ret)
            {
                exit(0);
            }
        }
        else
        {
            break;
        }
    }
    fclose(fp);
}

void read_content(int fd, unsigned long long length, string *res = NULL)
{
    for (int i = 0; i < length; i++)
    {
        char c;
        if (read(fd, &c, 1) != 1)
            break;
        BC_LOG("%c", c);
        if (res != NULL)
        {
            *res = *res + c;
        }
    }
    BC_LOG("\n");
}

void get_content_length(vector<string> &request, unsigned long long &length)
{
    for (int i = 0; i < request.size(); i++)
    {
        if (head_is(request[i], "Content-Length:", strlen("Content-Length:")))
        {
            sscanf(request[i].c_str() + strlen("Content-Length:"), "%llu", &length);
            break;
        }
    }
}

void process_profind(int fd, vector<string> &request)
{
    string res = "";
    int depi = 0;
    char lenstr[128];
    vector<string> cmd;
    unsigned long long length = 0;

    split(cmd, request[0]);

    struct stat stbuf;

    string path = urlDecode(cmd[1].c_str());
    path = "." + path;

    BC_LOG("#####pro find %s\n", path.c_str());

    if (lstat(path.c_str(), &stbuf) < 0)
    {
        write_404(fd);
        return;
    }

    for (int i = 0; i < request.size(); i++)
    {
        if (head_is(request[i], "Depth", strlen("Depth")))
        {
            depi = i;
        }
        if (head_is(request[i], "Content-Length:", strlen("Content-Length:")))
        {
            sscanf(request[i].c_str() + strlen("Content-Length:"), "%llu", &length);
        }
    }

    read_content(fd, length);

    toxml(path, cmd[1], res, request[depi]);
    sprintf(lenstr, "Content-Length: %d\r\n", (int)res.size());
    write_str(fd, "HTTP/1.1 207 Multi-Status\r\n");
    write_str(fd, "Date: Mon, 18 OCt 2021 03:02:30 GMT\r\n");
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, lenstr);
    write_str(fd, "\r\n");
    write_str(fd, res.c_str());
}

void process_put(int fd, vector<string> &request)
{
    string res = "";
    vector<string> cmd;
    split(cmd, request[0]);
    string path = urlDecode(cmd[1].c_str());
    path = "." + path;
    unsigned long long length = 0;

    get_content_length(request, length);

    BC_LOG("put content length %llu\n", length);
    FILE *fp = fopen(path.c_str(), "wb");
    if (fp == NULL)
    {
        write_404(fd);
        return;
    }
    while (length > 0)
    {
        char buf[4096];
        int read_sz = 4096;
        if (read_sz > length)
        {
            read_sz = length;
        }
        int ret = read(fd, buf, read_sz);
        if (ret <= 0)
        {
            break;
        }
        fwrite(buf, 1, ret, fp);
        length -= ret;
    }
    fclose(fp);
    BC_LOG("###create file %s ok\n", path.c_str());
    write_str(fd, "HTTP/1.1 201 Created\r\n");
    write_str(fd, "Etag: 123456\r\n");
    write_str(fd, "Content-Type: text/plain; chareset=utf-8\r\n");
    write_str(fd, "Content-Length:7\r\n");
    write_str(fd, "\r\n");
    write_str(fd, "Created");
}

void write_lock(string &res)
{
    res = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    res = res + "<D:prop xmlns:D=\"DAV:\">";
    res = res + "<D:lockdiscovery><D:activelock>" +
          "<D:locktype><D:write/></D:locktype>" +
          "<D:lockscope><D:exclusive/></D:lockscope>" +
          "<D:depth>infinity</D:depth>" +
          " <D:timeout>Second-3600</D:timeout>" +
          "<D:locktoken><D:href>1634111331</D:href></D:locktoken>" +
          "</D:activelock></D:lockdiscovery>";
    res = res + "</D:prop>";
}

void process_lock(int fd, vector<string> &request)
{
    char lenstr[128];
    vector<string> cmd;
    split(cmd, request[0]);
    // string path =   urlDecode(cmd[1].c_str());;
    //  path = "."+path;
    unsigned long long length = 0;

    get_content_length(request, length);
    read_content(fd, length);

    string res;
    write_lock(res);

    sprintf(lenstr, "Content-Length: %d\r\n", (int)res.size());
    write_str(fd, "HTTP/1.1 200 OK\r\n");
    write_str(fd, lenstr);
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "Lock-Token: 1634111331\r\n");
    write_str(fd, "\r\n");
    write_str(fd, res.c_str());
}

void process_patch(int fd, vector<string> &request)
{
    char lenstr[128];
    vector<string> cmd;
    split(cmd, request[0]);
    string path = urlDecode(cmd[1].c_str());
    ;
    path = "." + path;
    unsigned long long length = 0;
    string res = "";

    get_content_length(request, length);

    read_content(fd, length, &res);

    sprintf(lenstr, "Content-Length: %d\r\n", (int)res.size());
    write_str(fd, "HTTP/1.1 200 OK\r\n");
    write_str(fd, lenstr);
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "\r\n");
    write_str(fd, res.c_str());
}

void process_unlock(int fd, vector<string> &request)
{

    write_str(fd, "HTTP/1.1 204 No Content\r\n");
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "\r\n");
}

void process_delete(int fd, vector<string> &request)
{
    string res = "";
    char lenstr[128];
    vector<string> cmd;
    vector<string> depth;

    split(cmd, request[0]);

    struct stat stbuf;

    string path = urlDecode(cmd[1].c_str());
    ;
    path = "." + path;

    BC_LOG("delete %s\n", path.c_str());

    if (lstat(path.c_str(), &stbuf) < 0)
    {
        write_404(fd);
        return;
    }
    else
    {
        BC_LOG("####found %s\n", path.c_str());
        char cmd[512];
        sprintf(cmd, "rm -rf '%s'", path.c_str());
        if(system(cmd)!=0)
        {
            BC_LOG("rm '%s' error\n",path.c_str());
        }
    }

    write_str(fd, "HTTP/1.1 200 OK\r\n");
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "\r\n");
}

void get_real_name(char *host, char *real)
{
    int cnt = 0;
    int get = 0;
    *real = '.';
    real++;
    for (int i = 0; i < strlen(host); i++)
    {
        if (host[i] == '/')
        {
            cnt++;
            if (cnt == 3)
            {
                get = 1;
            }
        }
        if (get && host[i] != '\r' && host[i] != '\n')
        {
            *real = host[i];
            real++;
        }
    }
}

void process_move(int fd, vector<string> &request)
{
    string res = "";
    char lenstr[128];
    vector<string> cmd;
    vector<string> depth;
    char dest[255] = {0};

    split(cmd, request[0]);

    struct stat stbuf;

    string path = urlDecode(cmd[1].c_str());
    ;
    path = "." + path;

    if (lstat(path.c_str(), &stbuf) < 0)
    {
        write_404(fd);
        return;
    }

    for (int i = 0; i < request.size(); i++)
    {
        if (head_is(request[i], "Destination:", strlen("Destination:")))
        {
            strcpy(dest, request[i].c_str() + strlen("Destination:"));
            break;
        }
    }
    BC_LOG("dest %s\n", dest);
    char real[512] = {0};
    get_real_name(dest, real);
    BC_LOG("real %s\n", real);
    char *rr = urlDecode(real);
    char scmd[512];
    sprintf(scmd, "mv '%s' '%s'", path.c_str(), rr);
    BC_LOG("#####mv %s %s\n", path.c_str(), rr);

    if(system(scmd) !=0)
    {
        BC_LOG("move error\n");
    }

    write_str(fd, "HTTP/1.1 201 Created\r\n");
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "\r\n");
}

void process_mkcol(int fd, vector<string> &request)
{
    string res = "";
    char lenstr[128];
    vector<string> cmd;
    vector<string> depth;

    split(cmd, request[0]);

    struct stat stbuf;

    string path = urlDecode(cmd[1].c_str());
    ;
    path = "." + path;

    if (lstat(path.c_str(), &stbuf) >= 0)
    {
        write_errcode(fd, 409);
        return;
    }
    else
    {
        char cmd[512];
        sprintf(cmd, "mkdir '%s'", path.c_str());
        if(system(cmd)!=0)
        {
            BC_LOG("mkdir error\n");
        }
    }

    write_str(fd, "HTTP/1.1 201 Created\r\n");
    write_str(fd, "Content-Type: text/xml; chareset=utf-8\r\n");
    write_str(fd, "Content-Length: 0\r\n");
    write_str(fd, "\r\n");
}

void cli(int fd)
{

    vector<string> request;
    while (1)
    {
        string s = "";
        readline(fd, s);
        if (s.size() == 0)
            break;
        request.push_back(s);
    }
    if (request.size() == 0)
    {
        return;
    }
    if (head_is(request[0], "OPTIONS", 7))
    {
        write_options(fd);
    }
    if (head_is(request[0], "PROPFIND", 7))
    {
        process_profind(fd, request);
    }
    if (head_is(request[0], "GET", 3))
    {
        process_get(fd, request);
    }
    if (head_is(request[0], "PUT", 3))
    {
        process_put(fd, request);
    }
    if (head_is(request[0], "LOCK", 4))
    {
        process_lock(fd, request);
    }
    if (head_is(request[0], "PROPPATCH", 9))
    {
        process_patch(fd, request);
    }
    if (head_is(request[0], "UNLOCK", 6))
    {
        process_unlock(fd, request);
    }
    if (head_is(request[0], "DELETE", 6))
    {
        process_delete(fd, request);
    }
    if (head_is(request[0], "MKCOL", 5))
    {
        process_mkcol(fd, request);
    }
    if (head_is(request[0], "MOVE", 4))
    {
        process_move(fd, request);
    }
}



int get_addr(const char *host,char *str)
{
    int ret;
    struct addrinfo hints, *addr_list, *cur;
    int fd= -1;

    /*let the process be able to handle write error,not just exit*/
    signal(SIGPIPE, SIG_IGN);

    /* Do name resolution with both IPv6 and IPv4 */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, "88", &hints, &addr_list) != 0)
        return -1;

    /* Try the sockaddrs until a connection succeeds */
    ret = -1;
    for (cur = addr_list; cur != NULL; cur = cur->ai_next)
    {
        
        strcpy(str,inet_ntoa(((struct sockaddr_in*)cur->ai_addr)->sin_addr));
        ret = 0;
        break;
       
    }

    freeaddrinfo(addr_list);

    return ret;
  
}


int main(int argc, char *argv[])
{

    BC_LOG("Initialised.\n");
    int sockfd, new_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int sin_size, iDataNum;

    char caddr[126];
    char xaddr[126];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket error:%s\n\a", strerror(errno));
        return 0;
    }
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (argc > 2)
    {
        log = 1;
    }

  

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

        if(get_addr("snrtibi.vicp.net",caddr)!=0)
        {
            close(new_fd);
            continue;
        }
        if(strcmp(inet_ntoa(client_addr.sin_addr),caddr) !=0)
        {
            printf("get bad guy %s\n",inet_ntoa(client_addr.sin_addr));
            close(new_fd);
            continue;
        }

        // fprintf(stdout, "Server get connection from %s\n", inet_ntoa(client_addr.sin_addr));
        // fflush(stdout);

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
