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

		printf("ttyADV%d\t\tPID:%ld\n", i, (long)pid);
	}

	return 0;

}
