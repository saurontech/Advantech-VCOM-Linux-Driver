include ../Config.mk

CPATH = $(shell pwd)
DPATH = $(CPATH)/../driver
SPATH = $(CPATH)/../shared/ssl_select
JPATH = $(CPATH)/../shared/json
CONTAINER_PATH = $(CPATH)/../shared/container

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


CONTAINER_HDR = $(shell find $(CONTAINER_PATH) -name '*.h' -print)
CONTAINER_SRC = $(shell find $(CONTAINER_PATH) -name '*.c' -print)


_DEF =
n_DEF =
y_DEF =
$(TLS)_DEF += -D_VCOM_SUPPORT_TLS

ALLSRC = ${SRC} ${y_SRC} ${CONTAINER_SRC}
ALLHDR = ${KHDR} ${HDR} ${y_HDR} ${CONTAINER_HDR}

_INCLUDE =
y_INCLUDE =
n_INCLUDE =

y_INCLUDE += -I ${DPATH} -I ${CONTAINER_PATH} -I ${JPATH}
${TLS}_INCLUDE += -I ${SPATH}
y_INCLUDE += $(shell find ./ -type d -printf ' -I ./%P ' )

all: vcomd

vcomd: ${ALLSRC} ${ALLHDR}
	${CC} -Wall -Werror ${y_INCLUDE} -o $@ ${ALLSRC} ${y_LIBS} ${y_DEF}

clean:
	find ./ -name 'vcomd' -exec rm {} \;
