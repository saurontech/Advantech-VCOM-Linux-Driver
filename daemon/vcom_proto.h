#ifndef __VCOM_PROTO_H
#define __VCOM_PROTO_H
struct vc_attach_param{
	unsigned int p1;
	unsigned int p2;
}__attribute__((packed));

struct vc_attach_data{
	unsigned int p1;
	unsigned int p2;
	char ptr[4];
}__attribute__((packed));

struct vc_attach_uint{
	unsigned int p1;
	unsigned int p2;
	unsigned int uint;
}__attribute__((packed));

struct vc_attach_hflow{
	unsigned int p1;
	unsigned int p2;
	unsigned int chflow;
	unsigned int flowrep;
	unsigned int xonlimit;
	unsigned int xofflimit;
}__attribute__((packed));

struct vc_attach_lctrl{
	unsigned int p1;
	unsigned int p2;
	unsigned char sbit;
	unsigned char pair;
	unsigned char dbit;
//	unsigned char resv;
}__attribute__((packed));


#define VCOM_MAGIC	(htonl(0x4d414441))

struct vc_proto_hdr{
	unsigned int	pid;
	unsigned int	tid;
	unsigned short	cmd;
	unsigned short	len;
}__attribute__((packed));


#define vc_pack_size(a)		(sizeof(struct vc_proto_hdr) + sizeof(a))
#define vc_ext_size(a)	(sizeof(a) - sizeof(struct vc_attach_param))

struct vc_proto_packet{
	struct vc_proto_hdr hdr;
	union{
		struct vc_attach_param param;
		struct vc_attach_data data;
		struct vc_attach_uint uint32;
		struct vc_attach_hflow hflow;
		struct vc_attach_lctrl lctrl;
	}attach;
}__attribute__((packed));

#define pack_att(a, b)		((a)->attach.b)


enum VCOM_RESP{
	STATUS_SUCCESS			= 0x00000000,
	STATUS_INVALID_PARAMETER	= 0xC000000D,
	STATUS_DEVICE_BUSY		= 0x80000011,
};

enum VCOM_CMD
{
	VCOM_CMD_CREATE			= 0x0000,
	VCOM_CMD_CLOSE			= 0x0002,
	VCOM_CMD_WRITE			= 0x0004,
	VCOM_CMD_IOCTL			= 0x000e,
	VCOM_CMD_CLEANUP		= 0x0012,
	VCOM_CMD_QUEUEFREE		= 0x0900,
	VCOM_CMD_DATAREADY		= 0x0800,
	VCOM_CMD_SHUTDOWN		= 0x0010
};

enum VCOM_IOCTL
{
	VCOM_IOCTL_SET_BAUD_RATE	= 0x01,
	VCOM_IOCTL_SET_LINE_CONTROL	= 0x03,
	VCOM_IOCTL_SET_BREAK_ON		= 0x04,
	VCOM_IOCTL_SET_BREAK_OFF	= 0x05,
	VCOM_IOCTL_IMMEDIATE_CHAR	= 0x06,
	VCOM_IOCTL_SET_TIMEOUTS		= 0x07,
	VCOM_IOCTL_GET_TIMEOUTS		= 0x08,
	VCOM_IOCTL_SET_DTR		= 0x09,
	VCOM_IOCTL_CLR_DTR		= 0x0a,
	VCOM_IOCTL_SET_RTS		= 0x0c,
	VCOM_IOCTL_CLR_RTS		= 0x0d,
	VCOM_IOCTL_SET_XOFF		= 0x0e,
	VCOM_IOCTL_SET_XON		= 0x0f,
	VCOM_IOCTL_GET_WAIT_MASK	= 0x10,
	VCOM_IOCTL_SET_WAIT_MASK	= 0x11,
	VCOM_IOCTL_WAIT_ON_MASK 	= 0x12,
	VCOM_IOCTL_PURGE		= 0x13,
	VCOM_IOCTL_GET_BAUD_RATE	= 0x14,
	VCOM_IOCTL_GET_LINE_CONTROL	= 0x15,
	VCOM_IOCTL_GET_CHARS		= 0x16,
	VCOM_IOCTL_SET_CHARS		= 0x17,
	VCOM_IOCTL_GET_HANDFLOW		= 0x18,
	VCOM_IOCTL_SET_HANDFLOW		= 0x19,
	VCOM_IOCTL_GET_MODEMSTATUS	= 0x1a,
	VCOM_IOCTL_GET_COMMSTATUS	= 0x1b,
	VCOM_IOCTL_GET_PROPERTIES	= 0x1d,
	VCOM_IOCTL_GET_DTRRTS		= 0x1e,
	VCOM_IOCTL_GET_STATS		= 0x23,
	VCOM_IOCTL_CLEAR_STATS		= 0x24,
};

static inline void vc_init_hdr(struct vc_proto_hdr * hdr, unsigned int tid, 
					unsigned short cmd, unsigned short len)
{
	hdr->pid = VCOM_MAGIC;
	hdr->tid = htonl(tid);
	hdr->cmd = htons(cmd);
	hdr->len = htons(len);
}


static inline int vc_check_hdr(struct vc_proto_hdr * hdr, unsigned short cmd, unsigned short len)
{
	
	if(hdr->pid != VCOM_MAGIC){
		printf("wrong pid: %x instead\n", hdr->pid);
		return -1;
	}
	if(hdr->cmd != htons(cmd)){
		printf("wrong cmd: %x should be %x\n", ntohs(hdr->cmd), cmd);
		return -1;
	}
	if(hdr->len != htons(len)){
		printf("wrong len: %x should be %x\n", ntohs(hdr->len), len);
		return -1;
	}

	return 0;
}
#endif
