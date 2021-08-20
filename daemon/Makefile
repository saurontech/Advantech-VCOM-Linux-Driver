include ../Config.mk

CPATH = $(shell pwd)
DPATH = $(CPATH)/../driver
SPATH = $(CPATH)/../shared/ssl_select
JPATH = $(CPATH)/../shared/json

HDR = $(shell find $(CPATH) -name '*.h' -print)
KHDR = $(shell find $(DPATH) -name '*.h' -print)
SRC = $(shell find  $(CPATH) -name '*.c' -print)

_HDR = 
y_HDR =
n_HDR =

_SRC = 
y_SRC =
n_SRC =


${TLS}_HDR += $(shell find $(SPATH) -name '*.h' -print)
${TLS}_SRC += $(shell find $(SPATH) -name '*.c' -print)
${TLS}_HDR += $(shell find $(JPATH) -name '*.h' -print)
${TLS}_SRC += $(shell find $(JPATH) -name '*.c' -print)

_LIBS =
y_LIBS =
n_LIBS =
$(TLS)_LIBS += -lssl -lcrypto

_DEF =
n_DEF =
y_DEF =
$(TLS)_DEF += -D_VCOM_SUPPORT_TLS

ALLSRC = ${SRC} ${y_SRC}
ALLHDR = ${KHDR} ${HDR} ${y_HDR}
all: vcomd

vcomd: ${ALLSRC} ${ALLHDR}
	${CC} -Wall -Werror -I $(DPATH) -o $@ ${ALLSRC} ${y_LIBS} -I ${SPATH} -I ${JPATH} ${y_DEF}
clean:
	find ./ -name 'vcomd' -exec rm {} \;
