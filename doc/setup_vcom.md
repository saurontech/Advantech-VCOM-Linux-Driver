# Setup and start the VCOM Service
To starup the VCOM service, one would need to:
1. Add/Remove VCOM connections
2. startup/update the VCOM service

We've included the following tools to asist with the process:
| command | discription | example |
| ------- |:----------- |:-----------|
| advadd | add a VCOM connection | $ sudo advadd -t c524 -a 172.17.8.100 -p 1 -m 0 |
| advrm | remove a VCOM connection | $ sudo advrm -m 0 |
| advman | startup/update the VCOM service | $ sudo advman -o start |
| advls | list the VCOM connections | $ sudo advls |
| advps | list the connection status of the current system |  $ sudo advps |

The command **advadd**,**advrm**, **advls** handles the **"offline"** VCOM connection map.  
To make the VCOM connection **"active and current"**, one would need to use **advman** to start or update the service.

back to [README.md](../README.md)

## 1. Add/Remove VCOM connections

### Add connections to the VCOM connection map
Use **advadd** to add a VCOM connection.
| option | discription | info|
| :-----: |:----------- |:---|
| -a | IP Address| a IPv4, IPv6 or URL |
| -t | DeviceID |checkout [devic_id.md](device_id.md) |
| -m | MinorID of the driver node| starts from 0 |
| -p | Port Number on the Device | starts from 1 |
| -r | Redundent IP Address | |
| -s | Synchronize added node to the active service| |

the following examples shows how to add a connection with/without TLS.  
* Add a VCOM connection connecting **/dev/ttyADV0** with a EKI-1524-CE's 1st serial port:
```console 
foo@bar~:$ sudo advadd -t c524 -a 172.17.8.100 -p 1 -m 0
```
* Connect **/dev/ttyADV0** with a "VCOM over TLS" connection
```console
foo@bar~:$ sudo advadd -t ssl:c524 -a 172.17.8.100 -p 1 -m 0
```

### Remove unwanted connections from the VCOM connectoin map
Use **advrm** to remove connectons from the map.
* Remove the connection assigned to **/dev/ttyADV0**
```console
foo@bar~$ sudo advrm -m 0
```
* Remove connections, which are connected to 172.17.8.100
```console
foo@bar~:$ sudo advrm -a 172.17.8.100
```
* Remove connections, which are connected to an EKI-1522-CE
```console
foo@bar~$ sudo advrm -t c522
```
### List the connection map
Use **advls** to list the connection map, please notice this is the offline map.  
To make this map **"active and current"** one would have to startup the VCOM service via **advman**.
```console
foo@bar~:$ sudo advls
0   c524   172.17.8.224   1
1   c524   172.17.8.224   2
```
## 2. Startup/update the VCOM service

### Startup/updating the service with the current map
Use **advman** to startup or update the VCOM service everytime the VCOM map is modified.
```console
foo@bar~:$ sudo advman -o start
```
**advman** has only one option **"-o"**, however, it can have the following input values:  
| value | Discription |
|:-----:|:------------|
| start | start the system(Driver + Service) according to the offline connection map |
| stop | Stop the system(Driver + Service) |
| sync | sync/update the system according to the offline connection map |
| insert | Insert the driver, don't start the service |
| remove | Remove the driver |
| restart | Stop and then restart the service |

### Checking the VCOM connection status
Use **advps** to check the **active and current** VCOM connection status.
```console
foo@bar~:$ sudo advps 
ttyADV0 PID:622218 TCP Dev:c524 Port1 IP:172.17.8.222  
ttyADV1 PID:622221 TCP Dev:c524 Port2 IP:172.17.8.222|172.17.8.223  
ttyADV2 PID:622224 TLS Dev:c524 Port1 IP:172.17.8.224  
ttyADV3 PID:622227 TLS Dev:c524 Port2 IP:172.17.8.224  
```
