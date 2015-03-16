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

#include "vc_client_idle.h"
#include "vc_client_common.h"

extern struct vc_ops vc_netdown_ops;
extern struct vc_ops vc_pause_ops;
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
	return vc_common_xmit(attr, ADV_THIS);
}

struct vc_ops * vc_netup_close(struct vc_attr * attr)
{
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	vc_common_purge(attr, 0x0000000f);

//	return vc_netdown_ops.init(attr);
	return vc_common_close(attr, ADV_THIS);
}

struct vc_ops * vc_netup_open(struct vc_attr * attr)
{
//	printf("%s(%d)\n", __func__, __LINE__);
	
	return vc_common_open(attr, ADV_THIS);
}

struct vc_ops * vc_netup_error(struct vc_attr * attr, char * str, int num)
{
	printf("%s: %s(%d)\n", __func__, str, num);

	return vc_netdown_ops.init(attr);
}

struct vc_ops * vc_netup_init(struct vc_attr * attr)
{
//	update_eki_attr(attr, is_open, 1);
	vc_buf_update(attr, VC_BUF_RX);
	vc_buf_update(attr, VC_BUF_TX);
	return ADV_THIS;
}

struct vc_ops * vc_netup_ioctl(struct vc_attr * attr)
{
	return vc_common_ioctl(attr, ADV_THIS);
}

struct vc_ops * vc_netup_recv(struct vc_attr * attr, char *buf, int len)
{
	return vc_common_recv(attr, ADV_THIS, buf, len);
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

	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("cannot send WAIT_ON_MASK\n");
		return -1;
	}

	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("failed to  send WAIT_ON_MASK\n");
		return -1;
	}
	attr->tid++;

	return 0;
}


struct vc_ops * vc_netup_pause(struct vc_attr * attr)
{
	if(_pause_queue(attr) < 0){
		printf("queue free size = 0 failed\n");
		return vc_netdown_ops.init(attr);
	}

	return vc_pause_ops.init(attr);
}


struct vc_ops * vc_netup_poll(struct vc_attr * attr)
{
	return vc_idle_jmp(attr, ADV_THIS);
}

#undef ADV_THIS

struct vc_ops vc_netup_ops = {
	.open = vc_netup_open,
	.close = vc_netup_close,
	.xmit = vc_netup_xmit,
	.err = vc_netup_error,
	.init = vc_netup_init,
	.recv= vc_netup_recv,
	.ioctl = vc_netup_ioctl,
	.poll = vc_netup_poll,
	.pause = vc_netup_pause,
	.event = vc_netup_event,
};
