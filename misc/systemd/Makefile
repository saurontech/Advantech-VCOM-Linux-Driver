_SYSD_LIB_DIR = /lib/systemd/system
_SYSD_LIB_DIR_2 = /usr/lib/systemd/system
_S_FILE = advvcom.service
_SYSD_SERVICE_PATH = $(_SYSD_LIB_DIR)/$(_S_FILE)
_SYSD_SERVICE_PATH_2 = $(_SYSD_LIB_DIR_2)/$(_S_FILE)


install:
	if [ -d "$(_SYSD_LIB_DIR)" ]; then \
		install -o root -g root -m 644 $(_S_FILE) $(_SYSD_LIB_DIR); \
	else  \
		install -o root -g root -m 644 $(_S_FILE) $(_SYSD_LIB_DIR_2); \
	fi

uninstall:
	rm -f $(_SYSD_SERVICE_PATH)
	rm -f $(_SYSD_SERVICE_PATH_2)

