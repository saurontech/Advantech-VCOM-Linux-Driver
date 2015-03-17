============================================================================
	       	Advantech Linux Virtual TTY driver Installation Guide
		                     for Linux
                  Copyright (C) 2010, Advantech Co., Ltd.
============================================================================

This README file describes the HOW-TO of driver installation.

1. Introduction

   The Advantch Linux Virtual TTY driver makes you operate serial ports in remove
   machine conveniently.It consists of two layers: the driver and the Application. 
   
   Test environment
   +--------------------------+-------------------------+
   |  Distribution            | kernel version          | 
   +--------------------------+-------------------------+
   |	Red Hat 9             |  2.4.20-8               |  
   |	Redhat Enterprise 5.4 |  2.6.18-164.el5         |                 
   |	Fedora Core 13(64bit) |  2.6.33.3-85.fc13       |       
   |	Fedora Core 14        |  2.6.35.6-45.fc14       |   
   |	Fedora Core 16        |  3.1.0-7.fc16       	|           
   |	SUSE 10.1             |  2.6.16.13-4-default    |       
   |	SUSE 11.2             |  2.6.31.5-0.1-desktop   |     
   |	Mandriva 2010         |  2.6.31.5-desktop-1mnb  |
   |	Debian 5.0.4          |  2.6.26-2-686           |    
   |	Ubuntu 8.04           |  2.6.24-19-generic      |
   |	Ubuntu 11.10          |  3.0.0-12-generic       |
   +--------------------------+-------------------------+
	
2. Installation

2.1 Login as 'root' before executing the following instructions.

2.2 Uncompress driver

   2.2.1 # tar xvf XXX_Pseudo_TTY_Driver_V1.0.tar.bz2
   2.2.2 # cd advtty
   note: 
      1.the XXX is your host linux system. For example,if your host linux system is ubuntu10.04,
	then you need use the file Ubuntu10.04_Pseudo_TTY_Driver_V1.0.tar.bz2
      2.For Debian 3.1, Redhat 9.0 and Redhat 4.3, you should use "tar xvzf XXX_Pseudo_TTY_Driver_V1.0.tar.gz"
	to extract it.
3. install and uninstall the driver, start and stop the app
                              
   3.1 install
	# make install     # install driver at /usr/local/advtty and application at /sbin

   3.2 unisntall
	# make uninstall   # uninstall the driver and application

   3.3 start the app
	# advman -o insert # insert the driver.if you are starting the app at the first time, 
			   # you should insert the driver firstly. you can add this command to
			   # file /etc/profile
	# advman -o start  # start the application

   3.4 stop the app
	# advman -o stop   # stop the application
	# advman -o remove # remove the driver from kernel


4. How to test the driver

   Please check with user manual for details.

