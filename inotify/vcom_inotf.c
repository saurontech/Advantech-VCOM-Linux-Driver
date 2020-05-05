#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/inotify.h>

#define EVENT_SIZE	(sizeof(struct inotify_event))
#define EVENT_BUF_LEN	 (1024 * (EVENT_SIZE + 16))

#define FPATH_LEN		256


int parse_ievent(char *buf, int *i, const char *path)
{
#define CMDBUF_LEN		256
	char fpath[FPATH_LEN];
	char cmd[CMDBUF_LEN];
	struct inotify_event *event = (struct inotify_event *)&buf[*i];
	

	if(event->mask & IN_MODIFY){
		printf("state change: ");

		if(event->len > 0){		/* inotify a folder */
			snprintf(fpath, FPATH_LEN, "%s/%s", path, event->name);
			printf("%s\n", fpath);
			snprintf(cmd, CMDBUF_LEN, "logger -t %s -f %s", event->name, fpath);

		}else{	/* inotify a file */
			printf("%s\n", path);
			snprintf(cmd, CMDBUF_LEN, "logger -f %s", path);

		}
		system(cmd);

	}else{
		printf("unknown event = %x\n", event->mask);
	}

	*i += EVENT_SIZE + event->len;
#undef CMDBUF_LEN

	return 0;
}

void usage(char * cmd)
{
	printf("usage : %s [-p <PATH>] [-l] [-h]\n", cmd);
	printf("The most commonly used commands are:\n");
	printf("	-p	Monitoring file path\n");
	printf("	-l	Execute forever\n");
	printf("	-h	For help\n");
}

int main(int argc, char **argv)
{
	int len, i, loop;
	int fd;
	int wd;
	char buf[EVENT_BUF_LEN];
	int ch;
	char path[FPATH_LEN];

	if(argc < 2){
		usage(argv[0]);
		return -1;
	}

	path[0] = '\0';

	loop = 0;
	while((ch = getopt(argc, argv, "lhp:")) != -1){
		switch(ch){
			case 'l':
				loop = 1;	
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			case 'p':
				snprintf(path, FPATH_LEN, "%s", optarg);
				break;
			default:
				printf("Illegal argument, please type -h for help\n"); 
				return -1;
		}
	}
	/* creating the INOTIFY instance */
	if(strlen(path) == 0){
		usage(argv[0]);
		return -1;
	}

	fd = inotify_init();
	if (fd < 0) {
		perror("inotify_init");
		return -1;
	}
	/* add whatch */
	wd = inotify_add_watch( fd, path, IN_MODIFY );
	if(wd < 0){
		perror("inotify_add_watch");
		return -1;
	}
	do{
		i = 0;

		len = read( fd, buf, EVENT_BUF_LEN ); 
		if (len < 0){
			printf("read failed\n");
		}  
		/*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
		while (i < len){  
			if(parse_ievent(buf, &i, path) == -1){
				printf("parse_ievent failed\n");
				return -1;
			}
		}
	}while(loop);
	/*removing the directory/file from the watch list.*/
	inotify_rm_watch(fd, wd);
	/*closing the INOTIFY instance*/
	close(fd);
	return 0;
}
