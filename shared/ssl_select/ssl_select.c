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

#include "ssl_select.h"

static char * pass;

static void sigpipe_handle(int x){
}

static int password_cb(char *buf,int num,
		int rwflag,void *userdata)
{
	if(num<strlen(pass)+1)
		return(0);

	strcpy(buf,pass);
	return(strlen(pass));
}

static int verify_callback (int ok, X509_STORE_CTX *store)
{
	char data[256];

	X509 *cert = X509_STORE_CTX_get_current_cert(store);

	if (!ok)
	{
		int  depth = X509_STORE_CTX_get_error_depth(store);
		int  err = X509_STORE_CTX_get_error(store);

		printf("-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		printf("  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
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

SSL_CTX *initialize_ctx(char *rootCA, char * keyfile, char * password)
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
	if(strlen(keyfile) != 0){
		printf("setup keyfile\n");
		if(!(SSL_CTX_use_certificate_chain_file(ctx,
						keyfile))){
			printf("failed to use cert chain\n");
			return 0;
		}

		if(password != 0 && strlen(password) != 0){
			//printf("found password\n");
			pass=password;
			SSL_CTX_set_default_passwd_cb(ctx,
					password_cb);

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
				ERR_reason_error_string(ERR_get_error())
				);
			show_x509_err(info);
			break;

	}
	return SSL_OPS_FAIL;
}

#define _ssl_update_wait_event(INFO, WEVENT, ERRNO)	\
	_do_ssl_update_wait_event(INFO, &(INFO->WEVENT), ERRNO, __func__)

int ssl_connect_direct(ssl_info * info)
{
	int ret;
	int ssl_errno;
	info->connect.read = 0;
	info->connect.write = 0;

	printf("%s(%d)\n", __func__, __LINE__);

	ret = SSL_connect(info->ssl);
	if(ret > 0)
		return ret;

	ssl_errno = SSL_get_error(info->ssl, ret);
	
	return _ssl_update_wait_event(info, connect, ssl_errno);
}

int ssl_connect_simple(ssl_info * info, int to_ms)
{
	int ret;
	int conn;
	struct timeval tv;
	fd_set rfds, wfds;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	tv.tv_sec = to_ms / 1000;
	tv.tv_usec = (to_ms % 1000) * 1000;
	do{
		conn = ssl_connect_direct(info);
		if(conn > 0){
			return 0;

		}else if(conn == SSL_OPS_FAIL){
			break;
		}
		ssl_set_fds(info, 0, &rfds, &wfds);
		ret = select( info->sk + 1, &rfds, &wfds, 0, &tv);
	
	}while(ret > 0);

	return -1;
}


int ssl_send_direct(ssl_info * info, char *buf, int len)
{
	int wlen;
	int ssl_errno;

	info->send.write = 0;
	info->send.read = 0;

	wlen = SSL_write(info->ssl, buf, len);
	if(wlen > 0){
		return wlen;
	}

	ssl_errno = SSL_get_error(info->ssl, wlen);

	return _ssl_update_wait_event(info, send, ssl_errno);
}

int ssl_recv_direct(ssl_info * info, char * buf, int len)
{
	int rlen;
	int ssl_errno;

	info->recv.write = 0;
	info->recv.read = 0;
	rlen = SSL_read(info->ssl, buf, len);
	if(rlen > 0){
		return rlen;
	}

	ssl_errno = SSL_get_error(info->ssl, rlen);

	return _ssl_update_wait_event(info, recv, ssl_errno);
}

int ssl_send_simple(ssl_info * info, void * buf, int len, int to_ms)
{
	int slen;
	int ret;
	struct timeval tv;
	fd_set rfds, wfds;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	tv.tv_sec = to_ms / 1000;
	tv.tv_usec = (to_ms % 1000) * 1000;

	do{
		slen = ssl_send_direct(info, buf, len);
		if(slen > 0){
			return slen;

		}else if(slen == SSL_OPS_FAIL){
			break;
		}
		ssl_set_fds(info, 0, &rfds, &wfds);
		ret = select( info->sk + 1, &rfds, &wfds, 0, &tv);
	
	}while(ret > 0);

	return 0;
}

int ssl_recv_simple(ssl_info * info, void * buf, int len, int to_ms)
{
	int slen;
	int ret;
	struct timeval tv;
	fd_set rfds, wfds;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	tv.tv_sec = to_ms / 1000;
	tv.tv_usec = (to_ms % 1000) * 1000;

	do{
		slen = ssl_recv_direct(info, buf, len);
		if(slen > 0){
			return slen;

		}else if(slen == SSL_OPS_FAIL){
			break;
		}
		ssl_set_fds(info, 0, &rfds, &wfds);
		ret = select( info->sk + 1, &rfds, &wfds, 0, &tv);
	
	}while(ret > 0);

	return 0;
}



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
	}

	if(FD_ISSET(info->sk, wfds)){
		__handle_flag(info, connect, write, ret);
		__handle_flag(info, send, write, ret);
		__handle_flag(info, recv, write, ret);
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
	}

	info->sk = -1;
	INIT_WAIT_EVENT(info, connect);
	INIT_WAIT_EVENT(info, recv);
	INIT_WAIT_EVENT(info, send);
	INIT_WAIT_EVENT(info, accept);

	return info;

}
