#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> 
#include <ctype.h>
#include <dirent.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <syslog.h>
#include "jsmn.h"
#include "jstree.h"
#include "jstree_readhelper.h"


char * _config_password;
char * _config_keyfile;
char * _config_rootca;



BIO *bio_err=0;
static char *pass;

int __search_lport_stat_inode(int ipfamily, unsigned short port,  unsigned short stat)
{
	FILE *fp;
	char buf[2048];
	char path[1024];
	int ret;
	int found;
	int sl;
	char laddr[64];
	unsigned short lport;
	char raddr[64];
	char * ptr;
	unsigned int connection_state;
	char tmp[1024];
	int inode;
	snprintf(path, sizeof(path), "/proc/net/tcp%s", ipfamily==4?"":"6");
	//printf("path = %s\n", path);

	found = 0;

	fp = fopen(path, "r");
	
	//printf("fp= %x\n", fp);
	if(fp == NULL){
		return -1;
	}
	// skip first two lines
	for (int i = 0; i < 1; i++) {
		fgets(buf, sizeof(buf), fp);
	}

	while (fgets(buf, sizeof(buf), fp)) {
	//	printf("buf = %s\n", buf);
		ret = sscanf(buf, "%d: %s %s %x %s %s %s %s %s %d", 
				&sl, 
				laddr,
				raddr,
				&connection_state,
				tmp,
				tmp,
				tmp,
				tmp,
				tmp,
				&inode
				);
//		printf("sscanf ret = %d\n", ret);
		if(ret < 10){
			printf("!!!!!!!!!!!!!!!!!!!!!!!!!!! error scanf\n");
			continue;
		}
		if(connection_state != stat){
			continue;
		}
		ptr = strstr(laddr, ":");
		if(ptr == 0){
			//printf("didn't find :\n");
			continue;
		}
		sscanf(ptr+1, "%hx", &lport);
//		printf("ret = %d sl = %d laddr = %s port = %u state = %u inode= %d\n", ret, sl, laddr, lport, connection_state, inode);
		if(lport == port){
			found = 1;
			break;
		}
	}
	fclose(fp);

	if(found){
		//printf("!!!!!!!!!!!!!!!!!!!!! found inode %d\n", inode);
		return inode;
	}

	return -1;

}

int __search_port_inode( unsigned short port)
{
	int inode;

	do{
		inode = __search_lport_stat_inode(4, port, 1);
		if(inode >= 0){
			//printf("found tcp4 inode = %d", inode);
			break;
		}

		inode = __search_lport_stat_inode(6, port, 1);
	}while(0);

	return inode;
}

int __pid_search_fd(int pid, char * file)
{
	static DIR *dir = 0;
	struct dirent *entry;
	char *name;
	int n;
	int fd;
	char status[1204];
	char buf[1024];
	char fdpath[1024];
	struct stat sb;
	snprintf(fdpath, 1024, "/proc/%d/fd", pid);
	if (!dir) {
		dir = opendir(fdpath);
		if(!dir){
			printf("Can't open /proc/fd: %s", fdpath);
			return -1;
		}
	}
	for(;;) {
		if((entry = readdir(dir)) == NULL) {
			closedir(dir);
			dir = 0;
			return -1;
		}
		name = entry->d_name;
		if (!(*name >= '0' && *name <= '9'))
			continue;


		fd = atoi(name);

		sprintf(status, "/proc/%d/fd/%d", pid, fd);
		if(stat(status, &sb)){
			printf( "stat failed");
			continue;
		}
		if(lstat(status, &sb) < 0){
			printf( "lstat failed");
			continue;
		}

		n = readlink(status, buf, sizeof(buf));

		if(n <= 0){
			printf("readlink failed");
			closedir(dir);
			return -1;
		}

		if(n !=  strlen(file) ){
			//can't be the same file since the langth is different
			continue;
		}

		if(memcmp(buf, file, strlen(file)) == 0){
			closedir(dir);
			dir = 0;
			return pid;
		}
	}

}

