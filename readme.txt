============================================================================
	       	Advantech Linux Virtual TTY driver readme File
		                     for Linux
                  Copyright (C) 2018, Advantech Co., Ltd.
============================================================================

This README file describes the HOW-TO of driver installation and VCOM service managment.

1. Introduction

   The Advantch Linux Virtual TTY driver allows you to utilize the VCOM feature of ADVANTECH
   Device Servers. It consists of two parts: the Driver and the Daemon.
   The "driver" provides a tty I/O interface
   The "daemon" supports the connection between the driver and the Advantech Deivce.

   applicatoin <--tty I/O-->"driver"<-->"daemon"<--Advantech VCOM Protocol-->"device"
   
2. Installation

  2.1 Compile the driver

    2.1.1 Dependancy
	You should install the kernel header files to compile this driver.

	This driver is built with the following options, turned on as a default:
	  1. OpenSSL
	  2. DKMS

	To disable these options, edit the "Config.mk" according the instructions of section 6.1 and 7.1

      2.1.1.1 Ubuntu
	# sudo apt-get install build-essential linux-headers-generic
	# sudo apt-get install dkms
	# sudo apt-get install openssl libssl-dev

      2.1.1.2 OpenSUSE
	Open YaST / Software / Software Management.
	Select the View Button on the top left and pick Patterns. 
	Now, you will see several Patterns listed and you want to select:
	Development 
	[X] Base Development
	[X] Linux Kernel Development
	[X] C/C++ Development
	 
	Utilize the Search Button to install the following packages:
	[X] libopenssl-devel
	[X] dkms

      2.1.1.3 CentOS/RHEL/Fedora
	# dnf install kernel-devel kernel-headers gcc make
	# dnf install openssl-devel
	# dnf install openssl
	# dnf install dkms

	* Early RedHat systems (before CentOS 7/RHEL 7/Fedora 21) might require you to use "yum" instead of "dnf".

	* On CentOS 7, DKMS is included in the "EPEL"(Extended Packages for Enterpirse Linux),
	* One would need to enable EPEL with the following command:

	# yum install -y epel-release

	# yum install kernel-devel kernel-headers gcc make
	# yum install openssl-devel
	# yum install openssl
	# yum install dkms
	
	
    2.1.2 Compile the source code
	
    2.1.2.1 Customizeing build options
	Modify "Config.mk" to build the VCOM service with/without certain options.
	For example, one might wish to build a minimal VCOM service without TLS support.
	Reference section 6 & 7 for more detail.
	
    2.1.2.2 Building the source code
	This driver comes with a Makefile, therefore you can compile the driver with a single command.
	# make

    2.2 Configure the initial VCOM mapping
	modify the "config/advttyd.conf" file to match the VCOM mapping that you desire.
	After installation, this file will be copied to the installation directory(default: /usr/local/advtty)
	  Additional helper scripts will be installed alongside the VCOM driver to aide with future modificaitons of the VCOM mapping.
	For more detail, please reference section "5.2 Modifying the daemon configuartions".

    2.2.1 Configuration format
	The configure format is defined as:
		[Minor] [Device-Type] [Device-IP] [Port-Idx]

	For example, if you wish to build up a map like:
		/dev/ttyADV0	-->	EKI-1524-BE's 1st serial port(IP 172.17.8.12)
		/dev/ttyADV1	-->	EKI-1322's 2nd serial port(IP 10.0.0.100)
		/dev/ttyADV2	-->	EKI-1526-BE's 8th serial port(IP 192.168.1.12)
	
	Your advttyd.conf should look like:
		0	B524	172.17.8.12	1
		1	1322	10.0.0.100	2
		2	B526	192.168.1.12	8

    2.2.2 Device-Type Table
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
	| EKI-1521-CE		| C521		|
	| EKI-1522-CE		| C522		|
	| EKI-1524-CE		| C524		|
	| EKI-1528-CE		| D528		|
	| EKI-1528DR		| C528		|
	| EKI-1526-CE		| D526		|
	| EKI-1511-A		| 1501		|
	| EKI-1511X-B		| 1551		|
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
	+-----------------------+---------------+

    2.2.3 Ignore Device-Type
	Some devices can ignore the "Device-Type" parameter.
	  Enable the "VCOM Ignore Device ID" in the "System" column of the Web Configure GUI, and reboot the device;
	afterwards the device will accept a connection discarding a "Device-Type" mismatch during the "Open Port" handshake.
	
    2.2.4 install
	# make install        # install driver at /usr/local/advtty and application at /sbin

    2.3.5 unisntall
	# make uninstall      # uninstall the driver and application
	
	If vcom service is running, it must be stopped and removed before uninstalling.
	(for more detail on how to remove the service, checkout section 3.2)
	
	
