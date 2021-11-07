# Setup TLS for Device Server

If one wishes to use VCOM over TLS, 3 files are needed for each Device Server, which is composed of:
1. a key-pair
2. a diffi-hellman file
3. a rootCA

Back to [README.md](../README.md)
## Creating the TLS files
Use **adv-eki-tls-create** to generate files needed to setup the TLS connection.  
```console
foo@bar~:$ sudo adv-eki-tls-create -n my_EKI_abcd -e my_password_123
```
This command will prepare 2 files, and present the used rootCA: 
* key-pair(pub/priv): **my_EKI_abcd.pem**
* diffi-hellman: **my_EKI_abcd_dh1024.pem**
* rootCA: **/usr/local/advtty/rootCA.pem**

Upload them to the Device Server, setup the password and reboot the device.

| option | discription| value |
|:------:|:------|:-----|
| -h | show help message | N/A | 
| -n | output file name | string | 
| -k | use a custom RootCA private key | path/to/rootCA.key | 
| -p | use a custom RootCA public key | path/to/rootCA.pem | 
| -s | use a custom RootCA serial file | path/to/rootCA.srl |
| -e | secure key-pair with a password | string |
| -l | DSA length | number |
| -L | DH lenght | number |
