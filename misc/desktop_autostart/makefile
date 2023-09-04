check_su:
	if ! [ "$(shell id -u)" != 0 ];then\
		echo "Don't run this target as root";\
		exit 1;\
	fi
install: check_su
	mkdir -p ~/.config/autostart
	cp ./advvcom_dkms.desktop ~/.config/autostart

uninstall: check_su
	rm -f ~/.config/autostart/advvcom_dkms.desktop

