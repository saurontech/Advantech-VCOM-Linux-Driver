PTPATH = ../shared/proc_tools/
PTSRC = $(shell find ${PTPATH} -name "*.c" -print)
PTHDR = $(shell find ${PTPATH} -name "*.h" -print)

all: advps.c ${PTSRC} ${PTHDR}
	$(CC) -Wall -Werror -o advps $< -I ../driver -I ${PTPATH} ${PTSRC}
clean:
	rm -f ./advps

