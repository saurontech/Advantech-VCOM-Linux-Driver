#build VCOM with TLS support (y/n)
TLS = y
#install VCOM with DKMS (y/n)
DKMS = y
#register VCOM to systemd (y/n)
SYSTEMD = y
#register VCOM DKMS monitor to autostart.desktop (y/n)
DESKAUTOSTART = y

#life-time of the rootCA.pem public key
CA_DAYS = 1024

#life-time for the vcom.pem public key
CERT_DAYS = 1024
