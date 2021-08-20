DPATH = ../driver/
all: advttyd
PTPATH = ../shared/proc_tools/

PTSRC = $(shell find ${PTPATH} -name "*.c" -print)
PTHDR = $(shell find ${PTPATH} -name "*.h" -print)

advttyd: advttyd.c advttyd.h $(DPATH)advconf.h ${PTSRC} ${PTHDR}
	$(CC) -Wall -Werror -I $(DPATH) -o $@ advttyd.c -I ${PTPATH} ${PTSRC}

clean:
	find ./ -name 'advttyd' -exec rm {} \;
