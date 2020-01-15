INSTALL_PATH = /usr/local/advtty/
MODNAME = advvcom
VERSION = 1

all:
	make -C ./daemon
	make -C ./driver
	make -C ./initd
	make -C ./inotify
	make -C ./advps
	make -C ./sslproxy
	make -C ./keys
clean:
	make clean -C ./driver
	make clean -C ./daemon
	make clean -C ./initd
	make clean -C ./inotify
	make clean -C ./advps
	make clean -C ./sslproxy
	make clean -C ./keys

./keys/rootCA.srl:
	mv ./keys/.srl ./keys/rootCA.srl

install_ssl: ./keys/rootCA.srl
	cp ./sslproxy/advsslvcom $(INSTALL_PATH)
	cp ./sslproxy/config.json $(INSTALL_PATH)
	cp ./keys/rootCA.key $(INSTALL_PATH)
	cp ./keys/rootCA.pem $(INSTALL_PATH)
	cp ./keys/rootCA.srl $(INSTALL_PATH)
	cp ./keys/vcom.pem $(INSTALL_PATH)
	cp ./script/adv-eki-tls-create $(INSTALL_PATH)
	chmod 111 $(INSTALL_PATH)adv-eki-tls-create
	ln -sf $(INSTALL_PATH)advsslvcom /sbin/advsslvcom
	ln -sf $(INSTALL_PATH)adv-eki-tls-create /sbin/adv-eki-tls-create

install_daemon:
	install -d $(INSTALL_PATH)
	cp ./daemon/vcomd $(INSTALL_PATH)
	cp ./initd/advttyd $(INSTALL_PATH)
	cp ./config/advttyd.conf $(INSTALL_PATH)
	cp ./Makefile  $(INSTALL_PATH)
	cp ./script/advls $(INSTALL_PATH)
	cp ./script/advadd $(INSTALL_PATH)
	cp ./script/advrm $(INSTALL_PATH)
	cp ./script/advman $(INSTALL_PATH)
	cp ./inotify/vcinot $(INSTALL_PATH)
	cp ./advps/advps $(INSTALL_PATH)
	chmod 111 $(INSTALL_PATH)advls
	chmod 111 $(INSTALL_PATH)advadd
	chmod 111 $(INSTALL_PATH)advrm
	chmod 111 $(INSTALL_PATH)advman
	chmod 111 $(INSTALL_PATH)vcinot
	chmod 111 $(INSTALL_PATH)advps
	ln -sf $(INSTALL_PATH)advls /sbin/advls
	ln -sf $(INSTALL_PATH)advrm /sbin/advrm
	ln -sf $(INSTALL_PATH)advadd /sbin/advadd
	ln -sf $(INSTALL_PATH)advman /sbin/advman
	ln -sf $(INSTALL_PATH)vcinot /sbin/vcinot
	ln -sf $(INSTALL_PATH)advps /sbin/advps
	

install: install_daemon install_ssl
	cp ./driver/advvcom.ko $(INSTALL_PATH)
	
uninstall:
	rm -Rf $(INSTALL_PATH)
	rm -f /sbin/advrm
	rm -f /sbin/advls
	rm -f /sbin/advman
	rm -f /sbin/advadd
	rm -f /sbin/vcinot
	rm -f /sbin/advps
	rm -f /sbin/advsslvcom
	rm -f /sbin/adv-eki-tls-create
	
# use dkms
install_dkms: install_daemon
	make -C ./driver clean
	dkms add ./driver
	dkms build -m $(MODNAME) -v $(VERSION)
	dkms install -m $(MODNAME) -v $(VERSION)

uninstall_dkms: uninstall
	-dkms uninstall -m $(MODNAME) -v $(VERSION)
	dkms remove -m $(MODNAME) -v $(VERSION) --all
	rm -rf /usr/src/$(MODNAME)-$(VERSION)
