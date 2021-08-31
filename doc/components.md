# Main components of the Advantech VCOM Linux Driver
The Advantech VCOM Linux Driver is composed of the following components

| No. | Component Name |File Name(s) |Source code Location|  
|:---:|:---------------|:---------|:-------------------|  
|1. | Linux Driver| advvcom(DKMS) or advvcom.ko(without DKMS) | driver/ |
|2. | VCOM service daemon | vcomd | daemon/ |
|3. | Init daemon | advttyd | init/ |
|4. | Managment tools | advrm, advadd, advls, advman, advps, adv-eki-tls-create | scripts/ and advps/ |
|5. | TLS related CA & key files | rootCA.key, rootCA.pem, vcom.pem | keys/Makefile |
|6. | VCOM mapping config file | advttyd.conf | config/ |
|7. | TLS config file | ssl.json | config/ |

After installation, all of the files are copied to **"/usr/local/advtty"**.  

Go Back to [README.md](../README.md)
## Linux Driver
This is the driver that provides the "/dev/ttyADV*" tty interfaces.  
Its source code is located in the "driver/" directory

## VCOM service daemon
This "service daemon" connects each /dev/ttyADV* with a corresponding Device Server serial port.
Eash tty is supported by a "vcomd" daemon.
The source code is located in the "daemon/"

## Init service daemon
This "init daemon" reads from the "VCOM mapping config file" and invokes VCOM service daemons accordingly.  
the source code is located in the "init/" directory

## Managment tools
This is a set of tools desinged to simplify the managment process of the "**VCOM mapping config file**", "**Linux Driver**", "**VCOM service daemon**", and "**Init daemon**".

### advadd, advrm, and advls  
These are bash scripts used to add, remove and list VCOM connection mappings of the **VCOM mapping config file**.  

### advman  
A bash script designed to insert/remove the **Linux driver**, invoke/kill the **VCOM service daemons**.  

### advps  
A tool designed to show the status of all the active **VCOM service daemons**.  

### adv-eki-tls-create  
This bash script is designed to create files that are essential for TLS connection.  
they can be used by both the **EKI device server** and the **VCOM service daemon**.  
It can create "Public/Prive key pair", and "Diffie-Hellman file".  

**Three files are essential for creating a TLS connectoin:**  
 1. **RootCA public key**  
     VCOM drivers and Device servers that are connected over TLS must share the same RootCA public key.
 2. **Public/Private key pair**  
     VCOM drivers and Device servers must have their own "key pair"; however, they must be signed by the same "RootCA private key".
 3. **Diffie-Hellman file**   
     Needed by all the Device servers(TLS server) operating under TLS; however, it is not needed by the VCOM driver(TLS client).

## TLS related CA & key files
A few files are created during installation, refer to the **"keys/Makefile"** for details on how they are created.  
1. **RootCA.key**: This is the default RootCA private key, which is used to sign **"key pairs"**. Keep this file private and secure.
2. **RootCA.pem**: This is the default RootCA public key. This file will be publicly shared by all the Device servers
3. **vcom.pem**: This is the default public/private key pair. Keep it private, should be accessed by this installation only.

If the VCOM driver connects to a device server, which is already operating with key files signed by another RootCA private key, 
one must replace these files with files created by **adv-eki-tls-create**, referencing the same "RootCA private key".

## VCOM mapping config file
This file is read by the "init daemon"; the "init daemon" will create "VCOM service daemons" according to this configure file.

## TLS config file
This file is read by the "VCOM service daemon"; it defines the location of the "RootCA public key", and "keypair file", it also defines the "password" if it ever encounters a **encripted "keypair file"**.
