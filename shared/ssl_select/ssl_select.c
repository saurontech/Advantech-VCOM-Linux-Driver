#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> 
#include <ctype.h>
#include <dirent.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <syslog.h>
#include <netinet/tcp.h>
#include <sys/mman.h>
#include <linux/limits.h>

#include "ssl_select.h"

static void sigpipe_handle(int x){
}

static int password_cb(char *buf,int num,
		int rwflag,void *userdata)
{
	ssl_pwd_data * _password;
	
	if(userdata == 0)
		return 0;

	_password = (ssl_pwd_data *)userdata;

	if(_password->len > num){
		printf("sys buf len %d < passlen %d\n", num, _password->len);
		return 0;
	}

	snprintf(buf, num, "%s", _password->data);

	return _password->len;
}

static int verify_callback (int ok, X509_STORE_CTX *store)
{

	X509 *cert = X509_STORE_CTX_get_current_cert(store);

	if (!ok)
	{
		char data[256];
		int  depth = X509_STORE_CTX_get_error_depth(store);
		int  err = X509_STORE_CTX_get_error(store);

		printf("-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof(data));
		printf("  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, sizeof(data));
		printf("  subject  = %s\n", data);
		printf("  err %i:%s\n", err, X509_verify_cert_error_string(err) );
	}/*else{
	   printf("ssl verification OK\n");
	   X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
	   printf("  issuer   = %s\n", data);
	   X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
	   printf("  subject  = %s\n", data);
	   }*/	

	return ok;
}


void init_ssl_lib(void)
{
	SSL_library_init();
	SSL_load_error_strings();
}

static int _strvalid(char * buf, int bufmax)
{
	int len;

	len = strnlen(buf, bufmax);

	if(len == bufmax)
		len = 0;

	return len;
}

SSL_CTX *initialize_ctx(char *rootCA, char * keyfile, char * password, 
			ssl_pwd_data * _pwd)
{
	const SSL_METHOD *meth;
	SSL_CTX *ctx;
	unsigned int ssl_verify;

	/* Set up a SIGPIPE handler */
	signal(SIGPIPE,sigpipe_handle);

	/* Create our context*/
	meth=SSLv23_method();
	//meth = TLSv1_method();
	ctx=SSL_CTX_new(meth);

	/* Load our keys and certificates*/
	if(_strvalid(keyfile, PATH_MAX) != 0){
		printf("setup keyfile\n");
		if(!(SSL_CTX_use_certificate_chain_file(ctx,
						keyfile))){
			printf("failed to use cert chain\n");
			return 0;
		}
		
		if(password != 0 && 
			_strvalid(password, SSL_SEL_PASSWORD_MAX) &&
			_pwd != 0){
			snprintf(_pwd->data, sizeof(_pwd->data), "%s", password);
			_pwd->len = _strvalid(password, SSL_SEL_PASSWORD_MAX);

			SSL_CTX_set_default_passwd_cb_userdata(ctx, _pwd);

			SSL_CTX_set_default_passwd_cb(ctx, password_cb);
			

		}
		if(!(SSL_CTX_use_PrivateKey_file(ctx,
						keyfile,SSL_FILETYPE_PEM))){
			printf("Can't read key file\n");
			return 0;
		}
	}
	//syslog(LOG_DEBUG, "rootCA.pem path %s", _config_rootca);
	/* Load the CAs we trust*/
	if(!(SSL_CTX_load_verify_locations(ctx,
					rootCA/*"rootCA.pem"*/,0))){
		printf("Can't read CA list\n");
		return 0;
	}

	ssl_verify = SSL_VERIFY_PEER;

	SSL_CTX_set_verify(ctx, ssl_verify, verify_callback);

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
	SSL_CTX_set_verify_depth(ctx,1);
#endif

	//	SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	return ctx;
}

void destroy_ctx(SSL_CTX *ctx)
{
	SSL_CTX_free(ctx);
}


int ssl_set_fds(ssl_info *info, 
	int maxfd, fd_set *rfds, fd_set *wfds)
{
	int update_maxfd;
	int ret_maxfd;

	update_maxfd = 0;
	ret_maxfd = maxfd;

	if(info->recv.write || 
	info->connect.write || 
	info->send.write || 
	info->accept.write){
		update_maxfd = 1;
		FD_SET(info->sk, wfds);
	}

	if(info->recv.read || 
	info->connect.read || 
	info->send.read || 
	info->accept.read){
		update_maxfd = 1;
		FD_SET(info->sk, rfds);
	}

	if(update_maxfd){
		ret_maxfd = (maxfd > info->sk)?(maxfd):(info->sk);
	}

	return ret_maxfd;
}


int __set_block(int sock)
{
	int arg;

	if( (arg = fcntl(sock, F_GETFL, NULL)) < 0) {
		printf( "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));

		return -1;
	}
	arg &= (~O_NONBLOCK);
	if( fcntl(sock, F_SETFL, arg) < 0) {
		printf( "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));

		return -1;
	}

	return 0;
}

int __set_nonblock(int sock)
{
	int arg;

	if( (arg = fcntl(sock, F_GETFL, NULL)) < 0) {
		printf( "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));

		return -1;
	}
	arg |= O_NONBLOCK;
	if( fcntl(sock, F_SETFL, arg) < 0) {
		printf( "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));

		return -1;
	}

	return 0;
}

static void show_x509_err(ssl_info *info)
{
	long v_err;
	
	v_err = SSL_get_verify_result(info->ssl);
	if(v_err != X509_V_OK){
		printf(	"X509 error(%ld):%s\n",
				v_err,
				X509_verify_cert_error_string(v_err));
	}
}

static int show_x509_err_str(ssl_info *info, char * buf, int len)
{
	long v_err;
	int rlen = 0;
	
	v_err = SSL_get_verify_result(info->ssl);
	if(v_err != X509_V_OK){
		rlen = snprintf(buf, len,	
			"X509 error(%ld):%s",
			v_err,
			X509_verify_cert_error_string(v_err));
	}
	return rlen;
}


static int _do_ssl_update_wait_event(ssl_info * info, wait_event * wevent, 
		int ssl_errno, const char * func_str)
{
	switch(ssl_errno){
		case SSL_ERROR_WANT_READ:
			wevent->read = 1;
			return SSL_OPS_SELECT;
		case SSL_ERROR_WANT_WRITE:
			wevent->write = 1;
			return SSL_OPS_SELECT;
		case SSL_ERROR_ZERO_RETURN:
			printf("%s closed by remote peer\n", func_str);
			break;
		case SSL_ERROR_SYSCALL:
			printf("%s: %s\n", func_str, strerror(errno));
			break;
		default :
			printf( "%s failed(%d):%s\n",
				func_str, 
				ssl_errno, 
				ERR_reason_error_string(ERR_peek_last_error())
				);
			show_x509_err(info);
			break;

	}
	return SSL_OPS_FAIL;
}

int ssl_errno_str(ssl_info * info, int ssl_errno, char * buf, int buflen)
{
	int len;
	len = 0;
	switch(ssl_errno){
		case SSL_ERROR_WANT_READ:
			len = snprintf(buf, buflen, "SSL wait on read");
			break;
		case SSL_ERROR_WANT_WRITE:
			len = snprintf(buf, buflen, "SSL wait on write");
			break;
		case SSL_ERROR_ZERO_RETURN:
			len = snprintf(buf, buflen, 
					"SSL connection closed by peer");
			break;
		case SSL_ERROR_SYSCALL:
			len = snprintf(buf, buflen,"SSL syscall: %s", 
					strerror(errno));
			break;
		default :
			len = snprintf(buf, buflen, "SSL error(%d):%s;", 
				ssl_errno, 
				ERR_reason_error_string(ERR_peek_last_error())
				);
			ERR_clear_error();
			if(len < 0)
				break;
			len += show_x509_err_str(info, &buf[len], buflen - len);
			break;

	}
	return len;
}

#define _ssl_update_wait_event(INFO, WEVENT, ERRNO)	\
	_do_ssl_update_wait_event(INFO, &(INFO->WEVENT), ERRNO, __func__)

#define GEN_SSL_ACTION_DIRECT(ACTION, FUNC, SUCCESS_COND, ...) \
int ssl_##ACTION##_direct(ssl_info * info, ##__VA_ARGS__, int * r_errno)\
{\
	int ret;\
	int ssl_errno;\
	info->ACTION.read = 0;\
	info->ACTION.write = 0;\
	\
	ret = (FUNC);\
	\
	if(SUCCESS_COND){\
		/*printf("%s success\n", __func__);*/\
		return ret;\
	}\
	ssl_errno = SSL_get_error(info->ssl, ret);\
	\
	if(r_errno){\
		*r_errno = ssl_errno;\
	}\
	\
	return _ssl_update_wait_event(info, ACTION, ssl_errno);\
}

GEN_SSL_ACTION_DIRECT(accept, SSL_accept(info->ssl), ret == 1);
GEN_SSL_ACTION_DIRECT(connect, SSL_connect(info->ssl), ret == 1);
GEN_SSL_ACTION_DIRECT(recv, SSL_read(info->ssl, buf, len), ret > 0,void *buf, int len);
GEN_SSL_ACTION_DIRECT(send, SSL_write(info->ssl, buf, len), ret > 0, void *buf, int len);

#define GEN_SSL_ACTION_SIMPLE_TV(ACTION, FUNC, SUCCESS_COND, ...) \
int ssl_##ACTION##_simple_tv(ssl_info * info, ##__VA_ARGS__, struct timeval * tv, int *ssl_errno)\
{\
	int ret, sel;\
	fd_set rfds, wfds;\
	\
	do{\
		ret = (FUNC);\
		\
		if(SUCCESS_COND){\
			return ret;\
		}else if(ret == SSL_OPS_FAIL){\
			break;\
		}\
		\
		FD_ZERO(&rfds);\
		FD_ZERO(&wfds);\
		\
		ssl_set_fds(info, 0, &rfds, &wfds);\
		sel = select( info->sk + 1, &rfds, &wfds, 0, tv);\
		\
	}while(sel > 0);\
	\
	return ret;\
}

GEN_SSL_ACTION_SIMPLE_TV(accept, 
	ssl_accept_direct(info, ssl_errno), ret == 1);

GEN_SSL_ACTION_SIMPLE_TV(connect, 
	ssl_connect_direct(info, ssl_errno), ret == 1);

GEN_SSL_ACTION_SIMPLE_TV(recv, 
	ssl_recv_direct(info, buf, len, ssl_errno), ret > 0, void *buf, int len);

GEN_SSL_ACTION_SIMPLE_TV(send, 
	ssl_send_direct(info, buf, len, ssl_errno), ret > 0, void *buf, int len);

#define GEN_SSL_ACTION_SIMPLE(ACTION, FUNC, ...)\
int ssl_##ACTION##_simple(ssl_info * info, ##__VA_ARGS__, int to_ms, int *ssl_errno)\
{\
	struct timeval tv;\
	\
	tv.tv_sec = to_ms / 1000;\
	tv.tv_usec = (to_ms % 1000) * 1000;\
	\
	return (FUNC);\
}\

GEN_SSL_ACTION_SIMPLE(accept, ssl_accept_simple_tv(info, &tv, ssl_errno));
GEN_SSL_ACTION_SIMPLE(connect, ssl_connect_simple_tv(info, &tv, ssl_errno));
GEN_SSL_ACTION_SIMPLE(recv, ssl_recv_simple_tv(info, buf, len, &tv, ssl_errno), void *buf, int len);
GEN_SSL_ACTION_SIMPLE(send, ssl_send_simple_tv(info, buf, len, &tv, ssl_errno), void *buf, int len);


#define __handle_flag(INFO, FLAG, IOTYPE, BFIELD) \
		if((INFO)->FLAG.IOTYPE){ \
		(BFIELD) |= (invoke_ssl_##FLAG);\
		}

int ssl_handle_fds(ssl_info * info, 
			fd_set *rfds, fd_set *wfds)
{
	int ret;
	ret = 0;

	if(FD_ISSET(info->sk, rfds)){
		__handle_flag(info, connect, read, ret);
		__handle_flag(info, send, read, ret);
		__handle_flag(info, recv, read, ret);
		__handle_flag(info, accept, read, ret);
	}

	if(FD_ISSET(info->sk, wfds)){
		__handle_flag(info, connect, write, ret);
		__handle_flag(info, send, write, ret);
		__handle_flag(info, recv, write, ret);
		__handle_flag(info, accept, write, ret);
	}

	if(SSL_pending(info->ssl)){
		__handle_flag(info, recv, read, ret);
		__handle_flag(info, recv, write, ret);
	}

	return ret;
}

#define INIT_WAIT_EVENT(INFO, WEVENT)	{(INFO)->WEVENT.read = (INFO)->WEVENT.write = 0;}
ssl_info * sslinfo_alloc(void)
{
	ssl_info *info;
	info = malloc(sizeof(ssl_info));

	if(info == 0){
		printf("error allocating SSL info\n");
		return 0;
	}

	info->sk = -1;
	INIT_WAIT_EVENT(info, connect);
	INIT_WAIT_EVENT(info, recv);
	INIT_WAIT_EVENT(info, send);
	INIT_WAIT_EVENT(info, accept);

	return info;

}