3. Insert and remove the driver, start and stop the daemon

  After installation, the driver and daemon are installed; 
  however, before using the VCOM service, one must insert the driver and start the daemons.
  A helper script (advman) is installed alongside the driver to help start/stop/manage the VCOM service.
  Refer to section "5. Daemon configuration/managment" for more detail.

  3.1 start the daemon
	# advman -o insert      # insert the driver
	# advman -o start       # start the application

  3.2 stop the daemon
	# advman -o stop         # stop the daemon
	# advman -o remove       # remove the driver from kernel
  
  3.3 auto-starting the VCOM service at boot time
	Linux is a highly customizable environment.
	Starting the VCOM service during boot time depends on the choise of your initial daemon.
	However, we do provide a sample service file for systemd.

  3.3.1 Systemd
	A systemd service file is provided as a reference, if one choses to use systemd as the initial daemon.
	It is located at "misc/systemd/advvcom.service"

  3.3.1.1 Install advvcom.service
	# make install -C ./misc/systemd/

  3.3.1.2 Enabling advvcom.service
	Enabling the service on systemd will allow VCOM to auto-start during bootup.
	# systemctl enable advvcom.service

  3.3.1.3 Starting advvcom.service
	This will start the VCOM service via systemd right away.
	# systemctl start advvcom.service


4. System managment.
  Several tools were designed to help system administrators check on the status and historys of each VCOM connection.

  4.1 Checking the on-line/off-line status of the daemons.
	# advps            # this command shows all the tty interfaces that are currently supported by a on-line/running daemon
	ttyADV0		PID:19145
	ttyADV1		PID:19147
	ttyADV5		PID:19149

  4.2 Checking the status of a VCOM daemon.
	You can check the state machine of a individual VCOM connection by accessing the "monitor file" located in "/tmp/advmon/"
	# cat /tmp/advmon/advtty0       # check status of ttyADV0

	Pid 19145 | State [Net Down] > (Net Up)vc_recv_desp,61 > (Net Up)vc_recv_desp,61 > (Net Up)vc_recv_desp,61
	      ^a              ^b	 ^c.0                      ^c.1                      ^c.2
	a. Daemon PID. 
	b. Connection state.
	c. Exception history.

    4.2.1 Connection state.
	The current state of the VCOM daemon.

      4.2.1.1 [Net Down]
	The daemon is not connected to the Device server.
	This represents two possible situations:
	1. The TTY port is currently not opened by a user applicaton.
	2. The connection is currently down, due to configuration or connection errors.

      4.2.1.2 [Net Up]
	The daemon is connected to the Device server.

      4.2.1.3 [Sync]
	The daemon is synchronizing with the Device server.

      4.2.1.4 [Idle]
	The daemon hasn't recieved TCP packets from the device server for a while.

      4.2.1.5 [Pause]
	Data pause is triggered by TTY throttle.

	
    4.2.2 Exception history

      4.2.2.1 Message format
		("daemon state during exception occurrace") "name of the functon that encountered the exception","line number of the source code"

		example:
		  (Net Up)vs_recv_desp,61

      4.2.2.2 Order of the history
		Most Recent > ... > Earliest

  4.3 Logging the exception events
	vcinot is a tool designed to help system admins manage the VCOM service by pushing the exception events to syslog.
	# vcinot -p /tmp/advmon -l &                  # push all the exceptions to syslog
	# vcinot -p /tmp/advmon/advtty0 -l &          # push exceptions of "ttyADV0" only.
	# vcinot -p /tmp/advmon/advtty0 &             # only push the fist exception of "ttyADV0".

  4.4 Checking TLS/SSL logs
	The log messages assosiated with the TLS/SSL communications are stored in the "tmp/advsslmsg/" directory.
	
	# cat /tmp/advsslmsg/0 				# checking the TLS/SSL log messages of the /dev/ttyADV0
	2020-11-18|15:11:14:X509 error(67):CA certificate key too weak
	2020-11-18|15:11:14:SSL_connect failed(1):certificate verify failed
	2020-11-18|15:11:11:X509 error(67):CA certificate key too weak
	2020-11-18|15:11:11:SSL_connect failed(1):certificate verify failed


