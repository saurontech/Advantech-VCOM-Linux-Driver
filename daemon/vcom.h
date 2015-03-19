#ifndef __USER_SPACE_IF
#define __USER_SPACE_IF

#include "advvcom.h"


struct vc_attr{
	int fd;
	int sk;
	int attr_ptr;
	int xmit_pending;
	int ttyid;
	unsigned int port;
	unsigned int tid;
	unsigned short devid;
	struct vc_ops * pre_ops;
	void * mbase;
	char * ip_ptr;
	char * ip_red;
	struct adv_port_info eki;
//	struct adv_port_info proc;
	struct adv_port_info * attr;
	struct ring_buf tx;
	struct ring_buf rx;
};
#define ATTR_SAME 0
#define ATTR_DIFF 1
#define attr_p(a, b)	(((a)->attr[(a)->attr_ptr]).b)
#define eki_p(a, b)	((a)->eki.b)
//#define proc_p(a, b)	((a)->proc.b)

#define check_attr_stat(a, b)	(attr_p(a, b) == eki_p(a, b)?ATTR_SAME:ATTR_DIFF)

#define update_eki_attr(a, b, c)	do{eki_p(a, b) = c;}while(0)

struct vc_ops{
	struct vc_ops * (*open)(struct vc_attr *);
	struct vc_ops * (*close)(struct vc_attr *);
	struct vc_ops * (*ioctl)(struct vc_attr *);
	struct vc_ops * (*xmit)(struct vc_attr *);
	struct vc_ops * (*recv)(struct vc_attr *, char * buf, int len);
	struct vc_ops * (*poll)(struct vc_attr *);
	struct vc_ops * (*err)(struct vc_attr *, char * str, int num);
	struct vc_ops * (*init)(struct vc_attr *);
	struct vc_ops * (*pause)(struct vc_attr *);
	struct vc_ops * (*resume)(struct vc_attr *);
	struct vc_ops * (*event)(struct vc_attr *, struct timeval *, fd_set *r, fd_set *w, fd_set *e);
};

#define try_ops(a, b, c)	(((a)->b > 0)?(a)->b(c):a)
#define try_ops2(a, b, c, d)	(((a)->b > 0)?(a)->b(c, d):a)
#define try_ops3(a, b, c, d, e)	(((a)->b > 0)?(a)->b(c, d, e):a)
#define try_ops5(a, b, c, d, e, f, g)	(((a)->b > 0)?(a)->b(c, d, e, f, g):a)




static inline int __vc_sock_enblock(int sk, int enable)
{
	int skarg;
	skarg = fcntl(sk, F_GETFL, 0);
	if(skarg < 0){
		printf("failed to GETFL\n");
		return -1;
	}

	switch(enable){
	case 0:
		skarg |= O_NONBLOCK;
		break;
	default:
		skarg &= ~O_NONBLOCK;
		break;
	}

	if( fcntl(sk, F_SETFL, skarg) < 0){
		printf("failed to SETFL\n");
		return -1;
	}

	return 0;
}

#define VC_SKOPT_BLOCK		0
#define VC_SKOPT_NONBLOCK	1
#define VC_SKOPT_ENKALIVE	2
#define VC_SKOPT_DISKALIVE	3
static inline int vc_config_sock(int sk, int option, void * arg)
{
	int result;
#define OPT_ISMATCH(OPT, TMP)	(OPT==TMP?1:0)
	switch(option){
	case VC_SKOPT_BLOCK:
	case VC_SKOPT_NONBLOCK:
		result = __vc_sock_enblock(sk, OPT_ISMATCH(option, VC_SKOPT_BLOCK));	
		break;
	case VC_SKOPT_ENKALIVE:
	case VC_SKOPT_DISKALIVE:
		break;
	default:
		return -1;
	};
#undef OPT_ISMATCH
	return result;
}

#define VC_MAX_SKNUM	16
#define VC_PROTO_PORT	5202

static inline int __set_sockaddr_port(struct addrinfo *info, unsigned short port)
{
	struct sockaddr_in * sin;
	struct sockaddr_in6 * sin6;

	switch(info->ai_family){
	case AF_INET:
		printf("adding port %u to IPv4 address\n", port);
		sin = (struct sockaddr_in *)info->ai_addr;
		sin->sin_port = htons(port);
		break;
	case AF_INET6:
		printf("adding port %u to IPv6 address\n", port);
		sin6 = (struct sockaddr_in6 *)info->ai_addr;
		sin6->sin6_port = htons(port);
		break;
	default:
		printf("unknown address type cannot add port\n");
		return -1;	
	};

	return 0;
}



