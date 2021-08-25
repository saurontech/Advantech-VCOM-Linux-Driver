#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> 
#include <ctype.h>
#include <dirent.h>
#include <syslog.h>

#include "proc_tools.h"

#ifdef	STREAM
#include <sys/ptms.h>
#endif

#include "advttyd.h"

typedef struct _TTYINFO {
	pid_t   advttyp_pid;                        // process id for the port	                 
	char	mpt_nameidx_str[CF_MAXSTRLEN];      // Master pseudo TTY index
	char    dev_type_str[CF_MAXSTRLEN];         // Device Type
	char	dev_ipaddr_str[INET6_ADDRSTRLEN];       // Device IP address
	char    dev_portidx_str[CF_MAXSTRLEN];      // Device Port Index
	char	dev_redundant_ipaddr_str[INET6_ADDRSTRLEN];
	int has_redundant_ip;
	int dev_ssl;
} TTYINFO;

int __log_fd = -1;
int _restart;

static int  parse_env(char * cmdpath, char * workpath);
static int  paser_config(char * conf_name, TTYINFO ttyinfo[]);
static void spawn_ttyp(char * work_path, int nrport, TTYINFO ttyinfo[]);

#ifdef ADVTTY_DEBUG
static void log_msg(const char * msg);
#endif

void __close_stdfd(void)
{
	close(0);
	close(1);
	close(2);
}

void usage(char * cmd)
{
	printf("Usage : %s [-d -t]\n", cmd);
	printf("The most commonly used commands are:\n");
	printf("	-d	run as deamon\n");
	printf("	-t	run test, don't exec\n");
	printf("	-h	For help\n");
}


#define _safe_strcpy(DEST, SRC) snprintf(DEST, sizeof(DEST), "%s", SRC)

static int run_as_daemon;
static int testrun;
int setup_options(int argc, char *argv[])
{
	int ch;
	while((ch = getopt(argc, argv, "dht")) != -1)  {
		switch(ch){
			case 'h':
				usage(argv[0]);
				return -1;
			case 'd':
				run_as_daemon = 1;
				break;
			case 't':
				testrun = 1;
				break;

		}
	}

	return 0;
}

int main(int argc, char * argv[])
{
	int nrport;
	TTYINFO ttyinfo[CF_MAXPORTS];
	char work_path[PATH_MAX];
	char file_name[PATH_MAX];
	int ret;
	int wp_len;
	int cf_len;

	nrport = 0;
	run_as_daemon = 0;
	testrun = 0;

	if(setup_options(argc, argv)){
		usage(argv[0]);
		return -1;
	}

	if(run_as_daemon){
		__close_stdfd();
	}
	

	if(parse_env(argv[0], work_path) < 0)
		return -1;

	wp_len = strlen(work_path);
	cf_len = strlen(CF_CONFNAME);

	ret = snprintf(file_name, sizeof(file_name), "%s/%s", work_path, CF_CONFNAME);
	if(ret < wp_len + cf_len){
		syslog(LOG_DEBUG, "filename + configname trunc !!!");	
	}

	if((nrport = paser_config(file_name, ttyinfo)) <= 0) {
		syslog(LOG_DEBUG, "failed to paser config file");
		return 0;
	}
	ADV_LOGMSG("Advantech Virtual TTY daemon program - %s\n", CF_VERSION);
	spawn_ttyp(work_path, nrport, ttyinfo);

	return 0;
}

static int parse_env(char * cmdpath, char * workpath)
{
	int i;
	char currpath[PATH_MAX], tmpbuf[PATH_MAX];

	getcwd(currpath, sizeof(currpath));
	_safe_strcpy(tmpbuf, cmdpath);
	for(i = strlen(tmpbuf) - 1; i > 0; --i) {
		if(tmpbuf[i] == '/')
			break;
	}
	if(i) {
		tmpbuf[i] = 0;
		chdir(tmpbuf);
	}
	getcwd(workpath, PATH_MAX);
	chdir(currpath);
	return 0;
}


