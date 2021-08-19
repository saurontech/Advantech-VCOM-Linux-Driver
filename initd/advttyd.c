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
//static int  daemon_init(void);
static int  paser_config(char * conf_name, TTYINFO ttyinfo[]);
static void spawn_ttyp(char * work_path, int nrport, TTYINFO ttyinfo[]);
//static void shutdown_ttyp(int nrport, TTYINFO ttyinfo[]);
//static void restart_handle();
//static u_long device_ipaddr(char * ipaddr);
//static int  hexstr(char * strp);
//static int  log_open(char * log_name);
//static void log_close(void);
#ifdef ADVTTY_DEBUG
static void log_msg(const char * msg);
#endif

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
			syslog(LOG_DEBUG, "Can't open /proc/fd: %s", fdpath);
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
			syslog(LOG_DEBUG, "stat failed");
			continue;
		}
		if(lstat(status, &sb) < 0){
			syslog(LOG_DEBUG, "lstat failed");
			continue;
		}

		n = readlink(status, buf, sizeof(buf));

		if(n <= 0){
			syslog(LOG_DEBUG, "readlink failed");
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
			syslog(LOG_DEBUG, "Can't open /proc");
			return -1;
		}
	}
	for(;;) {
		if((entry = readdir(dir)) == NULL) {
		//	syslog(LOG_DEBUG, "%s(%d)readdir failed", __func__, __LINE__);
			syslog(LOG_DEBUG, "no %s were found operating %s", cmd, file);
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
			syslog(LOG_DEBUG, "%s(%d)fopen failed", __func__, __LINE__);
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

		if(memcmp(buf, cmd, strlen(cmd)) == 0){
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

void __close_stdfd(void)
{
	close(0);
	close(1);
	close(2);
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

//	__close_stdfd();

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
	strcpy(tmpbuf, cmdpath);
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
/*
static int daemon_init(void)   
{   
	pid_t   pid;  

	if(getppid() == 1)
		goto L_EXIT; 

#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif

	if((pid = fork()) < 0)
		return(-1);
	if(pid != 0)                // parent process
		exit(0);

	if(setpgrp() == -1) {
		return(-1);
	}
	signal(SIGHUP, SIG_IGN);	// immune from pgrp leader death
	setsid();                   // become session leader
	if((pid = fork()) < 0)
		return(-1);
	if(pid != 0)                // parent process
		exit(0);

L_EXIT:
	signal(SIGCLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	errno = 0;
	close(0);
	close(1);
	close(2);
	chdir("/");                 // change working directory
	umask(0);                   // clear   file   mode   creation
	return(0);   
}   
*/

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
	char *dev_type;
	int matchCount=0;
	char conf_dp[256];
	char *sslkeyword = "ssl:";

	
	nrport = 0;    
	if((conf_fp = fopen(conf_name, "r")) == NULL) {
		syslog(LOG_DEBUG, "Open the configuration file [%s] fail", conf_name);
		return nrport;
	}
	
	while(nrport < CF_MAXPORTS) {
		dev_type = dev_type_str;
		if(fgets(conf_dp, sizeof(conf_dp), conf_fp) == NULL)
			break;
		/*
		 * FORMATE
		 * Read configuration & the data format of every data line is :
		 * [Minor] [Device-Type] [Device-IP] [Port-Idx] [redundant-ip]
		 */
		matchCount = sscanf(conf_dp, "%s%s%s%s%s",
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
		/*
		if ((int_tmp = hexstr(dev_type)) <= 0 || int_tmp <= 0x1000)
			continue;
		*/
		/*
		   ulong_tmp = device_ipaddr(dev_ipaddr_str);
		   if ((ulong_tmp == (u_long)0xFFFFFFFF) || (ulong_tmp == 0))
		   continue;
		 */
		syslog(LOG_DEBUG,"dev_type success = %x\n", int_tmp);

		if ((int_tmp = atoi(dev_portidx_str)) <= 0 /*|| int_tmp > 16*/){
			syslog(LOG_INFO ,"advvcom/unsupported port index(%d) on port %d", int_tmp, nrport);
			continue;
		}
		strcpy(ttyinfo[nrport].mpt_nameidx_str, mpt_nameidx_str);
		strcpy(ttyinfo[nrport].dev_type_str, dev_type);
		strcpy(ttyinfo[nrport].dev_ipaddr_str, dev_ipaddr_str);
		strcpy(ttyinfo[nrport].dev_portidx_str, dev_portidx_str);
		if (matchCount > 4)
		{
			//ulong_tmp = device_ipaddr(dev_redundant_ipaddr_str);
			ADV_LOGMSG("redundant ip = %s\n", dev_redundant_ipaddr_str);
			//if ((ulong_tmp != (u_long)0xFFFFFFFF) && (ulong_tmp != 0))
			if(1)			
			{
				strcpy(ttyinfo[nrport].dev_redundant_ipaddr_str, dev_redundant_ipaddr_str);
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
		char * _raddr;
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
	}

	if(ttyinfo->dev_ssl){
		char * _sslcfg;
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
		
		oldpid = __cmd_inode_search_pid("vcomd", sb.st_ino, 
					oldcmd, sizeof(oldcmd), 
					&oldcmdlen);

		printf("oldpid =%d cmdlen = %d\n", oldpid, oldcmdlen);

		if(oldpid == 0){
			printf("no old pid no need to compare\n");
		}else if(oldcmd_cmp(oldcmd, oldcmdlen, &ttyinfo[idx]) == 0){
			printf("old command is the same\n");
			continue;
		}
		
		cmdidx = _create_syscmd(syscmd, sizeof(syscmd), ttyinfo, idx);

		if(oldpid > 0){
			snprintf(killcmd, sizeof(killcmd), "kill -9 %d", oldpid);
			//printf("%s\n", killcmd);
			system(killcmd);
		}

		snprintf(&syscmd[cmdidx], sizeof(syscmd) - cmdidx - 1, "&");
		//printf("%s\n", syscmd);
		system(syscmd);
	}


	return;
}
/*
static void shutdown_ttyp(int nrport, TTYINFO ttyinfo[])
{
	int idx;
	int wait_port;
	pid_t pid;

	for(idx = 0; idx < nrport; ++idx) {
		kill(ttyinfo[idx].advttyp_pid, SIGTERM);
	}

	for(wait_port = nrport; wait_port;) {
		for(idx = 0; idx < nrport; ++idx) {
			if(ttyinfo[idx].advttyp_pid == 0)
				continue;
			pid = waitpid(ttyinfo[idx].advttyp_pid, NULL, WNOHANG);
			if((pid == ttyinfo[idx].advttyp_pid) ||
					((pid < 0) && (errno == ECHILD))) {
				ttyinfo[idx].advttyp_pid = 0;
				--wait_port;
			}
		}
		if(wait_port)
			usleep(200 * 1000);
	}
	return;
}
*/
/*static void restart_handle()
{
	_restart = 1;
	return;
} */
/*
static int hexstr(char * strp)
{
	int i, ch, val;
	for(i = val = 0; (ch = *(strp + i)) != '\0'; ++i) {
		if(ch >= '0' && ch <= '9') {
			val = 16 * val + ch - '0';
		}
		else if((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
			ch = toupper(ch);
			val = 16 * val + ch - 'A' + 10;
		}
		else
			return 0;
	}
	return val;
}
*/
/*static int log_open(char * log_name)
{
	return __log_fd = open(log_name,
			O_WRONLY | O_CREAT | O_APPEND | O_NDELAY, 0666);
}*/
/*
static void log_close(void)
{
	if(__log_fd >= 0) {
		close(__log_fd);
		__log_fd = -1;
	}
	return;
}*/
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
