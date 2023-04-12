#ifndef _PROC_TOOLS_H
#define _PROC_TOOLS_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> 
#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>

int __search_lport_stat_inode(int ipfamily, unsigned short port,  unsigned short stat, ino_t *out);
int __search_port_inode( unsigned short port, int stat, ino_t * out);

int __pidpath_get_cmd(char * pidpath, char * cmd, int cmdlen);
int __pid_get_cmd(pid_t pid, char *cmd, int cmdlen);
char * __cmd_get_opts(char * cmd, int cmdlen, char * opt);

int __pid_search_fd_inode(pid_t pid, ino_t  inode);
int __cmd_inode_search_pid(char * cmd, ino_t inode, char * buf, int buflen, int *retlen, pid_t *pidret);

#define pt_buf_maxstrlen(BUF)	(sizeof(BUF) - (size_t)1)


#endif