extern struct vc_ops vc_netdown_ops;
extern struct vc_ops vc_netup_ops;

#define FD_RD_RDY	1
#define FD_WR_RDY	2
#define FD_EX_RDY	4
static inline int fdcheck(int fd, int type, struct timeval * ctv)
{
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	int ret;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	if(type| FD_RD_RDY){
		FD_SET(fd, &rfds);
	}
	if(type| FD_WR_RDY){
		FD_SET(fd, &wfds);
	}
	if(ctv > 0){
		ret = select(fd + 1, &rfds, &wfds, 0, ctv);
	}else{
		ret = select(fd + 1, &rfds, &wfds, 0, &tv);
	}

	if(ret == 0)
		return 0;

	ret = 0;

	if(FD_ISSET(fd, &rfds)){
		ret |= FD_RD_RDY;
	}

	if(FD_ISSET(fd, &wfds)){
		ret |= FD_WR_RDY;
	}

	return ret;
}

#define VC_BUF_RX	0
#define VC_BUF_TX	1
#define VC_BUF_ATTR	2
static inline int vc_buf_update(struct vc_attr * port, int rb_id)
{
	struct ring_buf *buf;
	int fd = port->fd;
	int head;
	int tail;
	int cmd_head;
	int cmd_tail;

	switch(rb_id){
	case VC_BUF_RX:
		cmd_head = ADVVCOM_IOCGRXHEAD;
		cmd_tail = ADVVCOM_IOCGRXTAIL;
		buf = &port->rx;
		break;
	case VC_BUF_TX:
		cmd_head = ADVVCOM_IOCGTXHEAD;
		cmd_tail = ADVVCOM_IOCGTXTAIL;
		buf = &port->tx;
		break;
	case VC_BUF_ATTR:
		if(ioctl(fd, ADVVCOM_IOCGATTRPTR, &port->attr_ptr)){
			printf("get attrptr failed\n");
		}
		return 0;
	default:
		printf("unknown buf type\n");
		return -1;
	}

	if(ioctl(fd, cmd_head, &head) < 0){
		printf("cannot get head\n");
		return -1;
	}
	
	if(ioctl(fd, cmd_tail, &tail) < 0){
		printf("cannot get tail\n");
		return -1;
	}

	buf->head = head;
	buf->tail = tail;

	return 0;
}

static inline void vc_buf_clear(struct vc_attr * port, unsigned int clrflags)
{
	int len = 0;

	if(ioctl(port->fd, ADVVCOM_IOCSCLR, &clrflags) < 0){
		printf("couldn't set iocsclr\n");
		return;
	}
	if(clrflags & ADV_CLR_RX){
		if(ioctl(port->fd, ADVVCOM_IOCSRXHEAD, &len) < 0){
				printf("move RX head failed\n");
		}
		vc_buf_update(port, VC_BUF_RX);
	}
	if(clrflags & ADV_CLR_TX){
		vc_buf_update(port, VC_BUF_RX);
	}
}

static inline int vc_buf_setup(struct vc_attr * port, int rb_id)
{
	struct ring_buf *buf;
	int fd = port->fd;
	char * mbase = port->mbase;
	int begin;
	int size;
	int cmd_begin;
	int cmd_size;

	switch(rb_id){
	case VC_BUF_RX:
		cmd_begin = ADVVCOM_IOCGRXBEGIN;
		cmd_size = ADVVCOM_IOCGRXSIZE;
		buf = &port->rx;
		break;
	case VC_BUF_TX:
		cmd_begin = ADVVCOM_IOCGTXBEGIN;
		cmd_size = ADVVCOM_IOCGTXSIZE;
		buf = &port->tx;
		break;
	case VC_BUF_ATTR:
		cmd_begin = ADVVCOM_IOCGATTRBEGIN;
		break;
	default:
		printf("unknown buf type\n");
		return -1;
	}

	if(ioctl(fd, cmd_begin, &begin)){
		return -1;
	}

	switch(rb_id){
	case VC_BUF_RX:
	case VC_BUF_TX:
		if(ioctl(fd, cmd_size, &size)){
			printf("failed to get size\n");
			return -1;
		}
		buf->begin = begin;
		buf->size = size;
		buf->mbase = &mbase[begin];
		break;

	case VC_BUF_ATTR:
		port->attr = (struct adv_port_info *)&mbase[begin];
		break;
	}

	return vc_buf_update(port, rb_id);
}


#endif
