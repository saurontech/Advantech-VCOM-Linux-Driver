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

#include "vc_client_sync.h"

extern struct vc_ops vc_netdown_ops;

struct vc_ops * vc_common_open(struct vc_attr * attr, struct vc_ops *current)
{
	int port;
	int plen;
	unsigned short devid;
	char pbuf[1024];
	struct vc_proto_packet * packet;

	packet = (struct vc_proto_packet *)pbuf;
	port = attr->port;
	devid = attr->devid;
	plen = vc_pack_open(packet, 0x001, devid, port, sizeof(pbuf));

	if(plen == 0){
		printf("cannot create open packet");
		close(attr->sk);
		attr->sk = -1;
		return vc_netdown_ops.init(attr);
	}
	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("not ready to send\n");
		close(attr->sk);
		attr->sk = -1;
		return vc_netdown_ops.init(attr);
	}
	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("send failed\n");
		close(attr->sk);
		attr->sk = -1;
		return vc_netdown_ops.init(attr);
	}

	attr->tid++;
	
	update_eki_attr(attr, is_open, 1);
	
	return vc_sync_jmp(attr, current);
}


struct vc_ops * vc_common_xmit(struct vc_attr * attr, struct vc_ops * current)
{

#define XMIT_LEN	1024
	char pbuf[XMIT_LEN + sizeof(struct vc_proto_hdr) + 
		sizeof(struct vc_attach_param)];
	struct vc_proto_packet * packet;
	int llen = get_rb_llength(attr->rx);
	int len = get_rb_length(attr->rx);
	int plen;
	char * ptr;
	
	packet = (struct vc_proto_packet *)pbuf;

	if(llen > XMIT_LEN){
		llen = XMIT_LEN;
	}
	if(len > XMIT_LEN){
		len = XMIT_LEN;
	}
	if(llen > len){
		printf("never go here !!! llen > len\n");
	}

#undef XMIT_LEN
	if(llen == len){
		ptr = &(attr->rx.mbase[get_rb_head(attr->rx)]);
		plen = vc_pack_xmit(packet, attr->tid, llen, ptr, sizeof(pbuf));
	}else{
		ptr = &(attr->rx.mbase[get_rb_head(attr->rx)]);
		memcpy(packet->attach.data.ptr, ptr, llen);
		ptr = attr->rx.mbase;
		memcpy(&(packet->attach.data.ptr[llen]), ptr, len - llen);
		plen = vc_pack_xmit(packet, attr->tid, len, 0, sizeof(pbuf));
	}
	
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

	attr->xmit_pending = len;
	attr->tid++;
	
	

	return current;
}


static int _set_ms(struct vc_attr *attr, char * pbuf, int buflen, unsigned int ms)
{
	unsigned int eki_ms;
	unsigned int uart_ms;
	int plen;
	int len;
	int step;
	struct vc_proto_packet * packet;

	eki_ms = eki_p(attr, ms);
	uart_ms = attr_p(attr, ms);
	step = 0;
	plen = 0;
	while(1){
		len = 0;
		packet = (struct vc_proto_packet *)&pbuf[plen];
		switch(step){
		case 0:
			if((eki_ms & ADV_MS_RTS) == (uart_ms & ADV_MS_RTS)){
				break;
			}
			if(uart_ms & ADV_MS_RTS){
				len = vc_pack_setrts(packet, attr->tid, buflen - plen);
			}else{
				len = vc_pack_clrrts(packet, attr->tid, buflen - plen);
			}
			break;
		case 1:
			if((eki_ms & ADV_MS_DTR) == (uart_ms & ADV_MS_DTR)){
				break;
			}
			if(uart_ms & ADV_MS_DTR){
				len = vc_pack_setdtr(packet, attr->tid, buflen - plen);
			}else{
				len = vc_pack_clrdtr(packet, attr->tid, buflen - plen);
			}
			break;
		default:
			return plen;
		}
		
		step++;
		if(len <= 0){
			continue;
		}

		plen += len;
		attr->tid++;
	}

	return 0;
}

static int vc_check_comset(struct vc_attr * attr)
{
	do{
		if(check_attr_stat(attr, pair) == ATTR_DIFF){
			break;
		}
		if(check_attr_stat(attr, stop) == ATTR_DIFF){
			break;
		}
		if(check_attr_stat(attr, byte) == ATTR_DIFF){
			break;
		}

		return 0;
	}while(0);

	return 1;
}

struct vc_ops * vc_common_ioctl(struct vc_attr * attr, struct vc_ops *current)
{
	char pbuf[1024];
	int plen;
	int loop_count;
	struct vc_proto_packet * packet;

