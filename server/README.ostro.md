#Installing on Ostro OS#

##Install Ostro##

The following guide assumes that the device has been flashed with
[Ostro OS](https://ostroproject.org).

The below has been verified to work with image **2016-04-13_04-08-19-build-222**

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

##Install the service##

When the service has been installed, it will automatically launch at boot time.

Make sure you are in the ```webbluetooth-edison-demo/server``` directory.

```
cp webbluetooth-edison-demo.service /lib/systemd/system

systemctl enable webbluetooth-edison-demo.service
systemctl start webbluetooth-edison-demo.service
```