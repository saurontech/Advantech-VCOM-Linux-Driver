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
#include "vc_client_netdown.h"
//#include "vcom_debug.h"

struct vc_ops vc_sync_ops;

#define ADV_THIS	(&vc_sync_ops)

struct vc_ops * vc_sync_open(struct vc_attr * attr)
{
	printf("%s(%d)\n", __func__, __LINE__);
	exit(0); //this function was designed never to be called
	return ADV_THIS;
}

struct vc_ops * vc_sync_close(struct vc_attr * attr)
{
	struct stk_vc * stk;

	stk = &attr->stk;
	printf("%s(%d)\n", __func__, __LINE__);
	vc_buf_clear(attr, ADV_CLR_RX|ADV_CLR_TX);
	stk_excp(stk);
	exit(0); //this function was designed never to be called

	return stk_curnt(stk)->init(attr);
}


static int _sync_ms(struct vc_attr *attr, unsigned int uart_ms)
{
	char pbuf[1024];
	char * dbg_msg;
	int plen;
	int step;
	int buflen;
	struct vc_proto_packet * packet;

	buflen = sizeof(pbuf);
	step = 0;
	dbg_msg = "NONE";
	while(1){
		plen = 0;
		packet = (struct vc_proto_packet *)pbuf;
		switch(step){
		case 0:
			dbg_msg = "int RTS";
			if(uart_ms & ADV_MS_RTS){
				plen = vc_pack_setrts(packet, attr->tid, buflen);
			}else{
				plen = vc_pack_clrrts(packet, attr->tid, buflen);
			}
			break;
		case 1:
			dbg_msg = "init DTR";
			if(uart_ms & ADV_MS_DTR){
				plen = vc_pack_setdtr(packet, attr->tid, buflen);
			}else{
				plen = vc_pack_clrdtr(packet, attr->tid, buflen);
			}
			break;
		default:
			return 0;
		}

		if(plen <= 0){
			printf("failed to create %s\n", dbg_msg);
			return -1;
		}

		if(vc_check_send(attr, packet, plen, dbg_msg) != 0){
			return -1;
		}	
	
		step++;
		attr->tid++;
	}

	printf("%s(%d) should never go here\n", __func__, __LINE__);
	return -1;
}


static int _sync_event(struct vc_attr * attr)
{
	char pbuf[1024];
	char * dbg_msg;
	int plen;
	int init_step;
	struct vc_proto_packet * packet;
	
	packet = (struct vc_proto_packet *)pbuf;

	init_step = 0;
	dbg_msg = "(NULL)";

	while(1){

		switch(init_step){
		case 0:
			plen = vc_pack_setwmask(packet, attr->tid, 0x38010000, sizeof(pbuf));
			dbg_msg = "SET_WAIT_MASK";
			break;
		case 1:
			plen = vc_pack_getmodem(packet, attr->tid, sizeof(pbuf));
			dbg_msg = "GET_MODEMSTATUS";
			break;
		default:
			//dbg_msg = "UNKNOWN";
			return 0;
		}	

		if(plen <= 0){
			printf("failed to create %s\n", dbg_msg);
			break;
		}

		if(vc_check_send(attr, packet, plen, dbg_msg) != 0){
			break;
		}

		attr->tid++;
		init_step++;
	}

	return -1;

}

static int _reg_sync_event(struct vc_attr * attr)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;
	
	packet = (struct vc_proto_packet *)pbuf;

	plen = vc_pack_womask(packet, attr->tid, sizeof(pbuf));

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

static int _sync_queue(struct vc_attr * attr)
{
	char pbuf[1024];
	int plen;
	struct vc_proto_packet * packet;
	int qfsize;
	
	packet = (struct vc_proto_packet *)pbuf;

	if(is_rb_empty(attr->tx)){
		qfsize = 1024;
	}else{
		qfsize = 0;
	}

	plen = vc_pack_qfsize(packet, attr->tid, qfsize, sizeof(pbuf));

	if(plen <= 0){
		printf("failed to create QUEUE_FREE\n");
		return -1;
	}

	
	if(vc_check_send(attr, packet, plen, "QUEUE_FREE") != 0){
		return -1;
	}

	attr->tid++;

	return 0;
}


struct vc_ops * vc_sync_init(struct vc_attr * attr)
{
	struct stk_vc * stk;
	unsigned int uart_ms;

	stk = &attr->stk;
	uart_ms = attr_p(attr, ms);

	if(_sync_ms(attr, uart_ms)){
		printf("failed to sync ms\n");
		stk_excp(stk);
		return stk_curnt(stk)->init(attr);
	}
	update_eki_attr(attr, ms, uart_ms);

	if(_sync_event(attr)){
		printf("failed to sync event\n");
		stk_excp(stk);

        return stk_curnt(stk)->init(attr); 
	}

	if(_sync_queue(attr)){
		printf("failed to sync queue\n");
		stk_excp(stk);

        return stk_curnt(stk)->init(attr);  
	}
	stk_pop(stk);
	return stk_curnt(stk)->init(attr);
}

struct vc_ops * vc_sync_recv(struct vc_attr * attr, char *buf, int len)
{
	struct vc_proto_packet * pbuf;
	struct stk_vc * stk;
	unsigned int subcmd;
	unsigned int tid;
	unsigned int status;
	
	pbuf = (struct vc_proto_packet *)buf;
	stk = &attr->stk;
	tid = ntohl(pbuf->hdr.tid);
	subcmd = ntohl(pbuf->attach.param.p2);
	status = ntohl(pbuf->attach.param.p1);

	switch(subcmd){
		case VCOM_IOCTL_WAIT_ON_MASK:
			if(_sync_event(attr)){
				stk_excp(stk);
        			return stk_curnt(stk)->init(attr);  
			}
			break;
		case VCOM_IOCTL_SET_WAIT_MASK:
			if(_reg_sync_event(attr)){
				stk_excp(stk);
        			return stk_curnt(stk)->init(attr);  
			}
			break;
		case VCOM_IOCTL_GET_MODEMSTATUS:
		{
			unsigned int modem;
			modem = le32toh(pbuf->attach.uint32.uint);
			if(ioctl(attr->fd, ADVVCOM_IOCSMCTRL, &modem)){
				printf("ioctl(mctrl) failed\n");
				stk_excp(stk);
        			return stk_curnt(stk)->init(attr);  
			}
			break;	
		}
		default:
			printf("recv ioctl tid %u cmd %x status %x\n", tid, subcmd, status);

	}
	stk_pop(stk);

	return stk_curnt(stk)->init(attr);
}

char * vc_sync_name(void)
{
	return "Sync";
}
#undef ADV_THIS

struct vc_ops vc_sync_ops = {
	.init = vc_sync_init,
	.recv = vc_sync_recv,
	.open = vc_sync_open,
	.close = vc_sync_close,
	.name = vc_sync_name,
};
