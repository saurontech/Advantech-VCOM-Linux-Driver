#ifndef __USER_SPACE_IF
#define __USER_SPACE_IF

#include <time.h>
#include <stdarg.h>

#include "advvcom.h"
#include "vcom_proto.h"

#ifdef _VCOM_SUPPORT_TLS
#include "ssl_select.h"
#endif

#define VC_PULL_TIME	10000000
#define VC_PULL_PSEC	(VC_PULL_TIME / 1000000)
#define VC_PULL_PUSEC	(VC_PULL_TIME % 1000000)

#define CONN_TO	10

#define VC_TIME_USED(a)	(VC_PULL_TIME - (a.tv_sec * 1000000 + a.tv_usec));
#define STACK_MAX 10
#define stk_full(a) ((a)->top>=STACK_MAX-1)
#define stk_empty(a) ((a)->top<0)
#define stk_bot(a) ((a)->top==0)
struct stk_vc{
	int top;
	struct vc_ops *stk_stat[STACK_MAX];
};

struct vc_attr{
	int fd;
	int sk;
	int attr_ptr;
	int xmit_pending;
	int ttyid;
#ifdef _VCOM_SUPPORT_TLS
	ssl_info *ssl;
#endif
	unsigned int port;
	unsigned int tid;
	unsigned short devid;
	struct stk_vc stk;
	void * mbase;
	char * ip_ptr;
	char * ip_red;
	struct adv_port_info eki;
	struct adv_port_info * attr;
	struct ring_buf tx;
	struct ring_buf rx;
};
#define ATTR_SAME 0
#define ATTR_DIFF 1
#define attr_p(a, b)	(((a)->attr[(a)->attr_ptr]).b)
#define eki_p(a, b)	((a)->eki.b)

#define check_attr_stat(a, b)	(attr_p(a, b) == eki_p(a, b)?ATTR_SAME:ATTR_DIFF)

#define update_eki_attr(a, b, c)	do{eki_p(a, b) = c;}while(0)

struct vc_ops{
	struct vc_ops * (*open)(struct vc_attr *);
	struct vc_ops * (*close)(struct vc_attr *);
	struct vc_ops * (*ioctl)(struct vc_attr *);
	struct vc_ops * (*xmit)(struct vc_attr *);
	struct vc_ops * (*recv)(struct vc_attr *, char * buf, int len);
	struct vc_ops * (*poll)(struct vc_attr *);
	struct vc_ops * (*init)(struct vc_attr *);
	struct vc_ops * (*pause)(struct vc_attr *);
	struct vc_ops * (*resume)(struct vc_attr *);
	struct vc_ops * (*event)(struct vc_attr *, struct timeval *, fd_set *r, fd_set *w, fd_set *e);
	char * (*name)(void);
};
#define try_ops1(a, b, c)	(((a)->b > 0)?(a)->b(c):a)
#define try_ops2(a, b, c, d)	(((a)->b > 0)?(a)->b(c, d):a)
#define try_ops3(a, b, c, d, e)	(((a)->b > 0)?(a)->b(c, d, e):a)
#define try_ops4(a, b, c, d, e, f)	(((a)->b > 0)?(a)->b(c, d, e, f):a)
#define try_ops5(a, b, c, d, e, f, g)	(((a)->b > 0)?(a)->b(c, d, e, f, g):a)

#define try_ovrld(_1, _2, _3, _4, _5, _6, _7, func,...) func
#define try_ops(args...) try_ovrld(args, try_ops5 \
					,try_ops4 \
					,try_ops3 \
					,try_ops2 \
					,try_ops1 \
					,...)(args)
static inline struct vc_ops * stk_curnt(struct stk_vc *stk);

/*
 * Vcom Monitor
 */
//#include "vcom_monitor.h"		// The normal monitor
#include "vcom_monitor_dbg.h"		// monitor with record debug log
//#include "vcom_monitor_pre_stat.h"	// monitor with record pre-state

#ifndef _VCOM_MONITOR_H
#define mon_update(...) do{}while(0)   // do nothing
#define mon_init(a) do{}while(0)        // do nothing
#define mon_update_check(...)  do{}while(0)
#endif

#define EXCP_SLEEPTIME 3
#define EXCP_RECONN_TIME 5
/*
 * Switch == 1, trigger inotify and record the log
 * Switch == 0, do nothing
*/
#define INO_PUSH_SWITCH 0
#define INO_POP_SWITCH 0
#define INO_RPLS_SWITCH 0
#define INO_RESTART_SWITCH 0

extern void * stk_mon;


static inline int _stk_log(struct stk_vc *stk, char * __form, ...)
{
	char tmp[512];
	int len;
	va_list args;

	len = 0;
	
	if(stk == 0 || stk->top < 0){
		printf("%s on empty stk\n", __func__);
		exit(0);
	}

	len += snprintf(tmp, sizeof(tmp), "(%s)", 
			stk_curnt(stk)->name());

	va_start(args, __form);
	len += vsnprintf(&tmp[len], sizeof(tmp) -len , __form, args);
	va_end(args);

	mon_update_check(stk, 0, tmp);
	
	return len;
}
/* 
 *	state machine stack
 */
