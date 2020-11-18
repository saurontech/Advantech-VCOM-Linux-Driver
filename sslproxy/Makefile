all: advsslvcom

advsslvcom: proxy.c json/jstree.c json/jsmn.c json/jsmn.h json/jstree.h json/jstree_readhelper.h ./proxy_dbg.h
	$(CC) ./proxy.c ./json/jstree.c ./json/jsmn.c -o advsslvcom -lssl -lcrypto -lpthread -I ./json -Wall -Werror
clean:
	rm -f ./advsslvcom
