# Advantech Linux VCOM driver
## Introduction
The Linux VCOM driver for Advantech Device Servers
Support extended featurs:
- TLS encription
- DKMS integration
- systemd integration
- Advanced system monitoring
  - VCOM connection status monitoring: /tmp/advmon/advtty*
  - > $ cat /tmp/advmon/advtty0
  - TLS connection status logging: /tmp/advsslmsg/*
  - > $ cat /tmp/advsslmsg/0
- syslog integration
  -  logging **/tmp/advcom/** events to syslog
  - > $ vcinot -p /tmp/advmon -l &  

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
> $ sudo apt-get install build-essential linux-headers-generic
> 
> $ sudo apt-get install dkms
> 
> $ sudo apt-get install openssl libssl-dev
#### On OpenSUSE
Open YaST / Software / Software Management.
Select the View Button on the top left and pick Patterns. 
Now, you will see several Patterns listed and you want to select:  
**Basic Development**
- Linux Kernel Development
-  C/C++ Development	 

Utilize the **Search** Button to install the following packages:
- libopenssl-devel
- dkms

#### On CentOS/RHEL/Fedora/RockyLinux baed systems
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
### Configure the installation
Before building the Driver, edit the **Config.mk** file to enable/disable/adjust:
1. OpenSSL support
2. DKMS integration
3. Systemd integration
4. SSL Certification length & duration

For more details, checkout the readme.txt
### Build Driver source code
Use command **make** to build the source code
> $ make

### Install Driver to system
Use command **make install** to install the driver with|without DKMS according to the **Config.mk** file.
> $ make install

## Setup and start the VCOM Service
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
| advps | list the connection status of the current system |  $ sudo advps |

### Add connections to the VCOM connection map
Use **advadd** to add a VCOM connection.
the following examples shows how to add a connection with/without TLS.
> **Add a VCOM connection connecting "/dev/ttyADV0" with a EKI-1524-CE's 1st serial port**
> 
> $ sudo advadd -t ssl:c524 -a 172.17.8.100 -p 1 -m 0
>
>**Connect /dev/ttyADV0 with a "VCOM over TLS" connection**
>
> $ sudo advadd -t ssl:c524 -a 172.17.8.100 -p 1 -m 0

### Remove unwanted connections from the VCOM connectoin map
Use **advrm** to remove connectons from the map.
> Remove the connection assigned to **/dev/ttyADV0**
> 
> $ sudo advrm -m 0
>
>Remove connections, which are connected to 172.17.8.100
>
> $ sudo advrm -a 172.17.8.100
> 
>Remove connections, which are connected to an EKI-1522-CE
>
> $ sudo advrm -t c522

#### Device ID table
The device ID is specified by **-t** when using **advadd** or **advrm**.  
If one uses an EKI Device server with **ignore device ID** enabled or with the VCOM service switched to version **2.0**, 
this value dosn't matter anymore.  Just give it any ID, for example "1524", for backword compatability.
|Device | Device ID|
|:-----:|:--------:|
| EKI-1521-AE		| 1521		|
| EKI-1522-AE		| 1522		|
| EKI-1524-AE		| 1524		|
| EKI-1528-AE		| 1528		|
| EKI-1526-AE		| 1526		|
| EKI-1521-BE		| B521		|
| EKI-1522-BE		| B522		|
| EKI-1524-BE		| B524		|
| EKI-1528-BE		| B528		|
| EKI-1526-BE		| B526		|
| EKI-1521-CE		| C521		|
| EKI-1522-CE		| C522		|
| EKI-1524-CE		| C524		|
| EKI-1528-CE		| D528		|
| EKI-1528DR		| C528		|
| EKI-1526-CE		| D526		|
| EKI-1321		| 1321		|
| EKI-1322		| 1322		|
| EKI-1361		| 1361		|
| EKI-1362		| 1362		|
| EKI-1361-BE		| B361		|
| EKI-1362-BE		| B362		|
| ADAM-4570-BE		| 4570		|
| ADAM-4570-CE		| D570		|
| ADAM-4571-BE		| 4571		|
| ADAM-4571-CE		| D571		|
| ADAM-4570L-CE		| B570		|
| ADAM-4570L-DE		| E570		|
| ADAM-4571L-CE		| B571		|
| ADAM-4571L-DE		| E571		|

### startup/updating the service with the current map
Use **advman** to starup or update the VCOM service everytime the VCOM map is modified.
> $ sudo advman -o start

