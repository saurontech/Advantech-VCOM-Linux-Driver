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

the following examples shows how to add a connection with/without TLS.
> **Add a VCOM connection connecting "/dev/ttyADV0" with a EKI-1524-CE's 1st serial port**
> 
> $ sudo advadd -t c524 -a 172.17.8.100 -p 1 -m 0
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

### List the connection map
Use **advls** to list the connection map, please notice this is the offline map.
To make this map **"active and current"** one would have to startup the VCOM service via **advman**.
> $ sudo advls

## 2. Startup/update the VCOM service

### Startup/updating the service with the current map
Use **advman** to startup or update the VCOM service everytime the VCOM map is modified.
> $ sudo advman -o start

### Checking the VCOM connection status
Use **advps** to check the current **active and current** VCOM connection status.
> $ sudo advps
>  
> ttyADV0 PID:622218 TCP Dev:c524 Port1 IP:172.17.8.222  
> ttyADV1 PID:622221 TCP Dev:c524 Port2 IP:172.17.8.222  
> ttyADV2 PID:622224 TLS Dev:c524 Port1 IP:172.17.8.224  
> ttyADV3 PID:622227 TLS Dev:c524 Port2 IP:172.17.8.224  
