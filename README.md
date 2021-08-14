# Advantech Linux VCOM driver
## Introduction
The Linux VCOM driver for Advantech Device Servers
Support extended featurs:
- TLS encription
- DKMS integration
- systemd integration
- Advanced system monitoring
  - VCOM connection status monitoring: /tmp/advmon/advtty*
  - TLS connection status logging: /tmp/advsslmsg/*

### Quality
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/66cde2a55b884e1e8a98adac7556e503)](https://www.codacy.com/gh/saurontech/Advantech-VCOM-Linux-Driver/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=saurontech/Advantech-VCOM-Linux-Driver&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/saurontech/advantech-vcom-linux-driver/badge/main)](https://www.codefactor.io/repository/github/saurontech/advantech-vcom-linux-driver/overview/main)
## Installation Guide
### Dependancy
This driver depends on:
1. kernel header files
2. OpenSSL
3. DKMS
#### Ubuntu
> $ sudo apt-get install build-essential linux-headers-generic
> 
> $ sudo apt-get install dkms
> 
> $ sudo apt-get install openssl libssl-dev
#### OpenSUSE
Open YaST / Software / Software Management.
Select the View Button on the top left and pick Patterns. 
Now, you will see several Patterns listed and you want to select:
**Basic Development**
- Linux Kernel Development
-  C/C++ Development	 

Utilize the **Search** Button to install the following packages:
- libopenssl-devel
- dkms

#### CentOS/RHEL/Fedora/Rocky Linux
> $ dnf install kernel-devel kernel-headers gcc make
> 
> $ dnf install openssl-devel
> 
> $ dnf install openssl
> 
> $ dnf install dkms

Early RedHat systems (before CentOS 7/RHEL 7/Fedora 21) might require you to use "yum" instead of "dnf".

On CentOS 7, DKMS is included in the "EPEL"(Extended Packages for Enterpirse Linux), one would need to enable EPEL with the following command:

> $ yum install -y epel-release

> $ yum install kernel-devel kernel-headers gcc make
> 
>	$ yum install openssl-devel
>	
>	$ yum install openssl
>	
>	$ yum install dkms
### Configure
Before building the Driver, edit the **Config.mk** file to enable/disable/adjust:
1. OpenSSL support
2. DKMS integration
3. Systemd integration
4. SSL Certification length & duration

For more details, checkout the readme.txt
### Build Driver
Use command **make** to build the source code
> $ make

### Install Driver
Use command **make install** to install the driver with|without DKMS according to the **Config.mk** file.
> $ make install

## Startup the VCOM Service
To starup the VCOM service, one would need to:
1. configure all the VCOM connections
2. startup/update the VCOM service

We've included the following tools to asist with the process:
| command | discription | example |
| ------- |:----------- |:-----------|
| advadd | add a VCOM connection | $ sudo advadd -t c524 -a 172.17.8.100 -p 1 -m 0 |
| advrm | remove a VCOM connection | $ sudo advrm -m 0 |
| advman | startup/update the VCOM service | $ sudo advman -o start |
| advls | list the VCOM connections | $ sudo advls |
| advps | list the connection status of the current system \ sdfasd |  $ sudo advps |

### add/remove a connection to the VCOM map
#### Device ID table
### startup/updating the service
