#define _GNU_SOURCE
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
#include "jsmn.h"
#include "jstree.h"
#include "jstree_read.h"
#include "vcom_load_sslcfg.h"
//#include "vcom_debug.h"

#define RBUF_SIZE	4096

extern void * stk_mon; 

static int recv_second_chance(int sock, char * buf, int buflen)
{
	int ret;
	struct timeval tv;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	ret = select(sock + 1, &rfds, 0, 0, &tv);

	if(ret <= 0){
		return 0;
	}

	return recv(sock, buf, buflen, 0);
}

static int vc_frame_recv_tcp(struct vc_attr *port, char *buf, int buflen)
{
	int len; 

	len = recv(port->sk, buf, buflen, 0);
	if(len <= 0){
		//stk_excp(stk);
		return -1;/*stk_curnt(stk)->init(port);*/
	}
	if(len != buflen){
		do{
			if(len < buflen){
				len += recv_second_chance(port->sk, &buf[len], buflen - len);
				if(len == buflen){
					break;
				}
			}
			return -1;/*stk_curnt(stk)->err(port, "Packet lenth too short", len);*/
		}while(0);
	}
	return len;
}

static int vc_frame_recv_ssl(struct vc_attr *port, char *buf, int buflen)
{
	int len;

	len = ssl_recv_direct(port->ssl, buf, buflen);
	if(len == SSL_OPS_SELECT){
		return len;
	}else if(len <= 0){
		printf("%s(%d)\n", __func__, __LINE__);
		//stk_excp(stk);
		return -1;/*stk_curnt(stk)->init(port);*/
	}
	if(len != buflen){
		do{
			if(len < buflen){
				len += ssl_recv_simple(port->ssl, &buf[len], buflen - len, 500);
				if(len == buflen){
					break;
				}
				
			}
			
			printf("%s(%d)\n", __func__, __LINE__);
			return SSL_OPS_FAIL;/*stk_curnt(stk)->err(port, "Packet lenth too short", len);*/
		}while(0);
	}
	return len;
}

struct vc_ops * vc_recv_desp(struct vc_attr *port)
{
	int hdr_len;
	int packet_len;
	unsigned short cmd;
	static char buf[RBUF_SIZE];
	struct vc_proto_hdr * hdr;
	struct stk_vc * stk;
	
	stk = &port->stk;

	hdr_len = sizeof(struct vc_proto_hdr) + sizeof(struct vc_attach_param);
	
	if(port->ssl){
		int __ret;

		__ret = vc_frame_recv_ssl(port, buf, hdr_len);

		if(__ret == SSL_OPS_SELECT){
			return stk_curnt(stk);
		}else if(__ret <= 0){/*SSL_OPS_FAIL*/
			printf("SSL recv failed\n");
			stk_excp(stk);
			return stk_curnt(stk)->init(port);
		}

	}else if(vc_frame_recv_tcp(port, buf, hdr_len) < 0){
		stk_excp(stk);
		return stk_curnt(stk)->init(port);
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
		
		if(port->ssl){
			int __ret;

			__ret = vc_frame_recv_ssl(port,  &buf[hdr_len], packet_len);

			if(__ret <= 0){
				stk_excp(stk);
				return stk_curnt(stk)->init(port);
			}

		}else if(vc_frame_recv_tcp(port, &buf[hdr_len], packet_len) < 0){
			stk_excp(stk);
			return stk_curnt(stk)->init(port);
		}
	}
	
	return try_ops(stk_curnt(stk), recv, port, buf, hdr_len + packet_len);
}
#undef RBUF_SIZE

void _init_std()
{
	int fd;
	
	if(dup2(STDIN_FILENO, STDIN_FILENO) == -1 &&
	   errno == EBADF){
		fd = open("/dev/null", O_RDONLY);
		dup2(fd, STDIN_FILENO);
	}

	if(dup2(STDOUT_FILENO, STDOUT_FILENO) == -1 &&
	   errno == EBADF){
		fd = open("/dev/null", O_RDWR);
		dup2(fd, STDOUT_FILENO);
	}

	if(dup2(STDERR_FILENO, STDERR_FILENO) == -1 &&
	   errno == EBADF){
		fd = open("/dev/null", O_RDWR);
		dup2(fd, STDERR_FILENO);
	}
}

void usage(char * cmd)
{
	printf("Usage : %s [-l/-t/-d/-a/-p/-r] [argument]\n", cmd);
	printf("The most commonly used commands are:\n");
	printf("	-l	Log file\n");
	printf("	-t	TTY id\n");
	printf("	-d	Device modle\n");
	printf("	-a	IP addr\n");
	printf("	-p	Device port\n");
	printf("	-r	Redundant IP\n");
	printf("	-s	enable TLS\n");
	printf("	-h	For help\n");
}

