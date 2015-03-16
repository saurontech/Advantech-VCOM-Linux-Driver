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

#include "vc_client_common.h"

extern struct vc_ops vc_netdown_ops;
static struct vc_ops vc_idle_ops;

#define ADV_THIS	(&vc_idle_ops)

static struct vc_ops * vc_idle_open(struct vc_attr * attr)
{
	struct vc_ops * nxt;

	nxt = attr->pre_ops;
	attr->pre_ops = 0;

	return nxt->open(attr);
}

static struct vc_ops * vc_idle_close(struct vc_attr * attr)
{
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	vc_common_purge(attr, 0x0000000f);

	return vc_common_close(attr, ADV_THIS);
}

static struct vc_ops * vc_idle_poll(struct vc_attr * attr)
{
	printf("%s(%d)\n", __func__, __LINE__);
	return vc_netdown_ops.init(attr);
}

static struct vc_ops * vc_idle_recv(struct vc_attr * attr, char * buf, int len)
{
	struct vc_ops * nxt;

	nxt = attr->pre_ops;
	attr->pre_ops = 0;

	return nxt->init(attr);
	//return attr->pre_ops->recv(attr, buf, len);
}

static struct vc_ops * vc_idle_error(struct vc_attr * attr, char * str, int num)
{
	printf("%s: %s(%d)\n", __func__, str, num);
	return vc_netdown_ops.init(attr);
}

static struct vc_ops * vc_idle_init(struct vc_attr * attr)
{
	char pbuf[sizeof(struct vc_proto_hdr) + 
		sizeof(struct vc_attach_param)];
	struct vc_proto_packet * packet;
	int plen;

	packet = (struct vc_proto_packet *)pbuf;

	plen = vc_pack_getwmask(packet, attr->tid, sizeof(pbuf));

	if(plen <= 0){
		printf("plen = %d\n", plen);
		return vc_netdown_ops.init(attr);
	}

	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("%s(%d)\n", __func__, __LINE__);
		close(attr->sk);
		attr->sk = -1;
		return vc_netdown_ops.init(attr);
	}
	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("%s(%d)\n", __func__, __LINE__);
		close(attr->sk);
		attr->sk = -1;
		return vc_netdown_ops.init(attr);
	}

	attr->tid++;
	
	return ADV_THIS;
}

struct vc_ops * vc_idle_jmp(struct vc_attr *attr, struct vc_ops *current)
{
	if(attr->pre_ops != 0){
		printf("loop call %s from unkown state\n", __func__);
		exit(0);
	}
	attr->pre_ops = current;
	return ADV_THIS->init(attr);
	
}


#undef ADV_THIS

static struct vc_ops vc_idle_ops = {
	.open = vc_idle_open,
	.close = vc_idle_close,
	.err = vc_idle_error,
	.init = vc_idle_init,
	.poll = vc_idle_poll,
	.recv = vc_idle_recv,
};
