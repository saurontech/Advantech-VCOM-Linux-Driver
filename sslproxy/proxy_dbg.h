#define FNAME_LEN 1024
#define MSIZE 20480

typedef struct dbg_mon_t{
	void * addr;
	int fd;
	int msgl;
	char fname[FNAME_LEN];
}dbg_mon;

static inline dbg_mon * pmon_open(char * fname)
{
	dbg_mon * dbg;

	dbg = malloc(sizeof(dbg_mon));
	if(dbg == 0){
		return 0;
	}

	dbg->fd = -1;
	dbg->addr = 0;

	if(fname <= 0){
		free(dbg);
		return 0;
	}

	snprintf(dbg->fname, FNAME_LEN, "%s", fname);

	dbg->fd = open(dbg->fname, O_RDWR | O_CREAT , S_IRWXO|S_IRWXG|S_IRWXU);
	if(dbg->fd < 0){
		printf("create log fail...\n");
		free(dbg);
		return 0;
	}

	ftruncate(dbg->fd, MSIZE);
	dbg->addr = mmap(0, MSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dbg->fd, 0);
	if(dbg->addr == MAP_FAILED){
		printf("mmap fail\n");
		//munmap(dbg->addr, MSIZE);
		close(dbg->fd);
		dbg->fd = 0;
		free(dbg);
		return 0;
	}

	return dbg;
	//return 0;
}

static inline int pmon_close(dbg_mon * dbg)
{
	if(dbg == 0){
		return 0;
	}

	if(dbg->fd == 0){
		free(dbg);
		return 0;
	}

	munmap(dbg->addr, MSIZE);
	close(dbg->fd);
	free(dbg);
	return 0;
}



static inline int pmon_log(dbg_mon * dbg, /*int sig, int time,*/ char * __form, ...)
{
	char * mem;
	int len;
	char tmp[1024];
	struct tm tm_buf;
	struct timeval tv;
	char tm_str[128];
	int tm_strlen;

	va_list args;
	

	if(dbg == 0 || dbg->fd < 0){
		return 0;
	}

	tm_strlen = 0;
//	if(time){
		gettimeofday(&tv, 0);
		localtime_r(&tv.tv_sec, &tm_buf);
		tm_strlen = strftime(tm_str, sizeof(tm_str), "%F|%T:", &tm_buf);
//	}

	mem = (char *)dbg->addr;

	va_start(args, __form);
	len = vsnprintf(tmp, MSIZE, __form, args);
	va_end(args);
	
	memmove(dbg->addr + len + tm_strlen, dbg->addr, MSIZE - len - tm_strlen);
	if(tm_strlen > 0){
		memcpy(dbg->addr, tm_str, tm_strlen);
	}
	memcpy(dbg->addr + tm_strlen, tmp, len);
	mem = dbg->addr;
	mem[MSIZE -1] ='\0';
	/*msgl = snprintf(mem, MSIZE, "Pid : %d | State : %s ",
			vc_mon.pid, msg);

	len = MSIZE - msgl;

	if(len <= 1){
		printf("%s len <= 1\n", __func__);
		return -1;
	}

	ptr = mem + msgl;
	if(dbg != 0)
		msgl += snprintf(ptr, len, "| E : %s   \n", dbg);

	len = MSIZE - msgl;
	ptr = mem + msgl;
	memset(ptr + 1, 0, len - 1);
	*/
	msync(mem, MSIZE, MS_SYNC);
/*	if(sig){
		ftruncate(dbg->fd, MSIZE);
	}*/

	return 0;
}
/*
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
*/
