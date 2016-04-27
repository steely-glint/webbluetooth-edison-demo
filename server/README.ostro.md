#Installing on Ostro OS#

##Install Ostro##

The following guide assumes that the device has been flashed with
[Ostro OS](https://ostroproject.org).

Nightly builds (all | dev) can be found
[here](https://download.ostroproject.org/builds/ostro-os/2016-04-25_13-31-11-build-432/images/edison/).
This guide has been verified to work with image **2016-04-13_04-08-19-build-222**

You can check your version with the following command:

```
cat /etc/os-release
```

You can flash your device using the following instructions:

```
bunzip <ostro image.toflash.tar.bz2>
tar xvf <ostro image.toflash.tar>
cd toFlash/
sudo ./flashall.sh
```

##Set up a WIFI connection##

To set up a permanent WIFI connection, use *connman*:

```
# connmanctl
> enable wifi
> agent on
> scan wifi
> services
> connect <service full name>
```

##Get the source code##

```
git clone https://github.com/01org/webbluetooth-edison-demo
```

##Install the dependencies##

For the installation to work, we need to make sure we are using the right ```ar``` binary:

```
mv /usr/bin/ar /usr/bin/ar-old
ln -s /usr/bin/i686-ostro-linux-ar /usr/bin/ar
```

Move to the directory and install the dependencies:

```
cd webbluetooth-edison-demo/server
npm install
```

##Enable Bluetooth##

First we enable Bluetooth via *connman* and set up the ```hci0``` interface
```
connmanctl enable bluetooth
rfkill unblock bluetooth
hciconfig hci0 up
```

Now verify that the ```hci0``` interface is up and working:

```
hcitool scan
```

Now reboot the device.

##Install the service##

When the service has been installed, it will automatically launch at boot time.

Make sure you are in the ```webbluetooth-edison-demo/server``` directory.

```
cp webbluetooth-edison-demo.service /lib/systemd/system

systemctl enable webbluetooth-edison-demo.service
systemctl start webbluetooth-edison-demo.service
```

The service should now start automatically everytime the device is powered on.
Reboot in order to make sure this is the case.