int startup(int argc, char **argv, struct vc_attr *port)
{
	char *addr;
	char *sslcfg;
	int ch;

	port->ttyid = -1;
	port->devid = 0;
	port->port = -1;
	port->ssl = 0;
	port->ssl_proxy = 0;
	addr = 0;

	if(argc < 2) {
		usage(argv[0]);
		return -1;
	}

	mon_init(0);
	while((ch = getopt(argc, argv, "sS:hl:t:d:a:p:r:")) != -1)  {
		switch(ch){
			case 'h':

				usage(argv[0]);
				return -1;
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
				sscanf(optarg, "%u", &(port->port));
				printf("setting device port : %u ...\n", port->port);
				break;  
			case 'r':
				addr = optarg;
				port->ip_red = addr;
				printf("setting RIP addr : %s ...\n", port->ip_red);
				break;
			case 's':
				port->ssl_proxy = 1;
				printf("ssl enabled\n");
				break;
			case 'S':
				sslcfg = optarg;
				printf("ssl config %s\n", sslcfg);
				if(loadconfig(sslcfg)){
					printf("cannot load config file");
				}
				
				port->ssl = sslinfo_alloc();
				init_ssl_lib();
				char wd_orig[1024];
				char * wd_new;
				if(getcwd(wd_orig, sizeof(wd_orig))){
					printf("original cwd is %s\n", wd_orig);
				}

				wd_new = create_cfg_cwd(sslcfg);
				chdir(wd_new);
				free(wd_new);
				printf("keyfile %s\n", _config_keyfile);
				printf("rootCA %s\n", _config_rootca);
				port->ssl->ctx = initialize_ctx( _config_rootca, 
								_config_keyfile, 
								_config_password);
				printf("create ctx @%p\n", port->ssl->ctx);
				chdir(wd_orig);
				break;
			default:
				usage(argv[0]);
				return -1;
		}
	}
	if(addr == NULL || port->port < 0 || port->devid == 0 || port->ttyid < 0){
		usage(argv[0]);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct vc_attr port;
	fd_set rfds;
	fd_set efds;
	fd_set wfds;
	int maxfd;
	int ret;
	unsigned int intflags;
	unsigned int lrecv;
	char filename[64];
	struct stk_vc * stk;
	struct timeval zerotv;

	const unsigned int psec = VC_PULL_PSEC;
	const unsigned int pusec = VC_PULL_PUSEC;

	_init_std();

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
	
	timerclear(&zerotv);
	while(1){
		struct timeval tv;
		struct timeval * tv_ptr;

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

		try_ops(stk_curnt(stk), event, &port, &tv, &rfds, &wfds, &efds);

		intflags = ADV_INT_RX|ADV_INT_TX;

		if(ioctl(port.fd, ADVVCOM_IOCSINTER, &intflags) < 0){
			printf("couldn't set iocstinter\n");
			break;
		}

		maxfd = port.fd;
		if(port.sk >= 0){
			maxfd = (port.sk > maxfd)?port.sk:maxfd;
			
		}
		
		tv_ptr = &tv;

		if(port.ssl && port.sk >= 0){

			vc_recv_desp(&port);
			ssl_set_fds(port.ssl, 0, &rfds, &wfds);
			port.ssl->recv.read = 1;//for VCOM protocol always recv
			if(SSL_pending(port.ssl->ssl)){
				tv_ptr = &zerotv;
			}
		}
		
		ret = select(maxfd + 1, &rfds, &wfds, &efds, tv_ptr);
		if(ret == 0 ){
			try_ops(stk_curnt(stk), poll, &port);
			continue;
		}

		if(FD_ISSET(port.fd, &efds)){
			vc_buf_update(&port, VC_BUF_ATTR);
		}

		if(port.sk < 0){
			//socket not connected do nothing;
		}else if(port.ssl){
			int tasks = 0;
			tasks = ssl_handle_fds(port.ssl, &rfds, &wfds);
			if(tasks & invoke_ssl_recv){
				//printf("invoked ssl recv\n");
				vc_recv_desp(&port);
			}else if(tasks){
				printf("other tasks %x\n", tasks);
			}
		} else if(FD_ISSET(port.sk, &rfds)){
			lrecv = 0;
			vc_recv_desp(&port);
		}else if(port.sk >= 0){
			unsigned int used = VC_TIME_USED(tv);
			lrecv += (used > 0)?used:1;
			if(lrecv > VC_PULL_TIME){
				stk_curnt(stk)->err(&port, "PROTO timeout", 0);
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