/*
#define _expmsg(msg, len) \
do{ if(stk->top >= 0)	\
		snprintf(msg, len, "(%s)%s,%d", stk->stk_stat[stk->top]->name(), __func__, __LINE__);	\
	else				\
		snprintf(msg, len, "(NULL)%s,%d", __func__, __LINE__);	\
}while(0)
*/
#define _expmsg_extra(msg, len, ...) \
do{ int r; if(stk->top >= 0) {\
_Pragma("GCC diagnostic ignored \"-Wformat-zero-length\"")\
		r= snprintf(msg, len, "(%s)%s,%d:", stk->stk_stat[stk->top]->name(), \
		__func__, __LINE__);	\
        snprintf(&msg[r],len -r, "" __VA_ARGS__);\
	}else{				\
		r= snprintf(msg, len, "(NULL)%s,%d:", __func__, __LINE__ );	\
        snprintf(&msg[r],len -r, "" __VA_ARGS__);\
        }\
_Pragma("GCC diagnostic warning \"-Wformat-zero-length\"")\
}while(0)

#define stk_push(a, b)	_stk_push(a, b)
static inline int
_stk_push(struct stk_vc *stk, struct vc_ops *current)
{
	if(stk_full(stk)){
		printf("%s : stack full\n", __func__);
		return -1;
	}

	stk->top += 1;
	stk->stk_stat[stk->top] = current;
	mon_update_check(stk, INO_PUSH_SWITCH);

	return 0;
}

#define stk_pop(a)	_stk_pop(a)
static inline int
_stk_pop(struct stk_vc *stk)
{
	if(stk_empty(stk)){
		printf("%s : stack empty\n", __func__);
		return -1;
	}

	stk->top -= 1;	
	mon_update_check(stk, INO_POP_SWITCH);

	return 0;
}

//#define stk_excp(a) do{char msg[128]; _expmsg(msg, 128); _stk_excp(a, msg);}while(0)
#define stk_excp(a, ...) do{char msg[256]; _expmsg_extra(msg, sizeof(msg),\
								__VA_ARGS__);\
								_stk_excp(a, msg);}while(0)
static inline int
_stk_excp(struct stk_vc *stk, char * msg)
{
	static time_t pre_excp_t = 0;
	time_t now_excp_t;
	
	if(stk_bot(stk)){
		printf("%s : at the bottom of stack now\n", __func__);
		return -1;
	}

	printf("stack exception !! %s\n", msg);
	stk->top = 0;
	mon_update_check(stk, 1, msg);
	
	if(pre_excp_t == 0){	//mean first time excption
		time(&pre_excp_t);
	}else{
		time(&now_excp_t);
		if((now_excp_t-pre_excp_t) < EXCP_RECONN_TIME){
			sleep(EXCP_SLEEPTIME);
		}
		pre_excp_t = now_excp_t;
	}

	return 0;
}		

#define stk_rpls(a, b)	_stk_rpls(a, b)
static inline int
_stk_rpls(struct stk_vc *stk, struct vc_ops *current)
{
	stk->stk_stat[stk->top] = current;
	mon_update_check(stk, INO_RPLS_SWITCH);

	return 0;
}

static inline struct vc_ops * stk_curnt(struct stk_vc *stk)	
{
	if(stk_empty(stk)){
		printf("%s : no state in stack\n", __func__);
		exit(0);
	}

	return stk->stk_stat[stk->top];
}

#define stk_restart(a)	_stk_restart(a)
static inline int
_stk_restart(struct stk_vc *stk)
{
	if(stk_bot(stk)){
		printf("at the bottom of stack now, should not call <%s> ...\n", __func__);
		return -1;
	}
	stk->top = 0;
	mon_update_check(stk, INO_RESTART_SWITCH);

	return 0;
}

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
extern struct vc_ops vc_netdown_ops;
extern struct vc_ops vc_netup_ops;

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
#define VC_SSL_PROXY	5555

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
	if(type& FD_RD_RDY){
		FD_SET(fd, &rfds);
	}
	if(type& FD_WR_RDY){
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

static inline int vc_check_send(struct vc_attr *attr, 
		struct vc_proto_packet *packet, int plen, char * dbg_msg)
{
#ifdef _VCOM_SUPPORT_TLS
	int ssl_errno;
	struct stk_vc * stk;

	stk = &attr->stk;
	if(attr->ssl){
		if(ssl_send_simple(attr->ssl, packet, plen, 1000, &ssl_errno) != plen){
			char ssl_errstr[256];
			printf("failed to send %s over SSL\n", dbg_msg);
			ssl_errno_str(attr->ssl, ssl_errno, 
				ssl_errstr, 
				sizeof(ssl_errstr));
			_stk_log(stk, "%s;%s", dbg_msg, ssl_errstr);
			return -1;
		}
		return 0;
	}
#endif
	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("cannot send %s\n", dbg_msg);
		return -1;
	}

	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("failed to  send %s\n", dbg_msg);
		return -1;
	}

	return 0;
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
		vc_buf_update(port, VC_BUF_TX);
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
