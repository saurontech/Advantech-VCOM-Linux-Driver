#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "advioctl.h"
#include "advtype.h"
#include "vcom_proto.h"
#include "vcom_proto_cmd.h"
#include "vcom.h"
//#include "vcom_debug.h"
#include "vc_client_pause.h"
#include "vc_client_netup.h"

struct vc_ops vc_netdown_ops;

static int _sock_err(int sk, char * buff, int buflen, int *errstrlen)
{
	socklen_t len;
	int arg;

	len = sizeof(int);

	if (getsockopt(sk, SOL_SOCKET, SO_ERROR, (void*)(&arg), &len) < 0){
		printf("failed to get socket error\n");
		if(buff != 0 && buflen != 0 && errstrlen != 0)
			*errstrlen = snprintf(buff, buflen , "can't get SO_ERROR");
		return -1;
	}
	if(arg){
		printf("Socket ERR: %s\n", strerror(arg));
		if(buff != 0 && buflen != 0 && errstrlen != 0)
			*errstrlen = snprintf(buff, buflen , "sock err: %s", strerror(arg));
		return -1;
	}

	return 0;
}

unsigned short vc_tcp_port = VC_PROTO_PORT;

int _create_sklist(int * sklist, int maxlen, char * addr, 
	fd_set * rfds, int *retlen, int *retmax)
{
	int sknum;
	int sk;
	int skmax;
	int ret;
	struct addrinfo hints;
        struct addrinfo *result, *ptr;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

        ret = getaddrinfo(addr , NULL, &hints, &result);
	if (ret != 0){
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	skmax = sk = -1;
	sknum = 0;

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next){
		if(ptr->ai_family != AF_INET && 
			ptr->ai_family != AF_INET6){
			printf("unkown ai_family\n");
			continue;
		}

		if(sknum >= maxlen){
			break;
		}

		sklist[sknum] = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sklist[sknum] < 0){
			printf("create socket failed(%d)\n", sknum);
			continue;
		}

		vc_config_sock(sklist[sknum], VC_SKOPT_NONBLOCK, 0);

		if( __set_sockaddr_port(ptr, vc_tcp_port)){
			printf("cannot set client port\n");
			close(sklist[sknum]);
			continue;
		}

		ret = connect(sklist[sknum], ptr->ai_addr, ptr->ai_addrlen);
		if(ret < 0 && errno != EINPROGRESS){
			printf("cannot connect\n");
			close(sklist[sknum]);
			continue;
		}else if(ret == 0){
			sk = sklist[sknum];
//			printf("connected successfully on %d\n", sk);
		}

		
		if(sklist[sknum] > skmax){
			skmax = sklist[sknum];
//			printf("setting skmax to %d\n", skmax);
		}

		FD_SET(sklist[sknum], rfds);
		sknum++;

		if(sk >= 0)
			break;
	}

	freeaddrinfo(result);

	*retlen = sknum;
	*retmax = skmax;

	return sk;

}

#define CONN_TO	10
int vc_connect(struct vc_attr * attr)
{
	struct timeval tv;
	fd_set rfds;
	char * addr = attr->ip_ptr;
	int ret;
	int sklist[VC_MAX_SKNUM];
	int sknum;
	int sk;
	int skmax;
	int i;
	int max;
	
	FD_ZERO(&rfds);
	sk = _create_sklist(sklist, VC_MAX_SKNUM, addr, &rfds, &sknum, &skmax);
	if(sk < 0 && attr->ip_red > 0){
		addr = attr->ip_red;
		sk = _create_sklist(&sklist[sknum], (VC_MAX_SKNUM - sknum), 
					addr, &rfds, &ret, &max);
		sknum += ret;
		if(max > skmax){
			skmax = max;
		}
	}

	do{
		struct stk_vc * stk;
		stk = &attr->stk;

		if(sk >= 0){
//			printf("already connected\n");
			break;
		}

		tv.tv_sec = CONN_TO;
		tv.tv_usec = 0;

		ret = select(skmax + 1, 0, &rfds, 0, &tv);
		if(ret <= 0){
			
			//printf("connection timeout\n");
			_stk_log(stk, "connect timeout %d", CONN_TO);
			break;
		}

		for( i = 0; i < sknum; i++){
			if(FD_ISSET(sklist[i], &rfds)){
				char serrmsg[128];
				int retlen;
				if(_sock_err(sklist[i], 
					serrmsg, sizeof(serrmsg), 
						&retlen)){
					//continue;
					_stk_log(stk, "%s", serrmsg);
					break;
				}
				sk = sklist[i];
				break;
			}
		}
	}while(0);

	for(i = 0; i < sknum; i++){
		if(sklist[i] != sk){
//			printf("closing socket %d != %d(sk)\n", sklist[i], sk);
			close(sklist[i]);
		}
	}

	if(sk >= 0){
		int enable = 1;
//		printf("set %d to blocking mode\n", sk);
		vc_config_sock(sk, VC_SKOPT_BLOCK, 0);
		setsockopt(sk, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
	}
	

	return sk;
}


#define ADV_THIS	(&vc_netdown_ops)

struct vc_ops * vc_netdown_close(struct vc_attr * attr)
{

	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);

	if(attr->sk >= 0){
#ifdef _VCOM_SUPPORT_TLS
		if(attr->ssl){
			__set_block(attr->ssl->sk);
			SSL_shutdown(attr->ssl->ssl);
			SSL_free(attr->ssl->ssl);
		}
#endif
		close(attr->sk);
		attr->sk = -1;
	}

	

	return ADV_THIS;
}

