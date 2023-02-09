# Create TLS files for the VCOM Driver
Occasionally, one might want to create new TLS files for a VCOM driver.  
For example, creating a "key-pair" to setup a new host, or building a whole set of TLS CA & key files to replace an expired CA.  
This document is the instruction guide on how to do both.  

Go back to [README.md](../README.md)

## Create a new "public/private key pair" for a VCOM driver
To create a new "key pair" for a VCOM driver, one can use the command "adv-eki-tls-create".  
On the original host holding the desired "RootCA private key":
```console
foo@bar:# sudo adv-eki-tls-create -n new_vcom_driver
...
---
Copy the following files to the corresponding EKI device server

* key-pair(pub/priv): new_vcom_driver.pem 
* diffi-hellman: new_vcom_driver_dh1024.pem
* rootCA: /usr/local/advtty/rootCA.pem
```
The "key pair" and "rootCA" will be needed.

On the destination host:
```console
foo@bar:# sudo cp ./new_vcom_driver.pem /usr/local/advtty/vcom.pem
foo@bar:# sudo cp ./rootCA.pem /usr/local/advtty/rootCA.pem
foo@bar:# sudo advman -o restart
```

# Create a new set "RootCA public/private key" and "key-pair files"
The "keys/Makefile" can help you create a new set of TLS files.
```console
foo@bar:# cd ./keys
foo@bar:keys# make clean
foo@bar:keys# make
foo@bar:keys# sudo cp ./RootCA.key /usr/local/advtty/
foo@bar:keys# sudo cp ./RootCA.pem /usr/local/advtty/
foo@bar:keys# sudo cp ./vcom.pem /usr/local/advtty/
foo@bar:keys# sudo advman -o restart
```