5. Daemon configuration/managment

  5.1 Managing the daemons
	"advman" is used to manage the driver and the daemons.

    5.1.1 Insert the driver
	# advman -o insert

    5.1.2 Remove the driver
	# advman -o remove

    5.1.3 Start all daemons
	This is used to start the service, or restart the daemons after modification of daemon configurations.
	# advman -o start
	# advman -o restart

    5.1.4 Stop all daemons
	# advman -o stop

  5.2 Modifying the daemon configuartions(VCOM mapping)

	Several tools are here to aide with modifying the VCOM mapping.
	Please notice that, after modifying the VCOM mapping, one must update the system to make it effective.
	Please refer to section 5.2.4 for more detail.

    5.2.1 Adding a connection
	# advadd -a 10.0.0.1 -t C524 -p 1 -m 0			# Connecting /dev/ttyADV0 to the first serial port of a EKI-1524-CE with the IP address of 10.0.0.1

    5.2.2 Removing a connection
	# advrm -m 0			# Remove the configuration of /dev/ttyADV0
	# advrm -t C524			# Remove all configurations with Device-Type "C524"
	# advrm -p 1			# Remove all configurations associated with the first serial port of a device server
	# advrm -p 1 -t C524		# Remove all configurations associated with the first serial port of a C524 Device-Type

    5.2.3 List the current configuratioin
	# advls
	0   1524    10.0.0.1    1
	1   1524    10.0.0.1    2

	Notice that the result of "advls" might not match the result of "advps".
	"advps" shows the "running" status of the daemons, while "advls" shows the configuaration, which might not yet be executed.
	Utilize "advman -o start" or "advman -o restart" to execute the configuration.
	
      5.2.3.1 message format
	[minor number] [device-type] [IP address] [serial number]
	example:
	   0	1524	10.0.0.1	1

    5.2.4 Update the system with the new configuration.
	After changing the daemon configurations(VCOM mapping), one must update the system to make the configuration effective.
	# advman -o start

6. VCOM over TLS

  6.1 Building the Driver with or without TLS support

	Advantech VCOM driver supports TLS by leveraging the OpenSSL library.
	One might wish to build this driver with or without TLS based on the capacity of the platform environment.
	Therefore, this driver offers the option to include or exclude the TLS library, before building it with the command "make".
	By editing the "TLS" option in the "Config.mk" file to "y" or "n", the drive will be built with or without TLS support.

	* Example 1:
	// building the source code with OpenSSL included
	// in the Config.mk file
	  TLS = y

	* Example 2:
	// building the source code with OpenSSL excluded
	// in the Config.mk file
	  TLS = n

  6.2 System overview

    6.2.1 Files and Binaries 
	If one choses to build the driver with TLS support, multiple files needed to run TLS will be generated at build time including:
	1. keys/rootCA.pem //the default rootCA file created at build time
	2. keys/vcom.pem // the default public/private key for the VCOM driver created at build time
	3. sslproxy/advsslvcom //the TLS service

	After installation, all of the files will be copied to the installation directory (default: /usr/local/advtty).

    6.2.2 Configuration of the default CA file
	The default rootCA.pem is created based on the "keys/rootca.conf" file. 
	One can edit the file to change the default info, which is used to generate the rootCA.
	The langth and validation period of the CA file is defined in the Config.mk file.

    6.2.3 Configuration of the TLS service.
	The configuration is stored in the "sslproxy/config.json" file, if one wishes to use customized files, please edit the file accordingly.
	
	After installation, the file will be copied to the installation directory (default: /usr/local/advtty).

    6.2.4 Configuration of the default certification file
	The default vcom.pem is created based on the "keys/vcom.conf" file. 
	One can edit the file to change the default info, which is used to generate the vcom.pem.
	The langth and validation period of the CA file is defined in the Config.mk file.

  6.3 Building DH files and Private/Public Key pairs for a EKI device server.
	
	* Build a Private/Public Key "EKI123.pem" based on /usr/local/advtty/rootCA.pem
	# adv-eki-tls-create -n EKI123
	
	For more details on "adv-eki-tls-create" command, use the "-h" option to get the full help message.
	# adv-eki-tls-create -h

  6.4 Configuring a ttyADV node to operate over a TLS conneciton.
	Please Notice: Not all EKI devices support TLS!
	Adding the "ssl:" tag before a "device type" option will configure the spacific node operate over TLS.
	#advadd -a 10.0.0.1 -t ssl:1224 -p 1 -m 0
	
7. DKMS(Dynamic Kernel Module Support)

  7.1 Installing the driver with or without DKMS
	By editing the "DKMS" option in the "Config.mk" file to "y" or "n", the drive will be installed("make install") with or without DKMS.


	
