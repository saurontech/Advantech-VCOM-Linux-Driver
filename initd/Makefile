DPATH = ../driver/
all: advttyd

advttyd: advttyd.c advttyd.h $(DPATH)advconf.h
	$(CC) -Wall -I $(DPATH) -o $@ advttyd.c

clean:
	find ./ -name 'advttyd' -exec rm {} \;
