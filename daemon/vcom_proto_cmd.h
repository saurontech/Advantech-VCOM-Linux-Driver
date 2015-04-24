#ifndef __VCOM_PROTO_CMD_H
#define __VCOM_PROTO_CMD_H

static inline int vc_pack_qfsize(struct vc_proto_packet * pbuf, unsigned int tid,
		int fsize, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
	
	cmd = VCOM_CMD_QUEUEFREE;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	p1 = 0;
	p2 = (unsigned int)fsize;

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);

	return plen;
}

static inline int vc_pack_close(struct vc_proto_packet * pbuf, unsigned int tid, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
	
	cmd = VCOM_CMD_CLOSE;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	p1 = 0;
	p2 = 0;

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);

	return plen;
}

static inline int vc_pack_xmit(struct vc_proto_packet * pbuf, unsigned int tid,
		int datalen, char *data, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
	
	cmd = VCOM_CMD_WRITE;
	plen = vc_pack_size(pbuf->attach.param) + datalen;
	len = (unsigned short)datalen;

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	p1 = (unsigned int)datalen;
	p2 = 0;

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);

	if(data > 0){
		memcpy(pbuf->attach.data.ptr, data, datalen);
	}

	return plen;
}

static inline int vc_check_xmit(struct vc_proto_packet *pbuf, int datalen, 
			unsigned int stat, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
		
	cmd = VCOM_CMD_WRITE;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);

//	p1 = STATUS_SUCCESS;
	p1 = stat;
	p2 = datalen;
	
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return -1;
	}
	if(vc_check_hdr(&pbuf->hdr, cmd, len)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if(pbuf->attach.param.p1 != htonl(p1)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if(pbuf->attach.param.p2 != htonl(p2)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}
	

	return 0;
}

static inline int vc_check_recv(struct vc_proto_packet *pbuf, int *datalen, int buflen)
{
	int alen;
	int plen;
	int status;

	plen = (int)ntohs(pbuf->hdr.len);
	status = (int)ntohl(pbuf->attach.data.p1);
	alen = (int)ntohl(pbuf->attach.data.p2);

	if(status != STATUS_SUCCESS){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}
	if(plen != alen){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}
	
	*datalen = alen;

	return 0;
}


static inline int vc_pack_open(struct vc_proto_packet * pbuf, unsigned int tid,
		unsigned short dev, unsigned int port, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
	
	cmd = VCOM_CMD_CREATE;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);

	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return 0;
	}
	
	vc_init_hdr(&pbuf->hdr, tid, cmd, len);

	p1 = dev;
	p1 = p1 << 16;
	p2 = port;

	pbuf->attach.param.p1 = htonl(p1);
	pbuf->attach.param.p2 = htonl(p2);

	return plen;
}

static inline int vc_check_open(struct vc_proto_packet * pbuf,
		unsigned short dev, unsigned int port, int buflen)
{
	unsigned short len;
	unsigned int p1;
	unsigned int p2;
	unsigned int cmd;
	int plen;
		
	cmd = VCOM_CMD_CREATE;
	plen = vc_pack_size(pbuf->attach.param);
	len = vc_ext_size(pbuf->attach.param);

	p1 = STATUS_SUCCESS;
	p2 = port;
	
	if(plen > buflen ){
		printf("%s(%d) wrong length\n", __func__, __LINE__);
		return -1;
	}
	if(vc_check_hdr(&pbuf->hdr, cmd, len)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if(pbuf->attach.param.p1 != htonl(p1)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if(pbuf->attach.param.p2 != htonl(p2)){
		printf("%s(%d)\n", __func__, __LINE__);
		return -1;
	}
	
	return 0;
}

#endif