	packet = (struct vc_proto_packet *)pbuf;

	loop_count = 15;

	do{
		plen = 0;
		if(check_attr_stat(attr, baud) == ATTR_DIFF){
			unsigned int baud;
			baud = attr_p(attr, baud);
			update_eki_attr(attr, baud, baud);
			plen = vc_pack_sbaud(packet, attr->tid, baud, sizeof(pbuf));
		}else if(vc_check_comset(attr)){
			int pair = attr_p(attr, pair);
			int stop = attr_p(attr, stop);
			int byte = attr_p(attr, byte);
			unsigned char cpair = (unsigned char)pair;
			unsigned char cstop = (unsigned char)stop;
			unsigned char cdata = (unsigned char)byte;

			update_eki_attr(attr, pair, pair);
			update_eki_attr(attr, stop, stop);
 			update_eki_attr(attr, byte, byte);
			plen = vc_pack_setlctl(packet, attr->tid, cpair, cdata, cstop, sizeof(pbuf));
		}else if(check_attr_stat(attr, flowctl) == ATTR_DIFF){
			int flowctl = attr_p(attr, flowctl);
			switch(flowctl){
			default:
			case ADV_FLOW_NONE:
				plen = vc_pack_hflow(packet, attr->tid, 
					0x01000000, 0x40000080, sizeof(pbuf));
				break;
			case ADV_FLOW_RTSCTS:
				plen = vc_pack_hflow(packet, attr->tid, 
					0x09000000, 0x80000080, sizeof(pbuf));
				break;
			case ADV_FLOW_XONXOFF:
				plen = vc_pack_hflow(packet, attr->tid, 
					0x01000000, 0x43000080, sizeof(pbuf));
				break;
			}
			update_eki_attr(attr, flowctl, flowctl);
			
		}else if(check_attr_stat(attr, ms) == ATTR_DIFF){
			unsigned int ms = attr_p(attr, ms);
			plen = _set_ms(attr, pbuf, sizeof(pbuf), ms);
			update_eki_attr(attr, ms, ms);
		}

		if(plen <= 0){
			//printf("%s(%d) plen = %d\n", __func__, __LINE__, plen);
			break;
		}

		if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
			printf("%s(%d)\n", __func__, __LINE__);
			return vc_netdown_ops.init(attr);
		}
		if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
			printf("%s(%d)\n", __func__, __LINE__);
			return vc_netdown_ops.init(attr);
		}

		attr->tid++;
	}while(--loop_count > 0);
	
	return current;
}

struct _ioctl_data{
	struct vc_attr * attr;
	struct vc_ops * this;
};

static int check_ioctl_ret(struct vc_proto_packet *pbuf,
		int buflen, void * in)
{
	
	struct _ioctl_data *data = (struct _ioctl_data *)in;
	struct vc_attr * attr = data->attr;
	struct vc_ops * current = data->this;
	struct vc_ops * ret_ops;
	unsigned int status;
	unsigned int subcmd;

	status = ntohl(pbuf->attach.param.p1);
	subcmd = ntohl(pbuf->attach.param.p2);

	switch(subcmd){
		case VCOM_IOCTL_SET_BAUD_RATE:
		case VCOM_IOCTL_SET_LINE_CONTROL:
		case VCOM_IOCTL_GET_WAIT_MASK:
		case VCOM_IOCTL_SET_HANDFLOW:
		case VCOM_IOCTL_SET_RTS:
		case VCOM_IOCTL_SET_DTR:
		case VCOM_IOCTL_CLR_RTS:
		case VCOM_IOCTL_CLR_DTR:
		case VCOM_IOCTL_PURGE:
			break;
		case VCOM_IOCTL_WAIT_ON_MASK:
		case VCOM_IOCTL_GET_MODEMSTATUS:
		case VCOM_IOCTL_SET_WAIT_MASK:
			ret_ops = vc_sync_jmp_recv(attr, current, (char *)pbuf, buflen);
			if(ret_ops != current){
				return -1;
			}
			break;
		default:
			printf("recv ioctl cmd %x status %x\n", subcmd, status);

			/*to do return*/
			break;
	}

	switch(status){
		case STATUS_SUCCESS:
			break;
		case STATUS_INVALID_PARAMETER:
			printf("cmd %x invalid param\n", subcmd);
			return -1;
		case STATUS_DEVICE_BUSY:
			printf("cmd %x device busy\n", subcmd);
			return -1;
		default:
			printf("recv ioctl cmd %x status %x\n", subcmd, status);
			return -1;
	}

	return 0;
}