static int paser_config(char * conf_name, TTYINFO ttyinfo[])
{
	int nrport;
	int int_tmp;
	FILE * conf_fp;
	char mpt_nameidx_str[CF_MAXSTRLEN];
	char dev_type_str[CF_MAXSTRLEN];
	char dev_portidx_str[CF_MAXSTRLEN];
	char dev_ipaddr_str[INET6_ADDRSTRLEN];
	char dev_redundant_ipaddr_str[INET6_ADDRSTRLEN];
	char sscanf_fmt[1024];
	char *dev_type;
	int matchCount=0;
	char conf_dp[256];
	char *sslkeyword = "ssl:";

	
	nrport = 0;    
	if((conf_fp = fopen(conf_name, "r")) == NULL) {
		syslog(LOG_DEBUG, "Open the configuration file [%s] fail", conf_name);
		return nrport;
	}

	snprintf(sscanf_fmt, sizeof(sscanf_fmt), 
			"%%%zus%%%zus%%%zus%%%zus%%%zus", 
			pt_buf_maxstrlen(mpt_nameidx_str),
			pt_buf_maxstrlen(dev_type_str),
			pt_buf_maxstrlen(dev_ipaddr_str), 
			pt_buf_maxstrlen(dev_portidx_str), 
			pt_buf_maxstrlen(dev_redundant_ipaddr_str));
	
	while(nrport < CF_MAXPORTS) {
		dev_type = dev_type_str;
		if(fgets(conf_dp, sizeof(conf_dp), conf_fp) == NULL)
			break;
		/*
		 * FORMATE
		 * Read configuration & the data format of every data line is :
		 * [Minor] [Device-Type] [Device-IP] [Port-Idx] [redundant-ip]
		 */
		

		matchCount = sscanf(conf_dp, sscanf_fmt,
				mpt_nameidx_str, dev_type_str,
				dev_ipaddr_str, dev_portidx_str, dev_redundant_ipaddr_str);

		if ( matchCount < 4) {
			continue;
		}
		//syslog(LOG_DEBUG,"matchCount = %d\n", matchCount);
		if(atoi(mpt_nameidx_str) > CF_MAXPORTS)
			continue;
		
		memset(&ttyinfo[nrport], 0, sizeof(TTYINFO));

		if(strlen(dev_type_str) > 4 && 
		memcmp(sslkeyword, dev_type_str, strlen(sslkeyword)) == 0){
			dev_type = dev_type_str + strlen(sslkeyword);
			ttyinfo[nrport].dev_ssl = 1;
			//syslog(LOG_DEBUG, "nrport %d, found ssl\n", nrport);
		}
		//syslog(LOG_DEBUG,"dev_type = %s\n", dev_type);

		if ((int_tmp = atoi(dev_portidx_str)) <= 0 /*|| int_tmp > 16*/){
			syslog(LOG_INFO ,"advvcom/unsupported port index(%d) on port %d", int_tmp, nrport);
			continue;
		}
		_safe_strcpy(ttyinfo[nrport].mpt_nameidx_str, mpt_nameidx_str);
		_safe_strcpy(ttyinfo[nrport].dev_type_str, dev_type);
		_safe_strcpy(ttyinfo[nrport].dev_ipaddr_str, dev_ipaddr_str);
		_safe_strcpy(ttyinfo[nrport].dev_portidx_str, dev_portidx_str);
		if (matchCount > 4)
		{
			//ulong_tmp = device_ipaddr(dev_redundant_ipaddr_str);
			ADV_LOGMSG("redundant ip = %s\n", dev_redundant_ipaddr_str);
			//if ((ulong_tmp != (u_long)0xFFFFFFFF) && (ulong_tmp != 0))
			if(1)			
			{
				_safe_strcpy(ttyinfo[nrport].dev_redundant_ipaddr_str, dev_redundant_ipaddr_str);
				ADV_LOGMSG("redundant ip copied= %s\n", ttyinfo[nrport].dev_redundant_ipaddr_str);
				ttyinfo[nrport].has_redundant_ip = 1;
			}
			else
			{
				ttyinfo[nrport].has_redundant_ip = 0;
			}		
		}
		//syslog(LOG_DEBUG,"dev[%d] ssl= %d\n", nrport, ttyinfo[nrport].dev_ssl);
		++nrport;
	}
	fclose(conf_fp);
	if(nrport == 0)
		ADV_LOGMSG("There is no configuration data for Advantech TTY\n");
	return nrport;
}

static char cmd[PATH_MAX];
static char log[PATH_MAX];
static char mon[PATH_MAX];
static char sslconf[PATH_MAX];

