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
#include "advlist.h"

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

//const unsigned short vc_tcp_port = VC_PROTO_PORT;

typedef struct	{
	int sk;
	struct list_head list;
}_client_info;

int _create_sklist(struct list_head *sklist, char * addr, char * port)
{
	int sk;
	int ret;
	struct addrinfo hints;
        struct addrinfo *result, *ptr;
	_client_info * sk_info;
	

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

        ret = getaddrinfo(addr , port, &hints, &result);
	if (ret != 0){
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}


	for (ptr = result; ptr != NULL; ptr = ptr->ai_next){
		if(ptr->ai_family != AF_INET && 
			ptr->ai_family != AF_INET6){
			printf("unkown ai_family\n");
			continue;
		}

		sk = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sk < 0){
			printf("create socket failed(%d)\n", sk);
			continue;
		}
		printf("create socket(%d)\n", sk);
		vc_config_sock(sk, VC_SKOPT_NONBLOCK, 0);

/*		if( __set_sockaddr_port(ptr, vc_tcp_port)){
			printf("cannot set client port\n");
			close(sk);
			continue;
		}*/

		ret = connect(sk, ptr->ai_addr, ptr->ai_addrlen);
		if(ret < 0 && errno != EINPROGRESS){
			printf("cannot connect\n");
			close(sk);
			continue;
		}else if(ret == 0){
			printf("connected successfully on %d\n", sk);
			break;
		}
		sk_info = malloc(sizeof(_client_info));
		printf("adding socket(%d) to waitlist ptr %p\n", sk, sk_info);
		sk_info->sk = sk;
		list_add_tail(&sk_info->list, sklist);
		sk = -1;
	}

	freeaddrinfo(result);


	return sk;

}

#define CONN_TO	10
int vc_connect(struct vc_attr * attr)
{
	struct timeval tv;
	fd_set rfds;
	char * addr = attr->ip_ptr;
	char service[16];
	int ret;
	int sk;
	LIST_HEAD(clients);
	struct list_head * list_ptr;
	struct list_head * next;
	_client_info * cli_ptr;
	
	snprintf(service, sizeof(service), "%hu",VC_PROTO_PORT);
	sk = _create_sklist(&clients, addr, service);
	if(sk < 0 && attr->ip_red > 0){
		addr = attr->ip_red;
		sk = _create_sklist(&clients, addr, "5202");
	}

	tv.tv_sec = CONN_TO;
	tv.tv_usec = 0;

	do{
		struct stk_vc * stk;
		int skmax;
		stk = &attr->stk;

		if(sk >= 0){
//			printf("already connected\n");
			break;
		}

		skmax = -1;
		FD_ZERO(&rfds);
		list_for_each(list_ptr, &clients){
			cli_ptr = container_of(list_ptr, _client_info, list);
			printf("checking socket(%d) at waitlist %p \n", cli_ptr->sk, cli_ptr);
			FD_SET(cli_ptr->sk, &rfds);
			if(cli_ptr->sk > skmax){
				skmax = cli_ptr->sk;
			}
		}

		ret = select(skmax + 1, 0, &rfds, 0, &tv);

		if(ret <= 0){
			//printf("connection timeout\n");
			_stk_log(stk, "connect timeout %d", CONN_TO);
			break;
		}

		list_for_each_safe(list_ptr, next, &clients){
			cli_ptr = container_of(list_ptr, _client_info, list);
			if(FD_ISSET(cli_ptr->sk, &rfds)){
				char serrmsg[128];
				int retlen;
				if(_sock_err(cli_ptr->sk, 
					serrmsg, sizeof(serrmsg), 
					&retlen)){

					close(cli_ptr->sk);
					list_del(&cli_ptr->list);
					free(cli_ptr);
					_stk_log(stk, "connect():%s", serrmsg);

					continue;
				}

				sk = cli_ptr->sk;
				list_del(&cli_ptr->list);
				free(cli_ptr);
				break;
			}
		}

	}while(1);

	list_for_each_safe(list_ptr, next, &clients){
		cli_ptr = container_of(list_ptr, _client_info, list);
		close(cli_ptr->sk);
		list_del(&cli_ptr->list);
		free(cli_ptr);
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
