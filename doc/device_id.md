# Device ID of the Advantech VCOM Portocol
## Introduction
Device ID is a 16 bit Hex Magic number, used in the Advantech VCOM protocol to check for device type.
However, this number is no longer relavent if you chose to use "VCOM protocol stack 2.0" on you **Device Server**.

**Not to be confused with the VCOM driver versoin!**  
The **"VCOM protocol stack version"** is an option on the **EKI Device server**.  
It indicates the EKI device to run a new VCOM protocol stack.  
Which is fully compatiable with the old VCOM protocol.

For the ones using "VCOM protocol stack 2.0" on the **Device server**, one can give any 16 but Hex number, 
from **FFFF** to **0000**.  
If **"ssl:"** is added before the hex, TLS is used for the given connection.  
```console
foo@bar:~$ sudo advadd -m 0 -a 172.17.8.224 -t ssl:ffff -p 1  
foo@bar:~$ sudo advadd -m 1 -a 172.17.8.222 -t c524 -p 1   
```
For the ones that stick with "VCOM protocol stack 1.0", and with to ignore this value.
 enable **"Ignore device ID"** on the Advantech Device Server.

Back to [Setup VCOM](setup_vcom.md)  
Back to [README.md](../README.md)
## Device ID 
| Device Name | ID (Hex) |
|-------------|----------|
| EKI-1521-AE	| 1521	|
| EKI-1522-AE	| 1522	|
| EKI-1524-AE	| 1524	|
| EKI-1528-AE	| 1528	|
| EKI-1526-AE	| 1526	|
| EKI-1521-BE	| B521	|
| EKI-1522-BE	| B522	|
| EKI-1524-BE	| B524	|
| EKI-1528-BE	| B528	|
| EKI-1526-BE	| B526	|
| EKI-1521-CE	| C521	|
| EKI-1522-CE	| C522	|
| EKI-1524-CE	| C524	|
| EKI-1528-CE	| D528	|
| EKI-1528DR	| C528	|
| EKI-1526-CE	| D526	|
| EKI-1511-A | 1501 |
| EKI-1511X-B | 1551 |
| EKI-1321	| 1321	|
| EKI-1322	| 1322	|
| EKI-1361	| 1361	|
| EKI-1362	| 1362	|
| EKI-1361-BE	| B361	|
| EKI-1362-BE	| B362	|
| ADAM-4570-BE	| 4570	|
| ADAM-4570-CE	| D570	|
| ADAM-4571-BE	| 4571	|
| ADAM-4571-CE	| D571	|
| ADAM-4570L-CE	| B570	|
| ADAM-4570L-DE	| E570	|
| ADAM-4571L-CE	| B571	|
| ADAM-4571L-DE	| E571	|
