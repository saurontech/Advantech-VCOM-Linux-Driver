check_su:
	if ! [ "$(shell id -u)" != 0 ];then\
		echo "Don't run this target as root";\
		exit 1;\
	fi
install: check_su
	mkdir -p ~/.config/autostart
	cp ./advvcom_dkms.desktop ~/.config/autostart
	sudo cp ./adv_dkms_stat_check /usr/local/advtty/
	sudo chmod 771 /usr/local/advtty/adv_dkms_stat_check


uninstall: check_su
	rm -f ~/.config/autostart/advvcom_dkms.desktop
	sudo rm -f /usr/local/advtty/adv_dkms_stat_check