struct vc_ops * vc_netdown_open(struct vc_attr * attr)
{
	int ret;
	struct stk_vc * stk;

	stk = &attr->stk;
	ret = vc_connect(attr);
	if(ret < 0){
		attr->sk = -1;
		return ADV_THIS;
	}

	attr->sk = ret;
#ifdef _VCOM_SUPPORT_TLS
	if(attr->ssl){
		int ssl_errno;
		ssl_info * _ssl = attr->ssl;
		_ssl->sk = attr->sk;
		_ssl->ssl = SSL_new(_ssl->ctx);
		//printf("using ctx@%p\n", _ssl->ctx);

		if(SSL_set_fd(_ssl->ssl, _ssl->sk) == 0){
			printf("SSL_set_fd failed\n");
			return stk_curnt(stk)->init(attr);
		}
		//printf("SSL_set_fd success\n");

		__set_nonblock(_ssl->sk);
		if(ssl_connect_simple(attr->ssl, 1000, &ssl_errno) < 0){			
			char ssl_errstr[256];

			printf("ssl_connect_simple_fail\n");
			ssl_errno_str(attr->ssl, ssl_errno, 
					ssl_errstr, sizeof(ssl_errstr));
			_stk_log(stk, "SSL_connect %s", ssl_errstr);
			return stk_curnt(stk)->init(attr);
		}
		//printf("ssl_connect success\n");
	}
#endif	
	if(is_rb_empty(attr->tx))
		stk_push(stk, &vc_netup_ops);
	else
		stk_push(stk, &vc_pause_ops);

	return stk_curnt(stk)->open(attr);
}

struct vc_ops * vc_netdown_error(struct vc_attr * attr, char * str, int num)
{
	printf("%s: %s(%d)\n", __func__, str, num);
	return ADV_THIS;
}

struct vc_ops * vc_netdown_init(struct vc_attr *attr)
{
	if(attr->sk >= 0){
#ifdef _VCOM_SUPPORT_TLS
		if(attr->ssl){
			__set_block(attr->ssl->sk);
			SSL_shutdown(attr->ssl->ssl);
			SSL_free(attr->ssl->ssl);
		}
#endif
		close(attr->sk);
		attr->sk = -1;
	}
	attr->tid = 0;
	attr->xmit_pending = 0;
	memset(&(attr->eki), 0, sizeof(struct adv_port_info));
	update_eki_attr(attr, stop, ADV_STOP_UNDEF);
	update_eki_attr(attr, flowctl, ADV_FLOW_UNDEF);
	update_eki_attr(attr, pair, ADV_PAIR_UNDEF);
	return ADV_THIS;
}
char * vc_netdown_name(void)
{	
	return "Net Down";
}
#undef ADV_THIS

struct vc_ops vc_netdown_ops = {
	.open = vc_netdown_open,
	.close = vc_netdown_close,
	.err = vc_netdown_error,
	.init = vc_netdown_init,
	.name = vc_netdown_name,
};
