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
#include <linux/sockios.h>
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
struct vc_ops vc_pause_ops;

#define ADV_THIS	(&vc_pause_ops)

static struct vc_ops * vc_pause_xmit(struct vc_attr * attr)
{
	return vc_common_xmit(attr, ADV_THIS);
}

static struct vc_ops * vc_pause_close(struct vc_attr * attr)
{
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	vc_common_purge(attr, 0x0000000f);

//	return vc_netdown_ops.init(attr);
	return vc_common_close(attr, ADV_THIS);
}

static struct vc_ops * vc_pause_open(struct vc_attr * attr)
{
	return vc_common_open(attr, ADV_THIS);
}

static struct vc_ops * vc_pause_error(struct vc_attr * attr, char * str, int num)
{
	printf("%s: %s(%d)\n", __func__, str, num);
	return vc_netdown_ops.init(attr);
}



static struct vc_ops * vc_pause_init(struct vc_attr * attr)
{
	vc_buf_update(attr, VC_BUF_RX);
	vc_buf_update(attr, VC_BUF_TX);

	return ADV_THIS;

}

static struct vc_ops * vc_pause_ioctl(struct vc_attr * attr)
{
	return vc_common_ioctl(attr, ADV_THIS);
}

static struct vc_ops * vc_pause_recv(struct vc_attr * attr, char *buf, int len)
{
		return vc_common_recv(attr, ADV_THIS, buf, len);
}

static struct vc_ops * vc_pause_event(struct vc_attr * attr, 
	struct timeval * tv, fd_set * rfds, fd_set * wfds, fd_set * efds)
{
	int buflen;

	if(attr->sk < 0){
		printf("shouldn't go here\n");
	}

	if(ioctl(attr->sk, SIOCINQ, &buflen)){
		printf("resume: failed to get SIOCINQ\n");
		return vc_netdown_ops.init(attr);
	}
	if(buflen){
		return ADV_THIS;
	}

	if(attr->xmit_pending == 0){
		FD_SET(attr->fd, rfds);
	}
	
	FD_SET(attr->fd, wfds);

	return ADV_THIS;
}

static int _resume_queue(struct vc_attr * attr)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;
	
	packet = (struct vc_proto_packet *)pbuf;

	plen = vc_pack_qfsize(packet, attr->tid, 1024, sizeof(pbuf));

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


static struct vc_ops * vc_pause_resume(struct vc_attr * attr)
{
	int buflen;

	if(ioctl(attr->sk, SIOCOUTQ, &buflen)){
		printf("resume: failed to get SIOCOUTQ\n");
		return vc_netdown_ops.init(attr);
	}
	if(buflen){
		return ADV_THIS;
	}
	
	if(_resume_queue(attr)){
		printf("%s(%d)\n", __func__, __LINE__);
		return vc_netdown_ops.init(attr);
	}

	return vc_netup_ops.init(attr);
}


static struct vc_ops * vc_pause_poll(struct vc_attr * attr)
{
	return vc_idle_jmp(attr, ADV_THIS);
}

#undef ADV_THIS

struct vc_ops vc_pause_ops = {
	.open = vc_pause_open,
	.close = vc_pause_close,
	.xmit = vc_pause_xmit,
	.err = vc_pause_error,
	.init = vc_pause_init,
	.recv= vc_pause_recv,
	.ioctl = vc_pause_ioctl,
	.poll = vc_pause_poll,
	.event = vc_pause_event,
	.resume = vc_pause_resume,
};
