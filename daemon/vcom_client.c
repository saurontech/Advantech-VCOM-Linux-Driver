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
#include <time.h>
#include <netdb.h>
#include "advioctl.h"
#include "advtype.h"
#include "vcom_proto.h"
#include "vcom.h"
//#include "vcom_debug.h"

#define RBUF_SIZE	4096
//static int log_fd = -1;
extern void * stk_mon; 
struct vc_ops * vc_recv_desp(struct vc_attr *port)
{
	int len;
	int hdr_len;
	int packet_len;
	unsigned short cmd;
	static char buf[RBUF_SIZE];
	struct vc_proto_hdr * hdr;
	struct stk_vc * stk;
	
	stk = &port->stk;

	hdr_len = sizeof(struct vc_proto_hdr) + sizeof(struct vc_attach_param);
	
	len = recv(port->sk, buf, hdr_len, 0);
	if(len <= 0){
		stk_excp(stk);
		return stk_curnt(stk)->init(port);
	}
	if(len != hdr_len){
		return stk_curnt(stk)->err(port, "Packet lenth too short", len);
	}

	hdr = (struct vc_proto_hdr *)buf;

	packet_len = (int)ntohs(hdr->len);
	cmd = ntohs(hdr->cmd);

	if(cmd == VCOM_CMD_DATAREADY){
		vc_buf_update(port, VC_BUF_TX);
	}
		
	if( packet_len < 0){
		return stk_curnt(stk)->err(port, "Wrong VCOM len", packet_len);
	}else if(packet_len > 0){
		if(packet_len > (RBUF_SIZE - hdr_len)){
			return stk_curnt(stk)->err(port, "payload is too long", packet_len);
		}
		len = recv(port->sk, &buf[hdr_len], packet_len, 0);
		if(len != packet_len)
			return stk_curnt(stk)->err(port, "VCOM len miss-match", len);
	}

	return try_ops3(stk_curnt(stk), recv, port, buf, hdr_len + packet_len);
}
#undef RBUF_SIZE
/*
int log_open(char *log_name)
{
    return log_fd = open(log_name,
                         O_WRONLY | O_CREAT | O_APPEND | O_NDELAY, 0666);
}

void log_close(void)
{
    if(log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
        printf("log closing ...\n");
    }
}

void log_write(const char *msg)
{
    char msgbuf[1024];
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    sprintf(msgbuf, "%02d-%02d-%04d %02d:%02d:%02d   ",
            lt->tm_mon+1, lt->tm_mday, lt->tm_year+1900,
            lt->tm_hour, lt->tm_min, lt->tm_sec);
    strcat(msgbuf, msg);
    write(log_fd, msgbuf, strlen(msgbuf));
}
*/

