#ifndef _VCOM_MONITER_H
#define _VCOM_MONITER_H
#define DEBUG_MONITOR
#define MSIZE 128
extern void * stk_mon;

struct vc_monitor{
    void * addr;
    int fd;
    int pid;
	int msgl;
	char fname[32];
};
struct vc_monitor vc_mon;

static inline int mon_init(char * fname)
{

	vc_mon.fd = -1;
	vc_mon.addr = 0;

	if(fname <= 0)
		return 0;

	vc_mon.pid  = getpid();
	sprintf(vc_mon.fname, "%s", fname);

	do{
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
			vc_mon.fd = 0;
			return -1;
		}
		stk_mon = &vc_mon;
	}while(0);
	return 0;
}

static inline int mon_update(struct stk_vc * stk, int sig)
{
	char * ptr;
	char * mem;
	char * msg;
	int msgl;
	int len;

	if(vc_mon.fd < 0)
		return 0;
	
	if(stk_empty(stk)){
		printf("stack empty ...\n");
		return -1;
    }

	msg = stk->stk_stat[stk->top]->name();
	mem = (char *)vc_mon.addr;
	msgl = sprintf(mem, "Pid = %d | State : %s  ", 
					vc_mon.pid, msg);
	
	len = MSIZE - msgl;
	if(len <= 1){
		printf("%s len <= 1\n", __func__);
		return -1;
	}
	
	ptr = mem + msgl;
    if(sig){
		msync(ptr, MSIZE, MS_SYNC);
		ftruncate(vc_mon.fd, MSIZE);
    }else{
        msync(ptr, MSIZE, MS_SYNC);
    }
    return 0;
}

#define mon_update_check(a, b)	 	\
	do{if(stk_mon) 		 	\
		mon_update(a, b);	\
	}while(0)

#endif
