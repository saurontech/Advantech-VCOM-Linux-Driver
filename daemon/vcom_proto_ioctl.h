#ifndef __VCOM_PROTO_IOCTL_H
#define __VCOM_PROTO_IOCTL_H

#ifndef _BSD_SOURCE
//this is needed to solve indianess issues
// htole32
#define _BSD_SOURCE
#endif

#include <endian.h>

static inline int vc_pack_purge(struct vc_proto_packet * pbuf, unsigned int tid,
		unsigned int pflags, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.uint32);
	len = vc_ext_size(pbuf->attach.uint32);

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_PURGE;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	pbuf->attach.uint32.uint = htole32(pflags); //little endian

	return plen;
}

typedef int (*do_check)(struct vc_proto_packet *pbuf, int buflen, void *in);

static inline int vc_check_ioctl(struct vc_proto_packet * pbuf, int buflen, 
				do_check func , void * in)
{
	int plen;
		
	plen = vc_pack_size(pbuf->attach.param);
		
	if(plen > buflen ){
		printf("%s(%d) wrong length %d should be %d\n", __func__, __LINE__, plen, buflen);
		return -1;
	}

	if(func > 0)
		return func(pbuf, buflen, in);

	return 0;
}

static inline int vc_pack_sbaud(struct vc_proto_packet * pbuf, unsigned int tid,
		unsigned int baud, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.uint32);
	len = vc_ext_size(pbuf->attach.uint32);

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_SET_BAUD_RATE;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	pbuf->attach.uint32.uint = htole32(baud); //little endian

	return plen;
}

static inline int _pack_ioctl_param(struct vc_proto_packet *pbuf, 
			unsigned int tid, unsigned int subcmd, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = subcmd;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	return plen;

}

static inline int vc_pack_womask(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_WAIT_ON_MASK, buflen);
}

static inline int vc_pack_setrts(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_SET_RTS, buflen);
}

static inline int vc_pack_setdtr(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_SET_DTR, buflen);
}

static inline int vc_pack_clrrts(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_CLR_RTS, buflen);
}

static inline int vc_pack_clrdtr(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_CLR_DTR, buflen);
}


static inline int vc_pack_getmodem(struct vc_proto_packet *pbuf, unsigned int tid, int buflen)
{
	return _pack_ioctl_param(pbuf, tid, VCOM_IOCTL_GET_MODEMSTATUS, buflen);
}

static inline int vc_pack_hflow(struct vc_proto_packet *pbuf, unsigned int tid, 
			int chflow, int flowrep, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.hflow);
	len = vc_ext_size(pbuf->attach.hflow);
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_SET_HANDFLOW;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);

	pbuf->attach.hflow.chflow = htonl(chflow);
	pbuf->attach.hflow.flowrep = htonl(flowrep);
	
	pbuf->attach.hflow.xonlimit = htonl(1024);
	pbuf->attach.hflow.xonlimit = htonl(512);

	return plen;
}


static inline int vc_pack_setwmask(struct vc_proto_packet * pbuf, unsigned int tid,
		unsigned int mask, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.uint32);
	len = vc_ext_size(pbuf->attach.uint32);
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_SET_WAIT_MASK;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	pbuf->attach.uint32.uint = htonl(mask);

	return plen;
}

static inline int vc_pack_getwmask(struct vc_proto_packet * pbuf, unsigned int tid, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_GET_WAIT_MASK;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	return plen;
}


static inline int vc_pack_setlctl(struct vc_proto_packet * pbuf, unsigned int tid, 
		 unsigned char pair, unsigned char dbit, unsigned char sbit, int buflen)
{
	unsigned short len;
	unsigned int cmd;
	unsigned int p1;
	unsigned int p2;
	int plen;

	cmd = VCOM_CMD_IOCTL;
	plen = vc_pack_size(pbuf->attach.lctrl);
	len = vc_ext_size(pbuf->attach.lctrl);
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	p1 = VCOM_IOCTL_SET_LINE_CONTROL;
	p2 = 0;
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);
	
	pbuf->attach.lctrl.pair = pair;
	pbuf->attach.lctrl.dbit = dbit;
	pbuf->attach.lctrl.sbit = sbit;

	return plen;
}

#endif
