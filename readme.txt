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
	
	
3. install and uninstall the driver, start and stop the daemon

  3.1 Configure the VCOM mapping
	modify the "config/advttyd.conf" file to match the VCOM mapping that you desire.

    3.1.1 Configuration format
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
	| ADAM-4570-BE		| 4570		|
	| ADAM-4570-CE		| D570		|
	| ADAM-4571-BE		| 4571		|
	| ADAM-4571-CE		| D571		|
	| ADAM-4570L-CE		| B570		|
	| ADAM-4570L-DE		| E570		|
	| ADAM-4571L-CE		| B571		|
	| ADAM-4571L-DE		| E571		|
	+-----------------------+---------------+

    3.1.3 Ignore Device-Type
	Some devices can ignore the "Device-Type" parameter.
	  Enable the "VCOM Ignore Device ID" in the "System" column of the Web Configure GUI, and reboot the device;
	afterwards the device will accept a connection discarding a "Device-Type" mismatch during the "Open Port" handshake.
	
  3.2 install
	# make install        # install driver at /usr/local/advtty and application at /sbin

  3.3 unisntall
	# make uninstall      # uninstall the driver and application
	
	If vcom service is running, it must be stopped and removed before uninstalling.
	(for details on how to remove the service, checkout section 3.5)

  3.4 start the daemon
	# advman -o insert      # insert the driver
	# advman -o start       # start the application

  3.5 stop the daemon
	# advman -o stop         # stop the daemon
	# advman -o remove       # remove the driver from kernel



4. System managment.

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


5. Daemon configuration

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

  5.2 Modifying the daemon configuartions

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
