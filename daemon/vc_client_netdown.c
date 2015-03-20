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

#include "vc_client_sync.h"

extern struct vc_ops vc_pause_ops;
extern struct vc_ops vc_netup_ops;
struct vc_ops vc_netdown_ops;


int _sock_err(int sk)
{
	socklen_t len;
	int arg;

	len = sizeof(int);

	if (getsockopt(sk, SOL_SOCKET, SO_ERROR, (void*)(&arg), &len) < 0){
		printf("failed to get socket error\n");
		return -1;
	}
	if(arg){
		printf("Socket ERR: %s\n", strerror(arg));
		return -1;
	}

	return 0;
}


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

		if( __set_sockaddr_port(ptr, VC_PROTO_PORT)){
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

int vc_connect(struct vc_attr * attr)
{
//	struct addrinfo hints;
//	struct addrinfo *result, *ptr;
//	struct sockaddr	saddr_ptr;
	struct timeval tv;
	fd_set rfds;
	char * addr = attr->ip_ptr;
	int ret;
	int sklist[VC_MAX_SKNUM];
	int sknum;
//	int skarg;
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
		if(sk >= 0){
//			printf("already connected\n");
			break;
		}

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		ret = select(skmax + 1, 0, &rfds, 0, &tv);
		if(ret <= 0){
			printf("connection timeout\n");
			break;
		}

		for( i = 0; i < sknum; i++){
			if(FD_ISSET(sklist[i], &rfds)){
				if(_sock_err(sklist[i])){
					continue;
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
/*
struct vc_ops * vc_netdown_xmit(struct vc_attr * attr)
{
	printf("%s(%d)\n", __func__, __LINE__);
	return ADV_THIS;
}
*/
struct vc_ops * vc_netdown_close(struct vc_attr * attr)
{
	printf("%s(%d)\n", __func__, __LINE__);
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);

	if(attr->sk > 0){
		printf("sk = %d\n", attr->sk);
		close(attr->sk);
		attr->sk = -1;
	}
	return ADV_THIS;
}

struct vc_ops * vc_netdown_open(struct vc_attr * attr)
{
	int ret;
	struct vc_ops * ops;
	
	ret = vc_connect(attr);

	attr->sk = ret;
	
	if(ret >= 0){

		attr->sk = ret;
		
		if(is_rb_empty(attr->tx)){
			ops = &vc_netup_ops;
		}else{
			ops =  &vc_pause_ops;
		}

		return ops->open(attr);
	}else{
		attr->sk = -1;
		return ADV_THIS;
	}
}

struct vc_ops * vc_netdown_error(struct vc_attr * attr, char * str, int num)
{
	printf("%s: %s(%d)\n", __func__, str, num);
	return ADV_THIS;
}

struct vc_ops * vc_netdown_init(struct vc_attr *attr)
{
	if(attr->sk >= 0){
		printf("close socket\n");
		close(attr->sk);
		attr->sk = -1;
	}
	attr->tid = 0;
	attr->xmit_pending = 0;
	attr->pre_ops = 0;
	memset(&(attr->eki), 0, sizeof(struct adv_port_info));
	update_eki_attr(attr, stop, ADV_STOP_UNDEF);
	update_eki_attr(attr, flowctl, ADV_FLOW_UNDEF);
	update_eki_attr(attr, pair, ADV_PAIR_UNDEF);
//	printf("eki_is_open = %x\n", eki_p(attr, is_open));
	return ADV_THIS;
}
/*
static struct vc_ops * vc_netdown_ioctl(struct vc_attr * attr)
{
	if(check_attr_stat(attr, baud) == ATTR_DIFF){
		unsigned int baud = attr_p(attr, baud);
		update_eki_attr(attr, baud, baud);
	}
	if(check_attr_stat(attr, pair) == ATTR_DIFF){
		int pair = attr_p(attr, pair);
		update_eki_attr(attr, pair, pair);
	}
	if(check_attr_stat(attr, stop) == ATTR_DIFF){
		int stop = attr_p(attr, stop);
		update_eki_attr(attr, stop, stop);
	}
	if(check_attr_stat(attr, byte) == ATTR_DIFF){
		int byte = attr_p(attr, byte);
		update_eki_attr(attr, byte, byte);
	}
	if(check_attr_stat(attr, flowctl) == ATTR_DIFF){
		int flowctl = attr_p(attr, flowctl);
		update_eki_attr(attr, flowctl, flowctl);
	}
	if(check_attr_stat(attr, ms) == ATTR_DIFF){
		unsigned int ms = attr_p(attr, ms);
		update_eki_attr(attr, ms, ms);
	}

	return ADV_THIS;
}
*/

#undef ADV_THIS

struct vc_ops vc_netdown_ops = {
	.open = vc_netdown_open,
	.close = vc_netdown_close,
//	.ioctl = vc_netdown_ioctl,
//	.xmit = vc_netdown_xmit,
	.err = vc_netdown_error,
	.init = vc_netdown_init,
};
