#ifndef _SSL_SELECT_H
#define _SSL_SELECT_H
#include <openssl/ssl.h>
#include <openssl/err.h>

enum{ 	
	SSL_OPS_FAIL	= -1,
	SSL_OPS_SELECT = -2
};



typedef struct {
	int write;
	int read;
}wait_event;

typedef struct {
	int sk;
	SSL *ssl;
	SSL_CTX *ctx;
	wait_event recv;
	wait_event send;
	wait_event connect;
	wait_event accept;
}ssl_info;

#ifndef SSL_SEL_PASSWORD_MAX
#define SSL_SEL_PASSWORD_MAX	512
#endif

typedef struct{
	int len;
	char data[SSL_SEL_PASSWORD_MAX];
}ssl_pwd_data;

ssl_info * sslinfo_alloc(void);

void init_ssl_lib(void);
SSL_CTX *initialize_ctx(char *rootCA, char * keyfile, char * password, ssl_pwd_data * buff);
void destroy_ctx(SSL_CTX *ctx);

int __set_block(int sock);
int __set_nonblock(int sock);

int ssl_connect_direct(ssl_info * info, int *ssl_errno);
int ssl_send_direct(ssl_info * info, void *buf, int len, int *ssl_errno);
int ssl_recv_direct(ssl_info * info, void * buf, int len, int *ssl_errno);
int ssl_accept_direct(ssl_info * info, int *ssl_errno);

int ssl_connect_simple(ssl_info * info, int to_ms, int *ssl_errno);
int ssl_accept_simple(ssl_info * info, int to_ms, int *ssl_errno);
int ssl_send_simple(ssl_info * info, void * buf, int len, int to_ms, int *ssl_errno);
int ssl_recv_simple(ssl_info * info, void * buf, int len, int to_ms, int *ssl_errno);

int ssl_connect_simple_tv(ssl_info * info, struct timeval * tv, int *ssl_errno);
int ssl_connect_accept_tv(ssl_info * info, struct timeval * tv, int *ssl_errno);
int ssl_send_simple_tv(ssl_info * info, void * buf, int len, struct timeval * tv, int *ssl_errno);
int ssl_recv_simple_tv(ssl_info * info, void * buf, int len, struct timeval * tv, int *ssl_errno);

int ssl_set_fds(ssl_info *info, int maxfd, fd_set *rfds, fd_set *wfds);
int ssl_errno_str(ssl_info * info, int ssl_errno, char * buf, int buflen);

#define invoke_ssl_send	(1 << 0)
#define invoke_ssl_recv	(1 << 1)
#define invoke_ssl_connect (1 << 2)
#define invoke_ssl_accept (1 << 3)
int ssl_handle_fds(ssl_info * info, fd_set *rfds, fd_set *wfds);

#endif
