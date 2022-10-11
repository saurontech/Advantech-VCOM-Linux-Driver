# VCOM driver with Secure Boot UEFI, MOK and DKMS
Modern PCs often use Secure boot to authenticate the boot proccess.
Linux kernels with CONFIG_MODULE_SIG enabled, will fail to load drivers that aren't correctly signed

The following parties are involved in the proccess of loading the VCOM driver:
1.  UEFI: the UEFI hosts a list of public keys, each kernel/module loaded will be checked against these keys.
2.  Machine Owner Key(MOK): this is a Public/Private key pair stored in DER formate.
3.  DKMS: by default, DKMS is used to auto-build our driver.

If one fails to load the driver with **advman -o insert**, this might indicate that your **DKMS** is not properly configured to work with **Secure Boot**.
One might chose to disable the secure boot, or configure your DKMS to work with Secure Boot.


## Confiugre DKMS to auto-sign drivers
The DKMS can be configured to auto-sign drivers.  
However, this proccess might differ according to the version of your DKMS.  
On **Newer** DKMS systems like the one used on RHEL9(DKMS-3.0.4), the drivers are signed with the "/var/lib/dkms/mok.key", and "/var/lib/dkms/mok.pub".

1. Check if secure boot is enabled
```console
foo@bar~:$ sudo mokutil --sb-state
```
2. List the enrolled public keys
```console
foo@bar~:$ sudo mokutil --list-enrolled
```
3. Dump the MOK info and check if it is on the enrolled list.
```console
foo@bar~:$ openssl x509 -in /var/lib/dkms/mok.pub -inform der -noout -text
```
If the MOK is not enrolled, one can enroll the MOK with the following command:
``` console
foo@bar~:$ sudo mokutil --import /var/lib/dkms/mok.pub
input password:
input password again:
```
Memorize the password, and reboot the system.  
During reboot, the UEFI will guide you through the proccess of enrolling the MOK.  
The password will be required to finish the process.  

4. Check the VCOM driver signature and see if it matches your MOK
```console
foo@bar~:$ sudo modinfo advvcom
```
If the driver is not signed, or not signed by your MOK, edit "/etc/dkms/framework.conf" and modify options "mok_signing_key" and "mok_certificate" according to the path of your MOK. 
Afterward, one must rebuild the driver with the DKMS.  
The easiest way is the operate the following instructions in our source code folder.
```console
foo@bar~:$ sudo advman -o remove
foo@bar~:$ sudo make uninstall_dkms
foo@bar~:$ sudo make install_dkms
```

## On Ubuntu 22.04 with earlier DKMS systems
The proccess is roughly the same, However, there are two major differences:
1. Ubuntu 22.04 places the MOK in a different path:
    1. private key: /var/lib/shim-signed/mok/MOK.priv
    2. public key: /var/lib/shim-isgned/mok/MOK.der

Enroll the MOK with:
```console
foo@bar~:$ sudo mokutil --import /var/lib/shim-signed/mok/MOK.der
```
2. The DKMS framework(/etc/dkms/framework.conf) options is different:
Uncomment the: 
```console
sing_tool="/etc/dkms/sign_helper.sh" 
```
The default MOK used in "sign_helper.sh" is located in "/root/", so one can edit "/etc/dkms/sign_helper.sh" to change it.  
```console
#!/bin/sh
/lib/modules/"$1"/build/scripts/sign-file sha512 /var/lib/shim-signed/mok/MOK.priv /var/lib/shim-isgned/mok/MOK.der "$2"
```
Rebuild the vcom driver with DKMS; in our source code folder:
```console
foo@bar~:$ sudo advman -o remove
foo@bar~:$ sudo make unisntall_dkms
foo@bar~:$ sudo make install_dkms
```

## Building ones own MOK
If for some reason, one needs to build its own MOK
the following command will build a MOK private/public key pair:
```console
foo@bar~:$ openssl req -new -x509 -newkey rsa:2048 -keyout ./MOK.priv -outform DER -out ./MOK.der -nodes -days 36500 -subj "/CN=ADVVCOM Driver Signing MOK"
```
