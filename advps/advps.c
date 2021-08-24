#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "advconf.h"
#include "proc_tools.h"

int main(int argc, char *argv[])
{
	int i;
	pid_t pid;
	char ifname[1024];
	char cmd[1024];
	int cmdlen;
	struct stat sb;	
		
	for(i = 0; i < VCOM_PORTS; i++){
		char * type;
		char * port;
		char * ip;
		char * rip;
		char * ssl;

		type = port = ip = rip = ssl = 0;

		snprintf(ifname, sizeof(ifname), "/proc/vcom/advproc%d", i);

		if(stat(ifname, &sb)){
			printf("cannot access VCOM interface %s\n", ifname);
			continue;
		}

		if(__cmd_inode_search_pid("vcomd", sb.st_ino, 
					cmd, sizeof(cmd), &cmdlen, 
					&pid)){
			continue;
		}
		
		type = __cmd_get_opts(cmd, cmdlen, "-d");
		port = __cmd_get_opts(cmd, cmdlen, "-p");
		ip = __cmd_get_opts(cmd, cmdlen, "-a");
		ssl = __cmd_get_opts(cmd, cmdlen, "-S");
		rip = __cmd_get_opts(cmd, cmdlen, "-r");

		printf("ttyADV%d PID:%ld %s Dev:%s Port%s IP:%s%s%s\n",
			 i, (long)pid, (ssl)?"TLS":"TCP",type, port, 
			ip, rip?"|":"", rip?rip:"");
	}

	return 0;

}