int __cmd_search_file(char * cmd, char * file, char *retcmd, int len)
{
	static DIR *dir = 0;
	struct dirent *entry;
	char *name;
	int n;
	char status[32];
	char buf[1024];
	int retlen;
	FILE *fp;
	int pid;
	struct stat sb;

	if (!dir) {
		dir = opendir("/proc");
		if(!dir){
			printf("Can't open /proc");
			return -1;
		}
	}
	for(;;) {
		if((entry = readdir(dir)) == NULL) {
			//syslog(LOG_DEBUG, "%s(%d)readdir failed", __func__, __LINE__);
			printf( "no %s were found operating %s", cmd, file);
			closedir(dir);
			dir = 0;
			return -1;
		}

		name = entry->d_name;
		if (!(*name >= '0' && *name <= '9'))
			continue;


		pid = atoi(name);

		sprintf(status, "/proc/%d", pid);
		if(stat(status, &sb))
			continue;
		sprintf(status, "/proc/%d/cmdline", pid);
		if((fp = fopen(status, "r")) == NULL){
			printf( "%s(%d)fopen failed", __func__, __LINE__);
			continue;
		}

		if((n=fread(buf, 1, sizeof(buf)-1, fp)) > 0) {

			if(buf[n-1]=='\n'){
				buf[--n] = 0;
			}
			name = buf;
			while(n) {
				if(((unsigned char)*name) < ' ')
					*name = ' ';
				name++;
				n--;
			}
			*name = 0;
			/* if NULL it work true also */
		}
		fclose(fp);

		if(strstr(buf, cmd) > 0){
			if(__pid_search_fd(pid, file) > 0){

				if(strlen(buf) > len){
					retlen = len -1;
				}else{
					retlen = strlen(buf);
				}

				memcpy(retcmd, buf, retlen);
				retcmd[retlen] = '\0';

				closedir(dir);
				dir = 0;
				//printf("command %s pid %d\n", cmd, pid);
				return pid;
			}else{
				//printf("pid %dcmd %s\n", pid, cmd);
			}
		}
	}
}

char* __pid_vcomd_get_address(int pid, char * buf, int len)
{
	int cnt;
	int fd;
	int i;
	char path[1024];
	char *ptr;

	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
	//printf("path = %s\n", path);

	fd = open(path, O_RDONLY);
	//printf("fd = %d\n", fd);
	cnt = read(fd, buf, len);
	close(fd);
	/*printf("cnt = %d buf= %s\n", cnt, buf);
	for (i = 0; i < cnt; i++){
		printf("[%d]%c(0x%hhx)\n", i, buf[i], buf[i]);
	}*/
	i = 0;
	do{
		ptr = strstr(&buf[i], "-a");
	//	printf("ptr = %x buf[%d]= %s\n", ptr, i, &buf[i]);
		if(ptr > 0){
			//printf("addr: %s\n", ptr+2);
			break;
		}
		i += strlen(&buf[i]);
	}while(++i < cnt);

	return ptr+2;

}

int init_serversock(char * service)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	int enable = 1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;

	s = getaddrinfo(NULL, service, &hints, &result);
	if (s != 0) {
	   printf("%s : getaddrinfo: %s.\n", __func__, gai_strerror(s));
	   return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd < 0) 
			continue;

		setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
		
		if(bind(sfd, rp->ai_addr, rp->ai_addrlen) != 0){
			printf("%s : bind %d not Success.\n", __func__ , sfd);	
			close(sfd);
			continue;
		}
		break;
	}
	freeaddrinfo(result);           /* No longer needed */

	if(listen(sfd, 32) != 0){
		printf("failed to listen\n");
		close(sfd);
		return -1;
	}

	return sfd;
}

