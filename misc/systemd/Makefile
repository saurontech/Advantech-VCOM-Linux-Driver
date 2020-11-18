_SYSD_LIB_DIR = /lib/systemd/system
_S_FILE = advvcom.service
_SYSD_SERVICE_PATH = $(_SYSD_LIB_DIR)/$(_S_FILE)

install:
	install -o root -g root -m 644 $(_S_FILE) $(_SYSD_LIB_DIR)

uninstall:
	rm -f $(_SYSD_SERVICE_PATH)

