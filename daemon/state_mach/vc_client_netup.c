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
#include "advioctl.h"
#include "advtype.h"
#include "vcom_proto.h"
#include "vcom_proto_cmd.h"
#include "vcom_proto_ioctl.h"
#include "vcom.h"
//#include "vcom_debug.h" 
#include "vc_client_netdown.h"
#include "vc_client_pause.h"
#include "vc_client_idle.h"

#include "vc_client_common.h"

struct vc_ops vc_netup_ops;

#define ADV_THIS	(&vc_netup_ops)

static struct vc_ops * vc_netup_event(struct vc_attr * attr, 
	struct timeval * tv, fd_set * rfds, fd_set * wfds, fd_set * efds)
{
	if(attr->xmit_pending == 0){
		FD_SET(attr->fd, rfds);
	}
	
	return ADV_THIS;
}

struct vc_ops * vc_netup_xmit(struct vc_attr * attr)
{
	return vc_common_xmit(attr);
}

struct vc_ops * vc_netup_close(struct vc_attr * attr)
{
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	vc_common_purge(attr, 0x0000000f);

	return vc_common_close(attr);
}

struct vc_ops * vc_netup_open(struct vc_attr * attr)
{
//	printf("%s(%d)\n", __func__, __LINE__);
	return vc_common_open(attr);
}


struct vc_ops * vc_netup_init(struct vc_attr * attr)
{
	vc_buf_update(attr, VC_BUF_RX);
	vc_buf_update(attr, VC_BUF_TX);

	return ADV_THIS;
}

struct vc_ops * vc_netup_ioctl(struct vc_attr * attr)
{
	return vc_common_ioctl(attr);
}

struct vc_ops * vc_netup_recv(struct vc_attr * attr, char *buf, int len)
{
	return vc_common_recv(attr, buf, len);
}

static int _pause_queue(struct vc_attr * attr)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;
	
	packet = (struct vc_proto_packet *)pbuf;

	plen = vc_pack_qfsize(packet, attr->tid, 0, sizeof(pbuf));

	if(plen <= 0){
		printf("failed to create WAIT_ON_MASK\n");
		return -1;
	}

	if(vc_check_send(attr, packet, plen, "WAIT_ON_MASK") != 0){
		return -1;
	}

	attr->tid++;

	return 0;
}

struct vc_ops * vc_netup_pause(struct vc_attr * attr)
{
	struct stk_vc * stk;

	stk = &attr->stk;
	if(_pause_queue(attr) < 0){
		printf("queue free size = 0 failed\n");
		stk_excp(stk);
		return stk_curnt(stk)->init(attr);	
	}
	stk_rpls(stk, &vc_pause_ops);

	return stk_curnt(stk)->init(attr);
}

struct vc_ops * vc_netup_poll(struct vc_attr * attr)
{
	struct stk_vc * stk;
	
	stk = &attr->stk;
	stk_push(stk, &vc_idle_ops); 
	return stk_curnt(stk)->init(attr);
}

char * vc_netup_name(void)
{
	return "Net Up";
}

#undef ADV_THIS

struct vc_ops vc_netup_ops = {
	.open = vc_netup_open,
	.close = vc_netup_close,
	.xmit = vc_netup_xmit,
	.init = vc_netup_init,
	.recv= vc_netup_recv,
	.ioctl = vc_netup_ioctl,
	.poll = vc_netup_poll,
	.pause = vc_netup_pause,
	.event = vc_netup_event,
	.name = vc_netup_name,
};
