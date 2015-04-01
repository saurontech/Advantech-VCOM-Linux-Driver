============================================================================
	       	Advantech Linux Virtual TTY driver Installation Guide
		                     for Linux
                  Copyright (C) 2010, Advantech Co., Ltd.
============================================================================

This README file describes the HOW-TO of driver installation.

1. Introduction

   The Advantch Linux Virtual TTY driver makes you operate serial ports in remove
   machine conveniently.It consists of two layers: the driver and the Application. 
   
2. Installation

2.1 Compile the driver

   2.1.1 Dependancy
	You should install the kernel header files to compile this driver.

   2.1.1.1 Ubuntu
	# sudo apt-get install build-essential linux-headers-generic

   2.1.1.2 OpenSUSE
	Open YaST / Software / Software Management.
	Select the View Button on the top left and pick Patterns. 
	Now, you will see several Patterns listed and you want to select:
	Development 
	[X] Base Development
	[X] Linux Kernel Development
	[X] C/C++ Development

   2.1.1.3 CentOS/RHEL/Fedora
	# yum install kernel-devel kernel-headers gcc make
	
   2.1.2 Compile the source code
	This driver comes with a Makefile, therefore you can compile the driver with a single command.
	# make
	
	
3. install and uninstall the driver, start and stop the app

   3.1 Configure the VCOM mapping
	modify the "config/advttyd.conf" file to match the VCOM mapping that you desire.

   3.1.1 Configuration format
	If configure format is defined as:
		[Minor] [Device-Type] [Device-IP] [Port-Idx]

	For example, if you wish to build up a map like:
		/dev/ttyADV0	-->	EKI-1524-BE's 1st serial port(IP 172.17.8.12)
		/dev/ttyADV1	-->	EKI-1322's 2nd serial port(IP 10.0.0.100)
		/dev/ttyADV2	-->	EKI-1526-BE's 8th serial port(IP 192.168.1.12)
	
	Your advttyd.conf should look like:
		0	B524	172.17.8.12	1
		1	1322	10.0.0.100	2
		2	B526	192.168.1.12	8

   3.1.2 Device-Type Table
	 _______________________________________
	| Device Name		| Device-Type	|			
        |=======================+===============|
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
	| EKI-1321		| 1321		|
	| EKI-1322		| 1322		|
	| EKI-1361		| 1361		|
	| EKI-1362		| 1362		|
	| ADAM-4570-BE		| 4570		|
	| ADAM-4570-CE		| D570		|
	| ADAM-4571-BE		| 4571		|
	| ADAM-4571-CE		| D571		|
	| ADAM-4570L-CE		| B570		|
	| ADAM-4570L-DE		| E570		|
	| ADAM-4571L-CE		| B571		|
	| ADAM-4571L-DE		| E571		|
	+-----------------------+---------------+
	
   3.2 install
	# make install     # install driver at /usr/local/advtty and application at /sbin

   3.3 unisntall
	# make uninstall   # uninstall the driver and application

   3.4 start the app
	# advman -o insert # insert the driver.if you are starting the app at the first time, 
			   # you should insert the driver firstly. you can add this command to
			   # file /etc/profile
	# advman -o start  # start the application

   3.5 stop the app
	# advman -o stop   # stop the application
	# advman -o remove # remove the driver from kernel


