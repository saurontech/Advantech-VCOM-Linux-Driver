#build VCOM with TLS support (y/n)
TLS = y
#install VCOM with DKMS (y/n)
DKMS = y
#install register VCOM to systemd (y/n)
SYSTEMD = y

#DSA length for the rootCA.key(private key)
CA_DSA_LEN = 2048
#life-time of the rootCA.pem public key
CA_DAYS = 1024

#DSA length for the vcom.pem public key
CERT_DSA_LEN = 2048
#life-time for the vcom.pem public key
CERT_DAYS = 1024
