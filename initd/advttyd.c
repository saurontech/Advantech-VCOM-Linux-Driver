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


#ifdef	STREAM
#include <sys/ptms.h>
#endif

#include "advttyd.h"

#define MON_PATH "/tmp/advmon"

typedef struct _TTYINFO {
	pid_t   advttyp_pid;                        // process id for the port	                 
	char	mpt_nameidx_str[CF_MAXSTRLEN];      // Master pseudo TTY index
	char    dev_type_str[CF_MAXSTRLEN];         // Device Type
	char	dev_ipaddr_str[INET6_ADDRSTRLEN];       // Device IP address
	char    dev_portidx_str[CF_MAXSTRLEN];      // Device Port Index
	char	dev_redundant_ipaddr_str[INET6_ADDRSTRLEN];
	int has_redundant_ip;
} TTYINFO;

int __log_fd = -1;
int _restart;

static int  parse_env(char * cmdpath, char * workpath);
static int  daemon_init(void);
static int  paser_config(char * conf_name, TTYINFO ttyinfo[]);
static void spawn_ttyp(char * work_path, int nrport, TTYINFO ttyinfo[]);
static void shutdown_ttyp(int nrport, TTYINFO ttyinfo[]);
static void restart_handle();
//static u_long device_ipaddr(char * ipaddr);
static int  hexstr(char * strp);
static int  log_open(char * log_name);
static void log_close(void);
#ifdef ADVTTY_DEBUG
static void log_msg(const char * msg);
#endif

int main(int argc, char * argv[])
{
	int nrport;
	TTYINFO ttyinfo[CF_MAXPORTS];
	char work_path[PATH_MAX];
	char file_name[PATH_MAX];

	_restart = 0;
	nrport = 0;

	if(parse_env(argv[0], work_path) < 0)
		exit(-1);

	if(daemon_init() < 0)
		exit(-2);

	sprintf(file_name,"%s/%s", work_path, CF_LOGNAME);
	if(log_open(file_name) < 0)
		exit(-3);

	for(;;) {
		if(_restart) {
			ADV_LOGMSG("Advantech Virtual TTY daemon program - restart\n");
			_restart = 0;
		}
		sprintf(file_name,"%s/%s", work_path, CF_CONFNAME);
		if((nrport = paser_config(file_name, ttyinfo)) <= 0) {
			signal(SIGTERM, ((void (*)())restart_handle));
			pause();
			continue;
		}
		ADV_LOGMSG("Advantech Virtual TTY daemon program - %s\n", CF_VERSION);
		spawn_ttyp(work_path, nrport, ttyinfo);
		signal(SIGTERM, ((void (*)())restart_handle));
		pause();
		shutdown_ttyp(nrport, ttyinfo);
	}
	exit(0);
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
	int matchCount=0;
	char conf_dp[256];

	nrport = 0;    
	if((conf_fp = fopen(conf_name, "r")) == NULL) {
		ADV_LOGMSG("Open the configuration file [%s] fail\n", conf_name);
		return nrport;
	}
	while(nrport < CF_MAXPORTS) {
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
		ADV_LOGMSG("matchCount = %d\n", matchCount);
		if(atoi(mpt_nameidx_str) > CF_MAXPORTS)
			continue;
		if ((int_tmp = hexstr(dev_type_str)) <= 0 || int_tmp <= 0x1000)
			continue;
		/*
		   ulong_tmp = device_ipaddr(dev_ipaddr_str);
		   if ((ulong_tmp == (u_long)0xFFFFFFFF) || (ulong_tmp == 0))
		   continue;
		 */
		if ((int_tmp = atoi(dev_portidx_str)) <= 0 || int_tmp > 16)
			continue;
		memset(&ttyinfo[nrport], 0, sizeof(TTYINFO));
		strcpy(ttyinfo[nrport].mpt_nameidx_str, mpt_nameidx_str);
		strcpy(ttyinfo[nrport].dev_type_str, dev_type_str);
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
		++nrport;
	}
	fclose(conf_fp);
	if(nrport == 0)
		ADV_LOGMSG("There is no configuration data for Advantech TTY\n");
	return nrport;
}

static void spawn_ttyp(char * work_path, int nrport, TTYINFO ttyinfo[])
{
	int idx;
	pid_t advttyp_pid;
	char cmd[PATH_MAX];
	char log[PATH_MAX];
	char mon[PATH_MAX];

	sprintf(cmd, "%s/%s", work_path, CF_PORTPROG);
	sprintf(log, "%s/%s", work_path, CF_LOGNAME);

	for(idx = 0; idx < nrport; ++idx) {
		sprintf(mon, "%s/advtty%d", MON_PATH, idx);
		if((advttyp_pid = fork()) < 0) {
			ADV_LOGMSG("Spawn MPT[%s] fail\n", ttyinfo[idx].mpt_nameidx_str);
			continue;
		}
		if(advttyp_pid == 0 && ttyinfo[idx].has_redundant_ip) {
			ADV_LOGMSG("executing command %s -l %s -t %s -d %s -a %s -p %s -r %s\n", 
					cmd,
					log, 
					ttyinfo[idx].mpt_nameidx_str, 
					ttyinfo[idx].dev_type_str, 
					ttyinfo[idx].dev_ipaddr_str, 
					ttyinfo[idx].dev_portidx_str, 
					ttyinfo[idx].dev_redundant_ipaddr_str);
			log_close();

			execl(cmd, CF_PORTPROG,
					"-l", mon,
					"-t", ttyinfo[idx].mpt_nameidx_str,
					"-d", ttyinfo[idx].dev_type_str,
					"-a", ttyinfo[idx].dev_ipaddr_str,
					"-p", ttyinfo[idx].dev_portidx_str,
					"-r", ttyinfo[idx].dev_redundant_ipaddr_str,
					NULL);
			exit(-4);      
		}
		else if(advttyp_pid == 0)
		{
			log_close();
			execl(cmd, CF_PORTPROG,
					"-l", mon,
					"-t", ttyinfo[idx].mpt_nameidx_str,
					"-d", ttyinfo[idx].dev_type_str,
					"-a", ttyinfo[idx].dev_ipaddr_str,
					"-p", ttyinfo[idx].dev_portidx_str,
					NULL);
			exit(-4);      
		}
		ttyinfo[idx].advttyp_pid = advttyp_pid;
	}
	return;
}

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

static void restart_handle()
{
	_restart = 1;
	return;
} 

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

static int log_open(char * log_name)
{
	return __log_fd = open(log_name,
			O_WRONLY | O_CREAT | O_APPEND | O_NDELAY, 0666);
}

static void log_close(void)
{
	if(__log_fd >= 0) {
		close(__log_fd);
		__log_fd = -1;
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
