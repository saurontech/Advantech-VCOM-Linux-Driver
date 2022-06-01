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
#ifdef _VCOM_SUPPORT_TLS
#include "jsmn.h"
#include "jstree.h"
#include "jstree_read.h"
#include "vcom_load_sslcfg.h"

static ssl_pwd_data m_ssl_pwd_data;
static vc_ssl_cfg m_sslcfg;
#endif
//#include "vcom_debug.h"

#define RBUF_SIZE	4096

void * stk_mon; 
#ifdef _VCOM_MONITOR_H
struct vc_monitor vc_mon;
#endif

static int custom_verify_callback (int ok, X509_STORE_CTX *store)
{

	

	if (!ok)
	{
		int err = X509_STORE_CTX_get_error(store);
		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		char data[256];
		int  depth = X509_STORE_CTX_get_error_depth(store);

		printf("-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof(data));
		printf("  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, sizeof(data));
		printf("  subject  = %s\n", data);
		printf("  err %i:%s\n", err, X509_verify_cert_error_string(err) );

		switch(err){
			case X509_V_ERR_CERT_NOT_YET_VALID:
			case X509_V_ERR_CERT_HAS_EXPIRED:
				ok = 1;
				break;
			default:
				break;
		}
	}/*else{
	   printf("ssl verification OK\n");
	   X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
	   printf("  issuer   = %s\n", data);
	   X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
	   printf("  subject  = %s\n", data);
	   }*/	

	return ok;
}

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

static int vc_frame_recv_tcp(struct vc_attr *port, char *buf, int rlen)
{
	int len; 

	len = recv(port->sk, buf, rlen, 0);
	if(len <= 0){
		//stk_excp(stk);
		return -1;/*stk_curnt(stk)->init(port);*/
	}
	if(len == rlen){
		return len;
	}

	if(len < rlen){
		len += recv_second_chance(port->sk, &buf[len], rlen - len);
		if(len == rlen){
			return len;
		}
	}
	
	return -2;/*stk_curnt(stk)->err(port, "Packet lenth too short", len);*/

}
#ifdef _VCOM_SUPPORT_TLS
static int vc_frame_recv_ssl(struct vc_attr *port, char *buf, int rlen)
{
	int len;
	int ssl_errno;
	struct stk_vc * stk;
	char ssl_errstr[256];
	
	stk = &port->stk;

	len = ssl_recv_direct(port->ssl, buf, rlen, &ssl_errno);
	if(len == SSL_OPS_SELECT){
		return len;
	}else if(len <= 0){
		//share debug code with recv frame body
		//return SSL_OPS_FAIL;
	}else if(len == rlen){
		return len;
	}else if(len < rlen){
		len += ssl_recv_simple(port->ssl, &buf[len], rlen - len, 500, &ssl_errno);
		if(len == rlen){
			return len;
		}
	}

	ssl_errno_str(port->ssl, ssl_errno, ssl_errstr, sizeof(ssl_errstr));
	
	_stk_log(stk, "%s %s", __func__, ssl_errstr);
	return SSL_OPS_FAIL;	
}
#endif

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
#ifdef _VCOM_SUPPORT_TLS
	if(port->ssl){
		int __ret;

		__ret = vc_frame_recv_ssl(port, buf, hdr_len);

		if(__ret == SSL_OPS_SELECT){
			return stk_curnt(stk);
		}else if(__ret <= 0){/*SSL_OPS_FAIL*/
			//printf("vc_frame_recv_ssl %d\n", __ret);
			stk_excp(stk);
			return stk_curnt(stk)->init(port);
		}

	}else 
#endif
	if(vc_frame_recv_tcp(port, buf, hdr_len) < 0){
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
		int __ret;
		if(packet_len > (RBUF_SIZE - hdr_len)){
			return stk_curnt(stk)->err(port, "payload is too long", packet_len);
		}

#ifdef _VCOM_SUPPORT_TLS	
		if(port->ssl){
			__ret = vc_frame_recv_ssl(port,  &buf[hdr_len], packet_len);

			if(__ret <= 0){
				//printf("%s(%d) cmd %hx\n",__func__,__LINE__, cmd);
				stk_excp(stk);
				return stk_curnt(stk)->init(port);
			}

		}else 
#endif
		if((__ret = vc_frame_recv_tcp(port, &buf[hdr_len], packet_len)) < 0){
			printf("vc_frame_recv_tcp error %d\n", __ret);
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
	printf("Usage : %s [-l/-t/-d/-a/-p/-r/-S] [argument]\n", cmd);
	printf("The most commonly used commands are:\n");
	printf("	-l	Log file\n");
	printf("	-t	TTY id(starts from 0)\n");
	printf("	-d	Device magic number\n");
	printf("	-a	IP addr\n");
	printf("	-p	Device port(starts from 1)\n");
	printf("	-r	Redundant IP\n");
#ifdef _VCOM_SUPPORT_TLS
	printf("	-S	enable TLS with a given json config\n");
#endif
	printf("	-h	For help\n");
}

int startup(int argc, char **argv, struct vc_attr *port)
{
	char *addr;
#ifdef _VCOM_SUPPORT_TLS
	char *sslcfg;
#endif
	int ch;
	int _set_devid;

	_set_devid = 0;

	port->ttyid = -1;
	port->devid = 0;
	port->port = 0;
#ifdef _VCOM_SUPPORT_TLS
	port->ssl = 0;
#endif
	addr = 0;

	if(argc < 2) {
		usage(argv[0]);
		return -1;
	}

	mon_init(0);
	while((ch = getopt(argc, argv, "S:hl:t:d:a:p:r:")) != -1)  {
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
				_set_devid = 1;
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
#ifdef _VCOM_SUPPORT_TLS
			case 'S':
				sslcfg = optarg;
				printf("ssl config %s\n", sslcfg);
				if(loadconfig(sslcfg, &m_sslcfg)){
					printf("cannot load config file");
					return -1;
				}
				
				port->ssl = sslinfo_alloc();
				init_ssl_lib();
				char wd_orig[1024];
				char * wd_new;
				if(getcwd(wd_orig, sizeof(wd_orig))){
					printf("original cwd is %s\n", wd_orig);
				}

				wd_new = create_cfg_cwd(sslcfg);
				if(wd_new){
					chdir(wd_new);
					free(wd_new);
				}

				port->ssl->ctx = initialize_ctx( m_sslcfg.rootca, 
								m_sslcfg.keyfile, 
								m_sslcfg.password,
								&m_ssl_pwd_data);

				if(m_sslcfg.accept_expired_key){
					unsigned int ssl_verify;
					ssl_verify = SSL_VERIFY_PEER;
					SSL_CTX_set_verify(port->ssl->ctx, 
						ssl_verify, custom_verify_callback);
				}
				
				if(port->ssl->ctx == 0){
					//printf("failed to lad key file\n");
					exit(0);
				}
				printf("create ctx @%p\n", port->ssl->ctx);
				chdir(wd_orig);
				break;
#endif
			default:
				usage(argv[0]);
				return -1;
		}
	}
	if(addr == NULL || port->port == 0 || port->ttyid < 0 || _set_devid == 0){
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

	if(port.mbase == 0){
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
		int maxfd;
		int ret;
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
#ifdef _VCOM_SUPPORT_TLS
		if(port.ssl && port.sk >= 0){

			vc_recv_desp(&port);
			ssl_set_fds(port.ssl, 0, &rfds, &wfds);
			//for VCOM protocol always recv when there is data to read
			port.ssl->recv.read = 1;
			if(SSL_pending(port.ssl->ssl)){
				tv_ptr = &zerotv;
			}
		}
#endif
		
		ret = select(maxfd + 1, &rfds, &wfds, &efds, tv_ptr);
		if(ret == 0 && tv_ptr != &zerotv){
			try_ops(stk_curnt(stk), poll, &port);
			continue;
		}

		if(FD_ISSET(port.fd, &efds)){
			vc_buf_update(&port, VC_BUF_ATTR);
		}

		if(port.sk < 0){
			//socket not connected do nothing;
		}
#ifdef _VCOM_SUPPORT_TLS
		else if(port.ssl){
			int tasks = 0;
			tasks = ssl_handle_fds(port.ssl, &rfds, &wfds);
			if(tasks & invoke_ssl_recv){
				lrecv = 0;
				//printf("invoked ssl recv\n");
				vc_recv_desp(&port);
			}else{
				if(tasks){
					printf("other tasks %x\n", tasks);
				}
				if(tv_ptr != &zerotv){
					unsigned int used = VC_TIME_USED(tv);
					lrecv += (used > 0)?used:1;	
				}
				if(lrecv > VC_PULL_TIME){
					stk_curnt(stk)->err(&port, "PROTO timeout", 0);
					lrecv = 0;
				}
			}
		}
#endif 
		else if(FD_ISSET(port.sk, &rfds)){
			lrecv = 0;
			vc_recv_desp(&port);
		}else{
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

