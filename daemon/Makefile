CPATH = $(shell pwd)
DPATH = $(CPATH)/../driver
HDR = $(shell find $(CPATH) -name '*.h' -print)
KHDR = $(shell find $(DPATH) -name '*.h' -print)
SRC = $(shell find  $(CPATH) -name '*.c' -print)
all: vcomd

vcomd: ${SRC} ${KHDR} ${HDR}
	gcc -Wall -I $(DPATH) -o $@ ${SRC} 
clean:
	find ./ -name 'vcomd' -exec rm {} \;