static int _create_syscmd(char * syscmd, int syscmdlen, 
			TTYINFO ttyinfo[], int idx)
{
	int cmdidx;

	cmdidx = 0;
	if(ttyinfo[idx].has_redundant_ip) {

		if(ttyinfo[idx].dev_ssl){
			cmdidx = snprintf(syscmd, syscmdlen, 
					"%s -l%s -t%s -d%s -a%s -p%s -r%s -S%s", 
					cmd,
					mon, 
					ttyinfo[idx].mpt_nameidx_str,
					ttyinfo[idx].dev_type_str,
					ttyinfo[idx].dev_ipaddr_str,
					ttyinfo[idx].dev_portidx_str,
					ttyinfo[idx].dev_redundant_ipaddr_str,
					sslconf
					);
		}else{
			ADV_LOGMSG("executing command %s -l %s -t %s -d %s -a %s -p %s -r %s \n", 
					cmd,
					log, 
					ttyinfo[idx].mpt_nameidx_str, 
					ttyinfo[idx].dev_type_str, 
					ttyinfo[idx].dev_ipaddr_str, 
					ttyinfo[idx].dev_portidx_str, 
					ttyinfo[idx].dev_redundant_ipaddr_str);
			cmdidx = snprintf(syscmd, syscmdlen, 
					"%s -l%s -t%s -d%s -a%s -p%s -r%s ", 
					cmd,
					mon, 
					ttyinfo[idx].mpt_nameidx_str,
					ttyinfo[idx].dev_type_str,
					ttyinfo[idx].dev_ipaddr_str,
					ttyinfo[idx].dev_portidx_str,
					ttyinfo[idx].dev_redundant_ipaddr_str
					);
		}

	}else{
		if(ttyinfo[idx].dev_ssl){
			cmdidx = snprintf(syscmd, syscmdlen, 
					"%s -l%s -t%s -d%s -a%s -p%s -S%s", 
					cmd,
					mon, 
					ttyinfo[idx].mpt_nameidx_str,
					ttyinfo[idx].dev_type_str,
					ttyinfo[idx].dev_ipaddr_str,
					ttyinfo[idx].dev_portidx_str,
					sslconf
					);
		}else{
			cmdidx = snprintf(syscmd, syscmdlen, 
					"%s -l%s -t%s -d%s -a%s -p%s ", 
					cmd,
					mon, 
					ttyinfo[idx].mpt_nameidx_str,
					ttyinfo[idx].dev_type_str,
					ttyinfo[idx].dev_ipaddr_str,
					ttyinfo[idx].dev_portidx_str
					);
		}

	}

	return cmdidx;
}

static int oldcmd_cmp(char *oldcmd, int oldcmdlen, TTYINFO * ttyinfo)
{
	int diff = 0;
	char * addr;
	char * _port;
	char * dtype;
	char * mfile;
	char * _raddr;
	char * _sslcfg;
	
	mfile = __cmd_get_opts(oldcmd, oldcmdlen, "-l");
	if(mfile == 0){
		printf("missing log\n");
		diff++;
	}else if(strncmp(mon, mfile, oldcmdlen)){
		diff++;
	}else{
		printf("log is the same\n");
	}

	dtype = __cmd_get_opts(oldcmd, oldcmdlen, "-d");

	if(dtype == 0){
		printf("missing devtype\n");
		diff++;
	}else if( strncmp(ttyinfo->dev_type_str, 
				dtype, strlen(dtype))){
		diff++;
		printf("devtype is diff\n");
	}else{
		printf("devtype is the same\n");
	}

	addr = __cmd_get_opts(oldcmd, oldcmdlen, "-a");

	if(addr == 0){
		printf("address is missing\n");
		diff++;
	}else if (strncmp(ttyinfo->dev_ipaddr_str, 
				addr, strlen(addr))){
		diff++;
		printf("port is diff\n");
	}else{
		printf("address is the same\n");
	}

	_port = __cmd_get_opts(oldcmd, oldcmdlen, "-p");

	if(_port== 0){
		printf("missing port\n");
	}else if( strncmp(ttyinfo->dev_portidx_str, 
				_port, strlen(_port))){
		diff++;
		printf("port is diff\n");
	}else{
		printf("port is the same\n");
	}

	if(ttyinfo->has_redundant_ip){
		_raddr = __cmd_get_opts(oldcmd, oldcmdlen, "-r");
		if(_raddr == 0){
			printf("missing redundent IP\n");
			diff++;
		}else if(strncmp(ttyinfo->dev_redundant_ipaddr_str, 
				_raddr, strlen(_raddr))){
			printf("raddr is diff\n");
			diff++;
		}else{
			printf("redundent IP is the same\n");
		}
	}else{
		_raddr = __cmd_get_opts(oldcmd, oldcmdlen, "-r");
		if(_raddr){
			diff++;
		}
	}

	if(ttyinfo->dev_ssl){
		_sslcfg = __cmd_get_opts(oldcmd, oldcmdlen, "-S");
		if(_sslcfg == 0){
			printf("missing ssl config\n");
			diff++;
		}else if(strncmp(sslconf, 
				_sslcfg, strlen(_sslcfg))){
			printf("ssl is diff\n");
			diff++;
		}else{
			printf("ssl is the same\n");
		}
	}else{
		_sslcfg = __cmd_get_opts(oldcmd, oldcmdlen, "-S");
		if(_sslcfg){
			printf("additional ssl config\n");
			diff++;
		}
	}

	return diff;
}


