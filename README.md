# Advantech Linux VCOM driver
## Introduction
The Linux VCOM driver for Advantech Device Servers
Support extended featurs:
- TLS encription
- DKMS integration
- systemd integration
- Advanced system monitoring
  - VCOM and TLS status monitoring: /tmp/advmon/advtty*
  ```console
  foo@bar~:$ cat /tmp/advmon/advtty0
  ```
- syslog integration
  -  logging **/tmp/advcom/** events to syslog
  ```console
  foo@bar~:$ vcinot -p /tmp/advmon -l &  
  ```

For details on the infrastructure and main components of this VCOM driver, check [here](doc/components.md) .

### Quality
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/66cde2a55b884e1e8a98adac7556e503)](https://www.codacy.com/gh/saurontech/Advantech-VCOM-Linux-Driver/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=saurontech/Advantech-VCOM-Linux-Driver&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/saurontech/advantech-vcom-linux-driver/badge/main)](https://www.codefactor.io/repository/github/saurontech/advantech-vcom-linux-driver/overview/main)
## Installation Guide
### Install Dependancy
This driver depends on:
1. kernel header files
2. OpenSSL
3. DKMS
#### On Ubuntu based systems
```console
foo@bar~:$ sudo apt-get install build-essential linux-headers-generic
foo@bar~:$ sudo apt-get install dkms
foo@bar~:$ sudo apt-get install openssl libssl-dev
```
#### On OpenSUSE
Open YaST / Software / Software Management.  
Select the **View** button on the top left and pick **Patterns**. 
Install the following packages under **Basic Development**:  
- Linux Kernel Development
-  C/C++ Development	 

Utilize the **Search** Button to install the following packages:
- libopenssl-devel
- dkms

#### On CentOS/RHEL/Fedora/RockyLinux baed systems
```console
foo@bar~:$ dnf install -y kernel-devel kernel-headers gcc make
foo@bar~:$ dnf install -y openssl-devel
foo@bar~:$ dnf install -y openssl
foo@bar~:$ dnf install -y dkms
```

Some systems include the **dkms** package in the **"Extra Packages for Enterprise Linux"(EPEL)**;  
therefore, if dkms failed to install, consider installing **EPEL** before installing dkms.  
``` console
foo@bar~:$ dnf install -y epel-release
foo@bar~:$ dnf install -y dkms
```
Early RedHat systems (before CentOS 7/RHEL 7/Fedora 21) might require you to use **"yum"** instead of **"dnf"**.
```console
foo@bar~:$ yum install -y epel-release
foo@bar~:$ yum install -y kernel-devel kernel-headers gcc make
foo@bar~:$ yum install -y openssl-devel
foo@bar~:$ yum install -y openssl
foo@bar~:$ yum install -y dkms
```

### Configure the installation
Before building the Driver, edit the **Config.mk** file to enable/disable/adjust:
1. OpenSSL support
2. DKMS integration
3. Systemd integration
4. SSL Certification length & duration

### Build source code
Use command **make** to build the source code
```console
foo@bar~: $ make
```
### Install Driver to system
Use command **make install** to install the driver with|without DKMS according to the **Config.mk** file.
```console
foo@bar~: $ make install
```

## Starting VCOM
To startup the VCOM service, follow these steps:
1. Setup and start the VCOM [check here](doc/setup_vcom.md)
2. If TLS is needed, create & upload TLS fils for each Device server [check here](doc/setup_tls_for_eki.md)

## DKMS auto-sign with MOK on Secure Boot UEFI systems
On Secure Boot UEFI enabled systems, it is very important to setup the DKMS to auto-sign the kenel modules.  
Otherwise, one may fail to load any custom moduels, including our vcom driver.  
[check here](doc/secure_boot.md) for a guide on how to setup your DKMS.

## Appendex
1. Create new "TLS files" or "key-pairs" needed for VCOM driver; click [here](doc/create_tls_files_driver.md)

