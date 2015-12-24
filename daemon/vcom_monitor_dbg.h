#ifndef _VCOM_MONITOR_H
#define _VCOM_MONITOR_H
#define MSIZE 1024		// 1K size file
#define FNAME_LEN 256
#define CUTTER	"> "

extern void * stk_mon;

struct vc_monitor{
	void * addr;
	int fd;
	int pid;
	int max_statl;
	int dbg_first;
	char fname[FNAME_LEN];
};
struct vc_monitor vc_mon;

static inline int mon_init(char * fname)
{
	vc_mon.fd = -1;
	vc_mon.addr = 0;
	vc_mon.max_statl = 0;
	vc_mon.dbg_first = 1;

	if(fname <= 0)
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
	memset(vc_mon.addr, ' ', MSIZE);
	stk_mon = &vc_mon;
	return 0;
}

static inline int mon_update(struct stk_vc * stk, int sig, const char * dbg)
{
	char * ptr;
	char * mem;
	char * stat;
	char tmp[64];	
	int len;
	int statl;
	int dbgl;

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
	statl = snprintf(tmp, sizeof(tmp), "Pid %d | State [%s] ", 
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
	if(dbg != 0 && sig){
		ptr = mem + vc_mon.max_statl;
		dbgl = snprintf(tmp, sizeof(tmp), "%s%s", CUTTER, dbg);
		
		len = MSIZE - dbgl - vc_mon.max_statl;
		if(len <= 1){
			printf("%s len <= 1\n", __func__);
			return -1;
		}

		if(!vc_mon.dbg_first){	
			memmove(ptr+dbgl+1, ptr, MSIZE-statl-dbgl); 
		}else{
			vc_mon.dbg_first = 0;
		}
		memcpy(ptr, tmp, dbgl);
		memset(ptr+dbgl, ' ', 1);
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
