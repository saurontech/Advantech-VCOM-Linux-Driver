#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "advconf.h"

int _pid_findfd(int pid, char * file)
{
	static DIR *dir = 0;
	struct dirent *entry;
	char *name;
	int n;
	int fd;
	char status[1204];
	char buf[1024];
	char fdpath[1024];
	FILE *fp;
	struct stat sb;
	char fdname[1024];
	snprintf(fdpath, 1024, "/proc/%d/fd", pid);
	if (!dir) {
		dir = opendir(fdpath);
		if(!dir){
			printf("Can't open /proc/fd: %s\n", fdpath);
			return -1;
		}
	}
	for(;;) {
		if((entry = readdir(dir)) == NULL) {
	//		printf("can't open dir in pidfindfd\n");
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
			printf("get stat\n");
			continue;
		}
		if(lstat(status, &sb) < 0){
			printf("lstat failed\n");
			continue;
		}

		n = readlink(status, buf, sizeof(buf));

		if(n <= 0){
			printf("n = %d\n", n);
			closedir(dir);
			return -1;
		}

		if(n !=  strlen(file) ){
			continue;
		}

		if(memcmp(buf, file, strlen(file)) == 0){
			closedir(dir);
			dir = 0;
			return pid;
		}
	}

}

int proc_findfd(char * cmd, char * file)
{
	static DIR *dir = 0;
	struct dirent *entry;
	char *name;
	int n;
	char status[32];
	char buf[1024];
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
			closedir(dir);
			dir = 0;
			return -1;
		}
		name = entry->d_name;
		if (!(*name >= '0' && *name <= '9'))
			continue;


		pid = atoi(name);

		if(cmd != 0){
			sprintf(status, "/proc/%d/cmdline", pid);
			if((fp = fopen(status, "r")) == NULL)
				continue;

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
			if(strstr(buf, cmd) == 0){
				continue;
			}
		}

		sprintf(status, "/proc/%d", pid);
		if(stat(status, &sb))
			continue;

		sprintf(status, "/proc/%d/fd", pid);
		if(stat(status, &sb))
			continue;

		if(_pid_findfd(pid, file) > 0){
			closedir(dir);
			dir = 0;
			return pid;
		}
	}
}

int main(int argc, char *argv[])
{
	int i;
	int pid;
	char ifname[1024];
	
		
	for(i = 0; i < VCOM_PORTS; i++){
		snprintf(ifname, sizeof(ifname), "/proc/vcom/advproc%d", i);
		pid = proc_findfd("vcomd", ifname);
		if(pid >= 0){
			printf("ttyADV%d\t\tPID:%d\n", i, pid);
		}
	}

	return 0;

}
