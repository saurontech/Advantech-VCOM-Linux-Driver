check_su:
	if ! [ "$(shell id -u)" != 0 ];then\
		echo "Don't run this target as root";\
		exit 1;\
	fi
add_me: check_su
	mkdir -p ~/.config/autostart
	cp ./advvcom_dkms.desktop ~/.config/autostart
install: 
	cp ./adv_dkms_stat_check /usr/local/advtty/
	chmod 775 /usr/local/advtty/adv_dkms_stat_check


uninstall:
	rm -f /usr/local/advtty/adv_dkms_stat_check

