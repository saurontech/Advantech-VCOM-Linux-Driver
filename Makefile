include Config.mk

INSTALL_PATH = /usr/local/advtty/
MODNAME = advvcom
VERSION = 1

_build =
_install =
y_build=
y_install=
y_build = build_basic
y_install = install_daemon
y_uninstall =

$(TLS)_build	+= build_ssl
$(TLS)_install	+= install_ssl
$(DKMS)_install	+= install_dkms
$(DKMS)_uninstall	+= uninstall_dkms
$(SYSTEMD)_install += install_systemd
$(SYSTEMD)_uninstall += uninstall_systemd

ifneq ($(DKMS), y)
y_install += install_driver
endif

all: $(y_build)
	
build_basic:
	make -C ./daemon
	make -C ./driver
	make -C ./initd
	make -C ./inotify
	make -C ./advps

build_ssl:
	make -C ./keys
	
clean:
	make clean -C ./driver
	make clean -C ./daemon
	make clean -C ./initd
	make clean -C ./inotify
	make clean -C ./advps
	make clean -C ./keys

cleanup_srl:
	if [ -s ./keys/rootCA.srl ]; then \
		echo "nothing to cleanup for"; \
	else \
		if [ -s ./keys/.srl ]; then \
			echo "using old OpenSSL"; mv ./keys/.srl ./keys/rootCA.srl; \
		else \
			echo "using OpenSSL 3"; touch ./keys/rootCA.srl; \
		fi \
	fi

install_ssl: cleanup_srl
	cp ./config/ssl.json $(INSTALL_PATH)
	cp ./keys/rootCA.key $(INSTALL_PATH)
	cp ./keys/rootCA.pem $(INSTALL_PATH)
	cp ./keys/rootCA.srl $(INSTALL_PATH)
	cp ./keys/vcom.pem $(INSTALL_PATH)
	cp ./script/adv-eki-tls-create $(INSTALL_PATH)
	chmod 111 $(INSTALL_PATH)adv-eki-tls-create
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
	ln -sf $(INSTALL_PATH)vcomd /sbin/vcomd
	
install_driver:
	cp ./driver/advvcom.ko $(INSTALL_PATH)

install: $(y_install)
	
uninstall: $(y_uninstall)
	rm -Rf $(INSTALL_PATH)
	rm -f /sbin/advrm
	rm -f /sbin/advls
	rm -f /sbin/advman
	rm -f /sbin/advadd
	rm -f /sbin/vcinot
	rm -f /sbin/advps
	rm -f /sbin/vcomd
	rm -f /sbin/adv-eki-tls-create
	
# use dkms
install_dkms: 
	make -C ./driver clean
	dkms add ./driver
	dkms build -m $(MODNAME) -v $(VERSION)
	dkms install -m $(MODNAME) -v $(VERSION)

uninstall_dkms:
	- dkms uninstall -m $(MODNAME) -v $(VERSION)
	dkms remove -m $(MODNAME) -v $(VERSION) --all
	rm -rf /usr/src/$(MODNAME)-$(VERSION)

install_systemd:
	make install -C ./misc/systemd
	systemctl enable advvcom.service

uninstall_systemd:
	systemctl stop advvcom.service
	systemctl disable advvcom.service
	make uninstall -C ./misc/systemd
	