int __set_block(int sock)
{
	int arg;

	if( (arg = fcntl(sock, F_GETFL, NULL)) < 0) {
		printf( "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));

		return -1;
	}
	arg &= (~O_NONBLOCK);
	if( fcntl(sock, F_SETFL, arg) < 0) {
		printf( "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));

		return -1;
	}

	return 0;
}

int __set_nonblock(int sock)
{
	int arg;

	if( (arg = fcntl(sock, F_GETFL, NULL)) < 0) {
		printf( "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));

		return -1;
	}
	arg |= O_NONBLOCK;
	if( fcntl(sock, F_SETFL, arg) < 0) {
		printf( "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));

		return -1;
	}

	return 0;
}

typedef struct pair_info_t{
	int tcp_sock;
	int tcp_rxlen;
	unsigned char tcp_rxbuf[2048];
	int tcp_rb;
	int tcp_wb;
	int (*tcp_send)(struct pair_info_t *self);
	int (*tcp_recv)(struct pair_info_t *self);

	int ssl_sock;
	SSL *ssl;
	int ssl_rxlen;
	unsigned char ssl_rxbuf[2048];
	int ssl_rb;
	int ssl_wbr;
	int ssl_wb;
	int ssl_rbw;
	int (*ssl_send)(struct pair_info_t *self);
	int (*ssl_recv)(struct pair_info_t *self);

	void (*free_pair)(struct pair_info_t *self);
}pair_info;

int tcp_send(pair_info * self)
{

	int wlen;
	int ptr = self->ssl_rxlen;

	self->tcp_wb = 0;
	//printf("ready to tcp send %d bytes of data\n", ptr);
	wlen = send(self->tcp_sock,self->ssl_rxbuf, ptr, MSG_NOSIGNAL);

	if(wlen < 0){
		switch(errno){
			case EAGAIN:
				self->tcp_wb = 1;
				break;

				/* Some other error */
			default:	      
				printf("TCP write problem\n");
				return -1;
				break;
		}
	}else if(wlen > 0){
		//_client_txbuf_clear(self, wlen);
		if(wlen == ptr){
			self->ssl_rxlen = 0;
		}else{
			printf("moving tcp send data wlen = %d totalt = %d\n", wlen, ptr);
			memmove(self->ssl_rxbuf, &self->ssl_rxbuf[wlen], ptr - wlen);
			self->ssl_rxlen -= wlen;
		}
		return wlen;
	}else{
		printf("send len = 0 !?\n");
	}

	return 0;

}

int tcp_recv(pair_info * self)
{
	int rlen;
	int ptr = self->tcp_rxlen;
	unsigned char * rxbuf = self->tcp_rxbuf;
	self->tcp_rb = 0;

	rlen = recv(self->tcp_sock,&rxbuf[ptr], sizeof(self->tcp_rxbuf) - ptr, 0);
	if(rlen < 0){
		switch(errno){
			case EAGAIN:
				self->tcp_rb = 1;
				break;
				/* Some other error */
			default:	      
				printf("TCP Read problem %d\n", errno);
				return -1;
				break;
		}
	}else if(rlen > 0){
		//printf("tcp recieved %d bytes of data total %d\n", rlen, self->tcp_rxlen);
		self->tcp_rxlen += rlen;
		return rlen;
	}else{
		printf("tcp closed by remote client\n");
		return -1;
	}

	return 0;

}

int berr(char * string)
{
	BIO_printf(bio_err,"%s\n",string);
	ERR_print_errors(bio_err);
	syslog(LOG_DEBUG, "%s", string);
	return 0;
}

void berr_exit(string)
	char *string;
{
	BIO_printf(bio_err,"%s\n",string);
	ERR_print_errors(bio_err);
	syslog(LOG_DEBUG, "%s", string);
	exit(-1);
	//
	//return 0;
}


int ssl_send(pair_info * self)
{
	

	int wlen;
//	int i;
	int ptr = self->tcp_rxlen;
	self->ssl_wb = 0;
	self->ssl_wbr = 0;

	//printf("ssl ready to send %d bytes of data\n", ptr);
	wlen = SSL_write(self->ssl,self->tcp_rxbuf, ptr);
	//printf("ssl wlen = %d\n", wlen);

	switch(SSL_get_error(self->ssl, wlen)){
		/* We wrote something*/
		case SSL_ERROR_NONE:
	/*		for(i = 0; i < wlen; i++){
				printf("%x ", self->txbuf[i]);
			}
			printf(" wlen = %d ptr = %d\n", wlen, ptr);*/
			//_client_txbuf_clear(self, wlen);
			if(wlen == ptr){
				self->tcp_rxlen = 0;
			}else{
				printf("moving tcp send data wlen = %d totalt = %d\n", wlen, ptr);
				memmove(self->tcp_rxbuf, &self->tcp_rxbuf[wlen], ptr - wlen);
				self->tcp_rxlen -= wlen;
			}
			return wlen;
			break;

			/* We would have blocked */
		case SSL_ERROR_WANT_WRITE:
			self->ssl_wb = 1;
			break;

			/* We get a WANT_READ if we're
			   trying to rehandshake and we block on
			   write during the current connection.

			   We need to wait on the socket to be readable
			   but reinitiate our write when it is */
		case SSL_ERROR_WANT_READ:
			self->ssl_wbr = 1;
			break;

			/* Some other error */
		default:	      
			berr("SSL write problem");
			return -1;
			break;
	}

	return 0;

}

int ssl_recv(pair_info * self)
{
	self->ssl_rb = 0;
	self->ssl_rbw = 0;
		int rlen;
	int err;
	//int i;
	char errstr[1024];
	int ptr = self->ssl_rxlen;
	unsigned char * rxbuf = self->ssl_rxbuf;
		//self->rbw = 0;
		//self->rb = 0;

	rlen = SSL_read(self->ssl, &rxbuf[ptr], sizeof(self->ssl_rxbuf) - ptr);
	err = SSL_get_error(self->ssl,rlen);

	switch(err){
		case SSL_ERROR_NONE:
			self->ssl_rxlen += rlen;
			//self->connto_reset(self);
		/*	for(i = 0; i < self->rxlen; i++){
				printf("%x ", rxbuf[i]);
			}
			printf("  ssl read %d\n", rlen);*/

			break;
		case SSL_ERROR_ZERO_RETURN:
			/* End of data */
			printf("ssl read zero return\n");
			return -1;
			break;
		case SSL_ERROR_WANT_READ:
			self->ssl_rb = 1;
			break;

			/* We get a WANT_WRITE if we're
			   trying to rehandshake and we block on
			   a write during that rehandshake.

			   We need to wait on the socket to be 
			   writeable but reinitiate the read
			   when it is */
		case SSL_ERROR_WANT_WRITE:
			self->ssl_rbw = 1;
			break;
		default:
			printf("sock = %d \n", self->ssl_sock);
			printf("default rlen %d err %d:\n", rlen, err);
			ERR_error_string_n(ERR_get_error(), errstr, 1024);
			printf("error string %s", errstr);
			berr("SSL read problem");
			return -1;
	}

		
	return 0;

}

void free_pair(pair_info * self)
{
	printf("%s(%d)\n", __func__, __LINE__);
	__set_block(self->tcp_sock);
	__set_block(self->ssl_sock);
	close(self->tcp_sock);
	SSL_shutdown(self->ssl);
	SSL_free(self->ssl);
	close(self->ssl_sock);
	free(self);
}

pair_info * alloc_pair_info()
{
	pair_info *ret;
	ret = malloc(sizeof(pair_info));
	ret->tcp_rxlen = 0;
	ret->tcp_rb = 0;
	ret->tcp_wb = 0;
	ret->ssl_rxlen = 0;
	ret->ssl_rb = 0;
	ret->ssl_wb = 0;
	ret->ssl_rbw = 0;
	ret->ssl_wbr = 0;
	
	ret->tcp_send = tcp_send;
	ret->tcp_recv = tcp_recv;
	ret->ssl_send = ssl_send;
	ret->ssl_recv = ssl_recv;
	ret->free_pair = free_pair;

	return ret;
}

void *pair_thread(void *data)
{
	pair_info * self = (pair_info *)data;
	int maxfd;
	int ret;
	fd_set rfds;
	fd_set wfds;
	struct timeval *tv;
	printf("pair thread created\n");

	do{
		if(self->tcp_rxlen < sizeof(self->tcp_rxbuf)){
			if(self->tcp_recv(self) < 0){
				self->free_pair(self);
				return 0;
			}
		}

		if(self->ssl_rxlen < sizeof(self->ssl_rxbuf)){
			if(self->ssl_recv(self) < 0){
				self->free_pair(self);
				return 0;
			}
		}

		if(self->tcp_rxlen > 0){
			if(self->ssl_send(self) < 0){
				self->free_pair(self);
				return 0;
			}
		}

		if(self->ssl_rxlen > 0){
			if(self->tcp_send(self) < 0){
				self->free_pair(self);
				return 0;
			}
		}

		maxfd = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		tv = 0;
		if(self->tcp_wb){
			FD_SET(self->tcp_sock, &wfds);
			if(self->tcp_sock > maxfd){
				maxfd = self->tcp_sock;
			}
		}

		if(self->tcp_rb ||
		self->tcp_rxlen < sizeof(self->tcp_rxbuf)){
			FD_SET(self->tcp_sock, &rfds);
			if(self->tcp_sock > maxfd){
				maxfd = self->tcp_sock;
			}
		}

		if(self->ssl_wb || self->ssl_rbw){
			FD_SET(self->ssl_sock, &wfds);
			if(self->ssl_sock > maxfd){
				maxfd = self->ssl_sock;
			}
		}

		if(self->ssl_rb || 
		self->ssl_wbr ||
		self->ssl_rxlen < sizeof(self->ssl_rxbuf)){
			FD_SET(self->ssl_sock, &rfds);
			if(self->ssl_sock > maxfd){
				maxfd = self->ssl_sock;
			}
		}

		if(maxfd < 0){
			continue;
		}

		ret = select(maxfd + 1, &rfds, &wfds, 0, tv);
		if(ret < 0){
			printf("select ret = %d\n", ret);

		}

	}while(1);
}



/*The password code is not thread safe*/
static int password_cb(char *buf,int num,
		int rwflag,void *userdata)
{
	if(num<strlen(pass)+1)
		return(0);

	strcpy(buf,pass);
	return(strlen(pass));
}



static void sigpipe_handle(int x){
}

int check_cert_chain(SSL *ssl)	
{

	X509 *peer;
	char peer_CN[256];

	if(SSL_get_verify_result(ssl)!=X509_V_OK){
		berr("Certificate doesn't verify");
		return -1;
	}
	/*Check the common name*/
	peer=SSL_get_peer_certificate(ssl);
	if(peer){
		X509_NAME_get_text_by_NID (
				X509_get_subject_name (peer),  NID_commonName,  peer_CN, 256);
		printf("common name %s\n", peer_CN);
	}

	return 0;
}

int verify_callback (int ok, X509_STORE_CTX *store)
{
	char data[256];

	X509 *cert = X509_STORE_CTX_get_current_cert(store);

	if (!ok)
	{
		int  depth = X509_STORE_CTX_get_error_depth(store);
		int  err = X509_STORE_CTX_get_error(store);

		printf("-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		printf("  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
		printf("  subject  = %s\n", data);
		printf("  err %i:%s\n", err, X509_verify_cert_error_string(err) );
	}/*else{
	   printf("ssl verification OK\n");
	   X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
	   printf("  issuer   = %s\n", data);
	   X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
	   printf("  subject  = %s\n", data);
	   }*/	

	return ok;
}

SSL_CTX *initialize_ctx(keyfile,password)
	char *keyfile;
	char *password;
{
	const SSL_METHOD *meth;
	SSL_CTX *ctx;
	unsigned int ssl_verify;

	if(!bio_err){
		/* Global system initialization*/
		SSL_library_init();
		SSL_load_error_strings();

		/* An error write context */
		bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);
	}

	/* Set up a SIGPIPE handler */
	signal(SIGPIPE,sigpipe_handle);

	/* Create our context*/
	meth=SSLv23_method();
	//meth = TLSv1_method();
	ctx=SSL_CTX_new(meth);

	/* Load our keys and certificates*/
	if(strlen(keyfile) != 0){
		printf("setup keyfile\n");
		if(!(SSL_CTX_use_certificate_chain_file(ctx,
						keyfile)))
			berr_exit("Can't read certificate file");

		if(strlen(password) != 0){
			pass=password;
			SSL_CTX_set_default_passwd_cb(ctx,
					password_cb);

			if(!(SSL_CTX_use_PrivateKey_file(ctx,
							keyfile,SSL_FILETYPE_PEM)))
				berr_exit("Can't read key file");
		}
	}
	//syslog(LOG_DEBUG, "rootCA.pem path %s", _config_rootca);
	/* Load the CAs we trust*/
	if(!(SSL_CTX_load_verify_locations(ctx,
					_config_rootca/*"rootCA.pem"*/,0)))
		berr_exit("Can't read CA list");

	ssl_verify = SSL_VERIFY_PEER;

	SSL_CTX_set_verify(ctx, ssl_verify, verify_callback);

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
	SSL_CTX_set_verify_depth(ctx,1);
#endif

	//	SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	return ctx;
}

void destroy_ctx(ctx)
	SSL_CTX *ctx;
{
	SSL_CTX_free(ctx);
}

void load_dh_params(ctx,file)
	SSL_CTX *ctx;
	char *file;
{
	DH *ret=0;
	BIO *bio;

	if ((bio=BIO_new_file(file,"r")) == NULL)
		berr_exit("Couldn't open DH file");

	ret=PEM_read_bio_DHparams(bio,NULL,NULL,
			NULL);
	BIO_free(bio);
	if(SSL_CTX_set_tmp_dh(ctx,ret)<0)
		berr_exit("Couldn't set DH parameters");
}


int loadconfig(char * filepath)
{
	int fd;
	int filelen;
	int ret;
	int tokcount;
	char * filedata;

	jsmn_parser p;
	jsmntok_t *tok;
	_tree_node * rnode;


	fd = open(filepath, O_RDONLY);
//	printf("%s fd = %d\n", filepath, fd);
	if(fd <= 0){
		//can't open file
		return -1;
	}

	filelen = lseek(fd, 0, SEEK_END);
//	printf("filelen = %d\n", filelen);
	
	lseek(fd, 0, SEEK_SET);
	filedata = malloc(filelen);

	ret = read(fd, filedata, filelen);

//	printf("ret = %d\n", ret);

	jsmn_init(&p);

	tokcount = 2;
	tok = malloc(sizeof(*tok) * tokcount);

	ret = 0;

	do{
		ret = jsmn_parse(&p, filedata, filelen, tok, tokcount);
		if(ret == JSMN_ERROR_NOMEM){
			tokcount = tokcount * 2;
			tok = realloc_it(tok, sizeof(*tok) * tokcount);
			if(tok == NULL){
				return -1;
			}
			continue;
		}else if(ret < 0){
			printf("failed ret = %d\n", ret);
		}
//		printf("jsmn_parse %d\n", ret);
		break;
	}while(1);

//	dump(filedata, tok, p.toknext, 0);

	jstreeret result;

	result = js2tree(filedata, tok, p.toknext);
	//dumptree(result.node, 0);

	if(readJSTree(result.node->r, &rnode, "ssl", "keyfile")!= 2){
		printf("didn't find keyfile\n");
		return -1;
	}
	//printf("found keyfile = %s\n", rnode->data.data);

	_config_keyfile = rnode->data.data;

	if(readJSTree(result.node->r, &rnode, "ssl", "rootca")!= 2){
		printf("didn't find keyfile\n");
		return -1;
	}
	//printf("found rootca = %s\n", rnode->data.data);

	_config_rootca = rnode->data.data;

	if(readJSTree(result.node->r, &rnode, "ssl", "password")!= 2){
		printf("didn't find keyfile\n");
		return -1;
	}
	//printf("found password = %s\n", rnode->data.data);

	_config_password = rnode->data.data;

	close(fd);

	return 0;
}
#define SSL_PORT "5555"
int main(int argc, char **argv)
{
	int server;
	int client;
	socklen_t addrlen;
	int inode;
	int pid;
	char sockname[1024];
	char cmd[1024];
	char buf[2048];
	char *addr_str;
	char *wd_end;
	//char dbug_tmp[1024];
	SSL_CTX *ctx;

	struct sockaddr_storage addr;
	unsigned short service;

	struct sockaddr_in remote;
	int sk;
	pair_info *pair;
	pthread_t tid;

	if(argc != 2){
		printf("%s [config.json]\n", argv[0]);
		return -1;
	}

	if(geteuid() != 0){
		printf("need to execuate as root\n");
		return -1;
	}

	/*if(getcwd(dbug_tmp, sizeof(dbug_tmp))){
		syslog(LOG_DEBUG, "cwd = %s\n", dbug_tmp);
	}*/
	if((wd_end = memrchr(argv[1], '/', strlen(argv[1])))){
		int wdlen = wd_end - argv[1] + 1;
		char *wd;

		wd = malloc(wdlen + 1);
		if(wd == 0){
			syslog(LOG_DEBUG, "can't malloc for chdir");
			printf("can't malloc for chdir\n");
		}
		wd[wdlen] = '\0';

		memcpy(wd, argv[1] , wdlen);
		syslog(LOG_DEBUG, "changing work dir to %s", wd);
		printf("changing work dir to %s\n", wd);
		chdir(wd);
	}	
	
	printf("loading configurations\n");
	if(loadconfig(argv[1])){
		printf("cannot load config file");
		return -1;

	}
	printf("initializing SSL context\n");
	ctx = initialize_ctx( _config_keyfile, _config_password);

	if(__search_lport_stat_inode(6, atoi(SSL_PORT), 0xa) >= 0){
		printf("found vcomproxy in background\n");
		exit(-1);
	}

	printf("invoking SSL service\n");
	syslog(LOG_DEBUG, "invoking SSL service\n");
	server = init_serversock(SSL_PORT);


	printf("system standby\n");
	syslog(LOG_DEBUG,"system standby\n");
	do{
		addrlen = sizeof(struct sockaddr_storage);
		client = accept(server, (struct sockaddr *)&addr, &addrlen);

		//syslog(LOG_DEBUG, "address family %hu len = %d client sock = %d\n", addr.ss_family, addrlen, client);

		if(addrlen == sizeof(struct sockaddr_in6)){
			/*syslog(LOG_DEBUG, "port6 = %hu-->%hu\n", 
					ntohs(((struct sockaddr_in6 *)(&addr))->sin6_port),
					((struct sockaddr_in6 *)(&addr))->sin6_port);*/
			service = ntohs(((struct sockaddr_in6 *)(&addr))->sin6_port);
		}else if(addrlen == sizeof(struct sockaddr_in)){
			//syslog(LOG_DEBUG, "port = %hu\n", ntohs(((struct sockaddr_in *)(&addr))->sin_port));
			service = ntohs(((struct sockaddr_in *)(&addr))->sin_port);
		}else{
			syslog(LOG_DEBUG, "addrlen = %d\n", addrlen);
			close(client);
			continue;
		}

		//printf("port = %hu\n", service);
		inode = __search_port_inode(service);
		//printf("inode = %d\n", inode);
		if(inode < 0){
			close(client);
			continue;
		}
		snprintf(sockname, sizeof(sockname), "socket:[%d]", inode);
		//printf("ready to find socket: %s\n", sockname);
		pid = __cmd_search_file("vcomd", sockname, cmd, sizeof(cmd));
		if(pid < 0){
			return -1;
		}
		//printf("found pid = %d\n", pid);
		addr_str = __pid_vcomd_get_address(pid, buf, sizeof(buf));
		//printf("address is -->%s\n", addr_str);
		
		if(inet_pton(AF_INET, addr_str, &remote.sin_addr) <= 0){
			printf("failed to create remote address\n");
			close(client);
			continue;
		}
		remote.sin_port = htons(5202);
		remote.sin_family = AF_INET;
		//printf("remote.sin_addr = %x port %x %x\n", remote.sin_addr, remote.sin_port, remote.sin_family);
		sk = socket(AF_INET, SOCK_STREAM, 0);
		if(sk < 0){
			printf("failed to create remote socket\n");
			close(client);
			continue;
		}
		//printf("sk = %d\n", sk);

		if(connect(sk, (struct sockaddr *)&remote, sizeof(remote)) < 0){
			printf("failed to connect:(%d)%s\n", errno, strerror(errno));
			close(sk);
			close(client);
			continue;
		}
		printf("ready to allocate pair info\n");
		pair = alloc_pair_info();
		pair->tcp_sock = client;
		pair->ssl_sock = sk;
		pair->ssl = SSL_new(ctx);
		//printf("pair ssl_sock %d tcp_sock %d\n", pair->ssl_sock, pair->tcp_sock);
		if(SSL_set_fd(pair->ssl, pair->ssl_sock) == 0){
			printf("SSL_set_fd failed\n");
			SSL_free(pair->ssl);
			free(pair);
			close(client);
			close(sk);
			continue;
		}
		
		if(SSL_connect(pair->ssl) <= 0){
			printf("SSL_connect failed\n");
			SSL_free(pair->ssl);
			free(pair);
			close(client);
			close(sk);
			continue;
		}

		__set_nonblock(pair->tcp_sock);
		__set_nonblock(pair->ssl_sock);

		if(pthread_create(&tid, 0, pair_thread, pair) != 0){
			printf("failed to create pthread\n");
		}
		pthread_detach(tid);
	}while(1);

	return 0;
}