int startup(int argc, char **argv, struct vc_attr *port)
{
	char *addr;
	char ch;
	
	if(argc < 2) {
		printf("Usage : ./vcomd -l 'log name' -t 'tty ID' -d 'device model' -a 'IP address' -p 'device port'\n");
		return -1;
	}
	mon_init(0);
	while((ch = getopt(argc, argv, "l:t:d:a:p:r:")) != -1)  {
		switch(ch){
			case 'l':
				printf("open log file : %s ...\n", optarg);		
				mon_init(optarg); 
				break;
			case 't':            
				sscanf(optarg, "%d", &(port->ttyid));
				printf("setting tty ID : %d ...\n", port->ttyid);
				break;
			case 'd':
				sscanf(optarg, "%hx", &(port->devid));
				printf("setting device model : %x ...\n", port->devid);
				break;
			case 'a':  
				addr = optarg;
				port->ip_ptr = addr;
				printf("setting IP addr : %s ...\n", port->ip_ptr);
				break;
			case 'p':
				sscanf(optarg, "%d", &(port->port));
				printf("setting device port : %d ...\n", port->port);
				break;  
			case 'r':
				addr = optarg;
				port->ip_red = addr;
				printf("setting RIP addr : %s ...\n", port->ip_red);
				break;
			default:
				printf("Usage : ./vcomd -l 'log name' -t 'tty ID' -d 'device model' -a 'IP address' -p 'device port'\n");
				return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct vc_attr port;
	struct timeval tv;
	fd_set rfds;
	fd_set efds;
	fd_set wfds;
	int maxfd;
	int ret;
	unsigned int intflags;
	unsigned int lrecv;
	char filename[64];
	struct stk_vc * stk;

	const unsigned int psec = VC_PULL_PSEC;
	const unsigned int pusec = VC_PULL_PUSEC;

	stk = &port.stk;
	
	port.ip_red = 0;
	stk_mon = 0;	// stack monitor init
	if(startup(argc, argv, &port) == -1)
        return 0;

	sprintf(filename, "/proc/vcom/advproc%d", port.ttyid);
	port.fd = open(filename, O_RDWR);
	if(port.fd < 0){
		printf("cannot open file:%s\n", strerror(errno));
		return 0;
	}

	port.mbase = (char *)mmap(0, 4096*3, 
			PROT_READ|PROT_WRITE, MAP_FILE |MAP_SHARED, port.fd, 0);

	if(port.mbase <= 0){
		printf("failed to mmap\n");
		return 0;
	}
	stk->top = -1;	// state mechine stack init
	port.sk	= -1;
	vc_buf_setup(&port, VC_BUF_TX);
	vc_buf_setup(&port, VC_BUF_RX);
	vc_buf_setup(&port, VC_BUF_ATTR);
	stk_push(stk, &vc_netdown_ops);
	stk_curnt(stk)->init(&port);
	lrecv = 0;

	while(1){

		tv.tv_sec = psec;
		tv.tv_usec = pusec;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		if(check_attr_stat(&port, is_open) == ATTR_DIFF){
			if(attr_p(&port, is_open)){
				stk_curnt(stk)->open(&port);
			}else{
				stk_curnt(stk)->close(&port);
			}
		}

		if(check_attr_stat(&port, baud) == ATTR_DIFF||
		check_attr_stat(&port, pair) == ATTR_DIFF ||
		check_attr_stat(&port, flowctl) == ATTR_DIFF||
		check_attr_stat(&port, byte) == ATTR_DIFF||
		check_attr_stat(&port, stop) == ATTR_DIFF||
		check_attr_stat(&port, ms) == ATTR_DIFF){
			try_ops(stk_curnt(stk), ioctl, &port);
		}
		
		if(port.xmit_pending == 0){
			if(!is_rb_empty(port.rx)){
				try_ops(stk_curnt(stk), xmit, &port);
			}
		}

		if(!is_rb_empty(port.tx)){
			try_ops(stk_curnt(stk), pause, &port);
		}


		FD_SET(port.fd, &efds);
		FD_SET(port.sk, &rfds);

		try_ops5(stk_curnt(stk), event, &port, &tv, &rfds, &wfds, &efds);

		intflags = ADV_INT_RX|ADV_INT_TX;

		if(ioctl(port.fd, ADVVCOM_IOCSINTER, &intflags) < 0){
			printf("couldn't set iocstinter\n");
			break;
		}

		maxfd = port.fd;
		if(port.sk >= 0){
			maxfd = (port.sk > maxfd)?port.sk:maxfd;
			
		}
	
		ret = select(maxfd + 1, &rfds, &wfds, &efds, &tv);
		if(ret == 0 ){
			try_ops(stk_curnt(stk), poll, &port);
			continue;
		}

		if(FD_ISSET(port.fd, &efds)){
			vc_buf_update(&port, VC_BUF_ATTR);
		}

		if(FD_ISSET(port.sk, &rfds)){
			lrecv = 0;
			vc_recv_desp(&port);
		}else if(port.sk >= 0){
			unsigned int used = VC_TIME_USED(tv);
			lrecv += (used > 0)?used:1;
			if(lrecv > VC_PULL_TIME){
				stk_curnt(stk)->err(&port, "PROTO timeout", 0);
				printf("lrecv = %u\n", lrecv);
				lrecv = 0;
			}
		}

		if(FD_ISSET(port.fd, &rfds)){
			vc_buf_update(&port, VC_BUF_RX);
		}

		if(FD_ISSET(port.fd, &wfds)){
			vc_buf_update(&port, VC_BUF_TX);
			try_ops(stk_curnt(stk), resume, &port);
		}
	}

	return 0;	
}

