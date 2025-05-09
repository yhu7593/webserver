#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST, //请求不完整
        GET_REQUEST, //请求完整
        BAD_REQUEST, //请求错误
        NO_RESOURCE, //没有资源
        FORBIDDEN_REQUEST, //没有权限
        FILE_REQUEST, //文件请求
        INTERNAL_ERROR, //内部错误
        CLOSED_CONNECTION //关闭连接
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;//所有socket上的epoll事件都注册在同一个epollfd上
    static int m_user_count;//统计用户数量
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    long m_read_idx;//标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    long m_checked_idx;//当前正在分析的字符在读缓冲区中的位置
    int m_start_line;//当前正在解析的行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;//标识写缓冲区中待发送的字节数
    CHECK_STATE m_check_state;//主状态机当前所处状态
    METHOD m_method;//请求方法
    char m_real_file[FILENAME_LEN];//客户请求的目标文件的完整路径
    char *m_url;//请求目标文件的文件名
    char *m_version;//HTTP协议版本
    char *m_host;//主机名
    long m_content_length;//HTTP请求的消息总长度
    bool m_linger;//HTTP请求是否要保持连接
    char *m_file_address;//客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;//目标文件的状态
    struct iovec m_iv[2];   //io向量,分散写
    int m_iv_count; //io向量的数量
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;//剩余发送字节数
    int bytes_have_send;//已经发送字节数
    char *doc_root;//网站根目录

    map<string, string> m_users;//用户信息
    int m_TRIGMode;//触发模式
    int m_close_log;//日志开关

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
