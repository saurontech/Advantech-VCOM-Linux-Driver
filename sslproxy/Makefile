all: advsslvcom

advsslvcom: proxy.c json/jstree.c json/jsmn.c json/jsmn.h json/jstree.h json/jstree_readhelper.h
	gcc ./proxy.c ./json/jstree.c ./json/jsmn.c -o advsslvcom -lssl -lcrypto -lpthread -I ./json -Wall
clean:
	rm -f ./advsslvcom
