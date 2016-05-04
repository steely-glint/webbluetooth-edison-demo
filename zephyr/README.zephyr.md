#Installing and testing on Zephyr#

##Source the build environment##

Source the zephyr-project/zephyr-env.sh to get up the environment

```
source zephyr-project/zephyr-env.sh
```

##Building and Running Project##

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make BOARD=qemu_x86 qemu

##Troubleshooting##

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

```
make clean          # discard results of previous builds
                    # but keep existing configuration info
```
or
```
make pristine       # discard results of previous builds
                    # and restore pre-defined configuration info
```

##Testing on host##

Using Chrome Dev (on Android or ChromeOS) seems to be the most stable (otherwise
the temperature readings might stop after a while. If you cannot connect or the
temperature isn't being notified, please restart Chrome. This is mostly an issue
on Android. Make sure you turn on Web Bluetooth in the about:flags

Disable bluetoothd on your Ubuntu to feel it up:

```
/etc/init.d/bluetooth stop
```

Check your interface number, a ```hci0``` would mean device 0

```
sudo hciconfig -a
```

Inside your bluez source dir

```
sudo ./btproxy -u -i <device-number>
```

Now compile and run qemu on your desktop:

```
make BOARD=qemu_x86 qemu
```