struct vc_ops * vc_common_recv(struct vc_attr * attr, struct vc_ops * current,
				char *buf, int len)
{
	struct vc_proto_packet * packet;
	unsigned int cmd;

	packet = (struct vc_proto_packet *)buf;
	if (len < sizeof(packet->hdr)){
		printf("packet not long enough to form a hdr\n");
		return vc_netdown_ops.init(attr);
	}

	cmd = ntohs(packet->hdr.cmd);

	switch(cmd){
	case VCOM_CMD_CREATE:
	{
		unsigned char devid = attr->devid;
		unsigned int port = attr->port;
		if(vc_check_open(packet, devid, port, len)){
			printf("open() packet check failed\n");
			return vc_netdown_ops.init(attr);
		}
		break;
	}
	case VCOM_CMD_IOCTL:
	{
		struct _ioctl_data data = {
			.attr = attr,
			.this = current,
		};
		if(vc_check_ioctl(packet, len, &check_ioctl_ret, (void *)&data)){
			printf("ioctl() packet check failed\n");
			return vc_netdown_ops.init(attr);	
		}
		break;
	}
	case VCOM_CMD_DATAREADY:
	{
		struct ring_buf *tx = &attr->tx;
		int lroom = get_rb_lroom(attr->tx);
		int room = get_rb_room(attr->tx);
		int tail = get_rb_tail(attr->tx);
		int datalen;
		char * ptr;
		if(vc_check_recv(packet, &datalen, len)){
			printf("cmd_read check failed\n");
			return vc_netdown_ops.init(attr);
		}

		if(room < datalen){
			printf("data over flow\n");
			return vc_netdown_ops.init(attr);
		}
		ptr = &(tx->mbase[tail]);
		
		if(lroom >= datalen){
			memcpy(ptr, packet->attach.data.ptr, datalen);
		}else{
			memcpy(ptr, packet->attach.data.ptr, lroom);
			memcpy(tx->mbase, &(packet->attach.data.ptr[lroom]), datalen - lroom);
		}

		if(ioctl(attr->fd, ADVVCOM_IOCSTXTAIL, &datalen)){
			printf("move tx tail failed\n");
		}

	}	break;
	case VCOM_CMD_WRITE:{
		unsigned int s = STATUS_SUCCESS;	
		unsigned int b = STATUS_DEVICE_BUSY;
		int xmit_len = attr->xmit_pending;

		
		attr->xmit_pending = 0;
		if(vc_check_xmit(packet, xmit_len, s, len) == 0){
			if(ioctl(attr->fd, ADVVCOM_IOCSRXHEAD, &xmit_len) < 0){
				printf("move RX head failed\n");
			}
			vc_buf_update(attr, VC_BUF_RX);
		}else if(vc_check_xmit(packet, xmit_len, b, len) == 0){
			printf("write command busy\n");
		}else{
			printf("write command failed\n");
			return vc_netdown_ops.init(attr);
		}
		break;
	}
	case VCOM_CMD_QUEUEFREE:
	case VCOM_CMD_CLOSE:
		break;
	
	default:
		printf("unknown cmd %x\n", packet->hdr.cmd);		
	}
	return current;
}


void vc_common_purge(struct vc_attr * attr, unsigned int pflags)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;

	packet = (struct vc_proto_packet *)pbuf;

	plen = vc_pack_purge(packet, attr->tid, pflags, sizeof(pbuf));
	if(plen == 0){
		printf("failed to create purge packet\n");
		return;
	}

	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("cannot send purge\n");
		return;
	}
	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("failed to send purge\n");
		return;
	}

	attr->tid++;

	return;
}

struct vc_ops * vc_common_close(struct vc_attr * attr, struct vc_ops * current)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;

	packet = (struct vc_proto_packet *)pbuf;

	update_eki_attr(attr, is_open, 0);
	update_eki_attr(attr, stop, ADV_STOP_UNDEF);
	update_eki_attr(attr, flowctl, ADV_FLOW_UNDEF);
	update_eki_attr(attr, pair, ADV_PAIR_UNDEF);

	plen = vc_pack_close(packet, attr->tid, sizeof(pbuf));
	if(plen == 0){
		printf("failed to create purge packet\n");
		return vc_netdown_ops.init(attr);
	}

	if(fdcheck(attr->sk, FD_WR_RDY, 0) == 0){
		printf("cannot send purge\n");
		return vc_netdown_ops.init(attr);
	}
	if(send(attr->sk, packet, plen, MSG_NOSIGNAL) != plen){
		printf("failed to send purge\n");
		return vc_netdown_ops.init(attr);
	}

	attr->tid++;

	return current;
}
