all: keys

keys: vcom.pem

rootCA.key:
	openssl dsaparam -genkey -out $@ 1024
	
rootCA.pem: rootCA.key
	openssl req -x509 -new -nodes -key $^ -sha256 -days 1024 -out $@ -config ./rootca.conf

vcom.key:
	openssl ecparam -name prime256v1 -out c_prime256v1.pem
	openssl ecparam -in c_prime256v1.pem -genkey -noout -out $@
vcom_pub.pem: vcom.key
	openssl ec -in $^ -pubout -out $@

vcom.csr: 
	openssl dsaparam -genkey -out vcomdsa.pem 1024
	openssl req -new -key vcomdsa.pem -out vcom.csr -config ./vcom.conf

vcom_cert.pem: vcom_pub.pem vcom.csr rootCA.key rootCA.pem
	openssl x509 -req -in vcom.csr -CAkey ./rootCA.key -CA ./rootCA.pem -force_pubkey vcom_pub.pem -out $@ -CAcreateserial

vcom.pem: vcom_cert.pem vcom.key
	cat $^ > $@
clean:
	rm -f *.pem
	rm -f *.csr
	rm -f *.key
	rm -f *.srl
