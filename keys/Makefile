include ../Config.mk


all: keys

keys: vcom.pem 

rootCA.key:
	openssl ecparam -name prime256v1 -genkey -out $@
rootCA.pem: rootCA.key
	openssl req -x509 -new -nodes -key $^ -sha256 -days $(CA_DAYS) -out $@ -config ./rootca.conf

vcom.key:
	openssl ecparam -name prime256v1 -out c_prime256v1.pem
	openssl ecparam -in c_prime256v1.pem -genkey -noout -out $@
vcom_pub.pem: vcom.key
	openssl ec -in $^ -pubout -out $@

vcom.csr: 
	openssl ecparam -name prime256v1 -genkey -out vcomec.pem
	openssl req -new -key vcomec.pem -out vcom.csr -config ./vcom.conf

vcom_cert.pem: vcom_pub.pem vcom.csr rootCA.key rootCA.pem
	openssl x509 -req -in vcom.csr -days $(CERT_DAYS) -CAkey ./rootCA.key -CA ./rootCA.pem -force_pubkey vcom_pub.pem -out $@ -CAcreateserial

vcom.pem: vcom_cert.pem vcom.key
	cat $^ > $@

clean:
	rm -f *.pem
	rm -f *.csr
	rm -f *.key
	rm -f *.srl
	rm -f .srl


