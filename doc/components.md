# Main components of the Advantech VCOM Linux Driver
The Advantech VCOM Linux Driver is composed of the following components
1. Linux Driver
2. VCOM service daemon
3. Init daemon
4. Managment tools 
5. TLS related CA & key files
6. VCOM mapping config file

After installation, all of the files are copied to "/usr/local/advtty".

## Linux Driver
This is the driver that provides the "/dev/ttyADV*" tty interfaces.  
Its source code is located in the "driver/" directory

## VCOM service daemon
This "vcomd" daemon connects each /dev/ttyADV* with a corresponding Device server serial port.
Eash tty is supported by a "vcomd" daemon.
The source code is located in the "daemon/"

## Init service daemon
This "advttyd" init daemon, reads from the VCOM mapping config file and invokes VCOM service daemons accordingly.
the source code is located in the "init/" directory

## Managment tools
This is a set of tools desinged to simplify the managment process of the **VCOM mapping config file**, **Linux Driver**, **VCOM service daemon**, and **Init daemon**.

### advadd, advrm, and advls  
These tools are used to add, remove and list VCOM connection mappings of the **VCOM mapping config file**, they are bash shell scripts.
These scripts are located in the "script/" directory.

### advman  
A bash script designed to insert/remove the **Linux driver**, invoke/kill the **VCOM service daemons**.  
the script is located in the "script/" directory.

### advps  
This tool is designed to show the status of all the active **VCOM service daemons**.
the source code is located in the "advps/" directory.  

### adv-eki-tls-create  
Located in "scripts/".
This is a bash script designed to create files that are essential for TLS connection.  
they can be used by both the **EKI device server** and the **VCOM service daemon**.  
It can create "Public/Prive key pair", and "Diffie-Hellman file".  

**Three files are essential for creating a TLS connectoin:**  
 1. **RootCA public key**  
     VCOM drivers and Device servers that are connected over TLS must share the same RootCA public key.
 2. **Public/Private key pair**  
     VCOM drivers and Device servers must have their own key pair; however, they must be signed by the same RootCA private key.
 3. **Diffie-Hellman file**   
     Needed by all the Device servers(TLS server) operating under TLS; however, it is not needed by the VCOM driver(TLS client).

## TLS related CA & key files
A few files are created during installation, refer to the **"keys/Makefile"** for details on how they are created.
1. **RootCA.key**: This is the default RootCA private key, which is used to sign **key pairs**, keep this file private and secure.
2. **RootCA.pem**: This is the default RootCA public key. This file will be publicly shared by all the Device servers
3. **vcom.pem**: This is the default public/private key pair. Keep it private, should be accessed by this installation only.

If the VCOM driver connects to a device server, which is already operating with key files signed by another RootCA private key, 
one must replace these files with files created by **adv-eki-tls-create**, referencing the same "RootCA private key".

