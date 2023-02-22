#ifndef _VCOM_MONITOR_H
#define _VCOM_MONITOR_H
#define MSIZE 2048		// 2K size file
#define FNAME_LEN 256
#define CUTTER	"> "
#define MON_MSGLEN_MAX 128

extern void * stk_mon;

struct vc_monitor{
	void * addr;
	int fd;
	int pid;
	int max_statl;
	int dbg_first;
	char fname[FNAME_LEN];
};

extern struct vc_monitor vc_mon;

static inline int mon_init(char * fname)
{
	vc_mon.fd = -1;
	vc_mon.addr = 0;
	vc_mon.max_statl = 0;
	vc_mon.dbg_first = 1;

	if(fname == 0)
		return 0;

	vc_mon.pid  = getpid();
	snprintf(vc_mon.fname, FNAME_LEN, "%s", fname);

	vc_mon.fd = open(vc_mon.fname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXO|S_IRWXG|S_IRWXU);
	if(vc_mon.fd < 0){
		printf("create log fail...\n");
		return -1;
	}

	ftruncate(vc_mon.fd, MSIZE);
	vc_mon.addr = mmap(0, MSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vc_mon.fd, 0);
	if(vc_mon.addr == MAP_FAILED){
		printf("mmap fail\n");
		munmap(vc_mon.addr, MSIZE);
		close(vc_mon.fd);
		vc_mon.fd = 0;
		return -1;
	}
	memset(vc_mon.addr, '\r', MSIZE);
	stk_mon = &vc_mon;
	return 0;
}

static inline int time2str(char *buf, int len)
{
	int tm_strlen;
	struct tm tm_buf;
	struct timeval tv;

	gettimeofday(&tv, 0);
	localtime_r(&tv.tv_sec, &tm_buf);
	tm_strlen = strftime(buf, len, "%F|%T:", &tm_buf);

	return tm_strlen;
}

static inline int mon_update(struct stk_vc * stk, int sig, const char * dbg)
{
	char * mem;
	char * stat;
	char tmp[MON_MSGLEN_MAX];	
	int len;
	int statl;

	if(vc_mon.fd < 0){
		return 0;
	}

	if(stk_empty(stk)){
		printf("stack empty ...\n");
		return -1;
	}

	stat = stk_curnt(stk)->name();
	mem = (char *)vc_mon.addr;
	memset(tmp, ' ', sizeof(tmp));
	statl = snprintf(tmp, sizeof(tmp), "Pid %d|State[%s]\n", 
				vc_mon.pid, stat);

	len = MSIZE - statl;
	if(len <= 1){
		printf("%s len <= 1\n", __func__);
		return -1;
	}

	if(statl > vc_mon.max_statl){
		memmove(mem+statl, mem+vc_mon.max_statl, MSIZE-statl);
		vc_mon.max_statl = statl;
	}
	memcpy(mem, tmp, vc_mon.max_statl);
	memset(tmp, ' ', sizeof(tmp));	
	/* for record debug message */
	if(dbg != 0 ){
		char * ptr;
		int msglen;
		int dbgl;
		msglen = 0;

		ptr = mem + vc_mon.max_statl;

//		msglen = snprintf(tmp, sizeof(tmp), "\n");

		dbgl = time2str(&tmp[msglen], sizeof(tmp) -msglen);
		msglen += dbgl;

		dbgl = snprintf(&tmp[msglen], sizeof(tmp)- msglen, "%s%s\n", CUTTER, dbg);
		msglen += dbgl;

		len = MSIZE - msglen - vc_mon.max_statl;
		if(len <= 1){
			printf("%s len <= 1\n", __func__);
			return -1;
		}

		if(!vc_mon.dbg_first){	
			
			memmove(ptr+msglen, ptr, MSIZE-statl-msglen); 
		}else{
			vc_mon.dbg_first = 0;
		}

		memcpy(ptr, tmp, msglen);
		//memset(ptr+msglen, ' ', 1);
	}
	/* Trigger the inotify */
	if(sig){
		msync(mem, MSIZE, MS_SYNC);
		ftruncate(vc_mon.fd, MSIZE);
	}else{
		msync(mem, MSIZE, MS_ASYNC);
	}

	return 0;
}

#define muc2(a, b)	 	\
	do{if(stk_mon) 		 	\
		mon_update(a, b, 0);	\
	}while(0)


#define muc3(a, b, c)	 	\
	do{if(stk_mon) 		 	\
		mon_update(a, b, c);	\
	}while(0)

#define muc_ovrld(_1, _2, _3, func, ...) func
#define mon_update_check(args...) muc_ovrld(args, muc3, muc2,...)(args)

#endif