static void spawn_ttyp(char * work_path, int nrport, TTYINFO ttyinfo[])
{
	int idx;
	int oldpid;
	int cmdidx;
	//int sslproxy = 0;
	int oldcmdlen;
	
	char oldcmd[2048];
	char vcomif[1024];
	char syscmd[1024];
	char killcmd[256];


	snprintf(cmd, sizeof(cmd), "%s/%s", work_path, CF_PORTPROG);
	snprintf(log, sizeof(log), "%s/%s", work_path, CF_LOGNAME);
	//snprintf(sslcmd, sizeof(sslcmd), "%s/%s", work_path, CF_SSLPROG);
	snprintf(sslconf, sizeof(sslconf), "%s/%s", work_path, CF_SSLCONF);
	
	for(idx = 0; idx < nrport; ++idx) {
		struct stat sb;
		//int __ret;
		sprintf(mon, "%s/advtty%s", MON_PATH, ttyinfo[idx].mpt_nameidx_str);
		
		snprintf(vcomif, sizeof(vcomif), 
				"/proc/vcom/advproc%s", 
				ttyinfo[idx].mpt_nameidx_str);
		printf("access %s\n", vcomif);
		if(stat(vcomif, &sb)){
			printf("cannot access VCOM interface %s\n", vcomif);
			exit(0);
		}

		printf("trying to find inode %ld\n", sb.st_ino);
		

		if( __cmd_inode_search_pid("vcomd", sb.st_ino, 
					oldcmd, sizeof(oldcmd), 
					&oldcmdlen, &oldpid) < 0 ){
			printf("no old pid no need to compare\n");
		}else if(oldcmd_cmp(oldcmd, oldcmdlen, &ttyinfo[idx]) == 0){
			printf("old command is the same\n");
			continue;
		}
		
		cmdidx = _create_syscmd(syscmd, sizeof(syscmd), ttyinfo, idx);

		if(oldpid > 0){
			snprintf(killcmd, sizeof(killcmd), "kill -9 %d", oldpid);
			printf("exec killcmd:%s\n", killcmd);
			if(!testrun){
				system(killcmd);
			}
		}

		snprintf(&syscmd[cmdidx], sizeof(syscmd) - cmdidx - 1, "&");
		printf("exec cmd: %s\n", syscmd);
		if(!testrun){
			system(syscmd);
		}
	}


	return;
}
#ifdef ADVTTYD_DEBUG
static void log_msg(const char * msg)
{
	long t;
	struct tm * lt;
	char msg_buf[1024];

	if(__log_fd < 0)
		return;

	t = time(0);
	lt = localtime(&t);	
	sprintf(msg_buf, "%02d-%02d-%4d %02d:%02d:%02d  ",
			lt->tm_mon + 1, lt->tm_mday, lt->tm_year + 1900,
			lt->tm_hour, lt->tm_min, lt->tm_sec);
	strcat(msg_buf, msg);

	if(flock(__log_fd, LOCK_EX) < 0)
		return;
	write(__log_fd, msg_buf, strlen(msg_buf));
	flock(__log_fd,  LOCK_UN);
	return;
}
#endif
