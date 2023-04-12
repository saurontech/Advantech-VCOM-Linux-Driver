#include "proc_tools.h"

// TCP_ESTABLISHED = 1,
// TCP_LISTEN = 10
int __search_lport_stat_inode(int ipfamily, unsigned short port,  unsigned short stat, ino_t *out)
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
	char scan_format[512];
	ino_t inode;
	int i;

	snprintf(path, sizeof(path), "/proc/net/tcp%s", ipfamily==4?"":"6");
	//printf("path = %s\n", path);
	snprintf(scan_format, sizeof(scan_format),
			"%%d: %%%zus %%%zus %%x %%%zus %%%zus %%%zus %%%zus %%%zus %%ju",
			pt_buf_maxstrlen(laddr), 
			pt_buf_maxstrlen(raddr), 
			pt_buf_maxstrlen(tmp),
			pt_buf_maxstrlen(tmp), 
			pt_buf_maxstrlen(tmp), 
			pt_buf_maxstrlen(tmp), 
			pt_buf_maxstrlen(tmp));

	found = 0;

	fp = fopen(path, "r");
	
	//printf("fp= %x\n", fp);
	if(fp == NULL){
		return -1;
	}
	// skip first two lines
	for (i = 0; i < 1; i++) {
		fgets(buf, sizeof(buf), fp);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		//printf("buf(%d) = %s\n", buf, strlen(buf));
		
		ret = sscanf(buf, scan_format, 
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
		//printf("ret = %d sl = %d laddr = %s port = %u state = %u inode= %d\n", ret, sl, laddr, lport, connection_state, inode);
		if(lport == port){
			found = 1;
			break;
		}
	}
	fclose(fp);

	if(found){
		*out = inode;
		return 0;
	}

	return -1;

}

int __search_port_inode( unsigned short port, int stat, ino_t * out)
{
	ino_t inode;
	int ret;
	do{
		ret = __search_lport_stat_inode(4, port, stat, &inode);
		if(ret == 0){
			//printf("found tcp4 inode = %d", inode);
			break;
		}

		ret = __search_lport_stat_inode(6, port, stat, &inode);
		if(ret == 0){
			//printf("found tcp4 inode = %d", inode);
			break;
		}
		return -1;
	}while(0);

	*out = inode;

	return 0;
}

//search if pid is operating this inode
//returns 0 on error
int __pid_search_fd_inode(pid_t pid, ino_t  inode)
{
	DIR *dir;
	struct dirent *entry;
	char *name;
	int fd;
	char status[PATH_MAX];
	char fdpath[PATH_MAX];
	struct stat sb;
	snprintf(fdpath, sizeof(fdpath), "/proc/%d/fd", pid);
	dir = opendir(fdpath);
	if(!dir){
		printf("Can't open /proc/fd: %s", fdpath);
		return -1;
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

		snprintf(status, sizeof(status), "/proc/%d/fd/%d", pid, fd);

		if(stat(status, &sb)){
			//printf( "stat failed\n");
			continue;
		}
		//printf("inode %ju st_ino = %ju\n", sb.st_ino, inode);
		if(sb.st_ino == inode){
			//printf("found inode %ju  at pid %d\n", sb.st_ino, pid);
			closedir(dir);
			return 0;
		}
		
	}

}

int __pidpath_get_cmd(char * pidpath, char * cmd, int cmdlen)
{
	int fd;
	int cnt;

	if((fd = open(pidpath, O_RDONLY)) < 0){
		printf( "%s(%d)fopen failed", __func__, __LINE__);
		return -1;
	}

	cnt = read(fd, cmd, cmdlen);
	close(fd);

	if(cnt <= 0){
		return -1;
	}

	return cnt;
}


int __pid_get_cmd(pid_t pid, char * cmd, int cmdlen)
{
	char pidpath[PATH_MAX];

	snprintf(pidpath, sizeof(pidpath), "/poc/%lu/cmdline", (unsigned long)pid);
	return __pidpath_get_cmd(pidpath, cmd, cmdlen);
}

int __cmd_inode_search_pid(char * cmd, ino_t inode, char * buf, int buflen, int *retlen, pid_t *pidret)
{
	DIR *dir;
	struct dirent *entry;
	char *name;
	char status[PATH_MAX];
	struct stat sb;
	long pid; //pid is signed on linux

	dir = opendir("/proc");
	if(!dir){
		printf("Can't open /proc");
		return -1;
	}

	for(;;) {

		int cmdlen;
		if((entry = readdir(dir)) == NULL) {
			//printf( "(%d/%d)no %s were found operating %d", found_vcomd, i, cmd, inode);
			closedir(dir);
			return -1;
		}

		name = entry->d_name;
		if (!(*name >= '0' && *name <= '9'))
			continue;


		pid = atol(name);

		snprintf(status, sizeof(status), "/proc/%ld", pid);
		if(stat(status, &sb)){
			continue;
		}
		snprintf(status, sizeof(status), "/proc/%ld/cmdline", pid);

		cmdlen = __pidpath_get_cmd(status, buf, buflen);

		if(cmdlen <= 0){
			continue;
		}

		if(cmd == 0 || strstr(buf, cmd) > 0){
			if(__pid_search_fd_inode(pid, inode) == 0){
				*retlen = cmdlen;
				
				closedir(dir);
				*pidret = (pid_t)pid;
				return 0;
			}else{
				//printf("pid %dcmd %s\n", pid, cmd);
			}
		}
	}
}

char * __cmd_get_opts(char * cmd, int cmdlen, char * opt)
{
	int i;
	char *ptr;

	i = 0;
	do{
		ptr = strstr(&cmd[i], opt);

		if(ptr > 0){
			ptr += strlen(opt);

			if(strcmp(&cmd[i], opt) == 0){
				ptr++;				
			}
			break;
		}
		i += strlen(&cmd[i]);
		
	}while(++i < cmdlen);

	return ptr;
}

