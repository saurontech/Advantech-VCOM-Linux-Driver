CPATH = $(shell pwd)
DPATH = $(CPATH)/../driver
SPATH = $(CPATH)/../shared/ssl_select
JPATH = $(CPATH)/../shared/json

HDR = $(shell find $(CPATH) -name '*.h' -print)
KHDR = $(shell find $(DPATH) -name '*.h' -print)
SRC = $(shell find  $(CPATH) -name '*.c' -print)
S_HDR = $(shell find $(SPATH) -name '*.h' -print)
S_SRC = $(shell find $(SPATH) -name '*.c' -print)
J_HDR = $(shell find $(JPATH) -name '*.h' -print)
J_SRC = $(shell find $(JPATH) -name '*.c' -print)

ALLSRC = ${SRC} ${S_SRC} ${J_SRC}
ALLHDR = ${KHDR} ${HDR}  ${S_HDR} ${J_HDR}

all: vcomd

vcomd: ${ALLSRC} ${ALLHDR}
	$(CC) -Wall -Werror -I $(DPATH) -o $@ ${ALLSRC} ${LIBS} -I ${SPATH} -lssl -lcrypto -I ${JPATH}
clean:
	find ./ -name 'vcomd' -exec rm {} \;
