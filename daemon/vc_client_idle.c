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
#include "vcom_proto_ioctl.h"
#include "vcom.h"
//#include "vcom_debug.h"
#include "vc_client_netdown.h"

#include "vc_client_common.h"

struct vc_ops vc_idle_ops;

#define ADV_THIS	(&vc_idle_ops)

struct vc_ops * vc_idle_open(struct vc_attr * attr)
{
	struct stk_vc * stk;

	stk = &attr->stk;
	stk_pop(stk);
	
	return stk_curnt(stk)->open(attr); 
}

struct vc_ops * vc_idle_close(struct vc_attr * attr)
{
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	vc_common_purge(attr, 0x0000000f);

	return vc_common_close(attr);
}
struct vc_ops * vc_idle_poll(struct vc_attr * attr)
{
	struct stk_vc * stk;

	stk = &attr->stk;
	printf("%s(%d)\n", __func__, __LINE__);
	stk_restart(stk);

	return stk_curnt(stk)->init(attr);
}

struct vc_ops * vc_idle_recv(struct vc_attr * attr, char * buf, int len)
{
	struct stk_vc * stk;

	stk = &attr->stk;	
	stk_pop(stk);
	return stk_curnt(stk)->init(attr);
}

struct vc_ops * vc_idle_error(struct vc_attr * attr, char * str, int num)
{
	struct stk_vc * stk;

	stk = &attr->stk;
	printf("%s: %s(%d)\n", __func__, str, num);
	stk_excp(stk);

	return stk_curnt(stk)->init(attr);
}

struct vc_ops * vc_idle_init(struct vc_attr * attr)
{
	char pbuf[sizeof(struct vc_proto_hdr) + 
		sizeof(struct vc_attach_param)];
	struct vc_proto_packet * packet;
	struct stk_vc * stk;
	int plen;

	stk = &attr->stk;
	packet = (struct vc_proto_packet *)pbuf;
	
	plen = vc_pack_getwmask(packet, attr->tid, sizeof(pbuf));

	if(plen <= 0){
		printf("plen = %d\n", plen);
		stk_excp(stk);
		return stk_curnt(stk)->init(attr);
	}

	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("%s(%d)\n", __func__, __LINE__);
		close(attr->sk);
		attr->sk = -1;
		stk_excp(stk);                                                                         
        return stk_curnt(stk)->init(attr);
	}
	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("%s(%d)\n", __func__, __LINE__);
		close(attr->sk);
		attr->sk = -1;
		stk_excp(stk);                                                                         
        return stk_curnt(stk)->init(attr);
	}
	attr->tid++;
	
	return ADV_THIS;
}
char * vc_idle_name(void)
{
	return "Idle";
}
#undef ADV_THIS
struct vc_ops vc_idle_ops = {
	.open = vc_idle_open,
	.close = vc_idle_close,
	.err = vc_idle_error,
	.init = vc_idle_init,
	.poll = vc_idle_poll,
	.recv = vc_idle_recv,
	.name = vc_idle_name,
};
