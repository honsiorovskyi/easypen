Genius EasyPen 3x4 serial tablet driver for Linux kernel
========================================================

This driver is used to provide and event-based input device
which can be connected to the X-server using `evdev` module.

In order to use this driver you should attach your serial port
to which your physical device is connected to the kernel using
the patched version of the `inputattach` utility (patch attached).
This `inputattach` utility will initialize your physical device
and then attach it to the kernel. Then your device will be 'catched'
by the X-server.

Installation
------------

* Install `linux-headers` and `dkms`. For example, for Ubuntu:
```
  sudo apt-get install linux-headers-`uname -r` dkms
```

* Copy sources of the driver (or clone the repository) to the
`/usr/src` folder:
```
  sudo cp -R <path-to-the-driver>/easypen /usr/src/easypen-0.1
```

* To install module open a terminal and enter:
```
    sudo dkms -k `uname -r` -m easypen/0.1 install
```

* To uninstall module open a terminal and enter:
```
    sudo dkms -k `uname -r` -m easypen/0.1 remove
```

* To get patched version of `inputattach` utility do:
```
    # get the sources of inputattach utility
    git clone git://linuxconsole.git.sourceforge.net/gitroot/linuxconsole/linuxconsole
    
    # apply patch
    cd linuxconsole
    patch -p1 < <path-to-the-driver>/easypen/inputattach-easypen-0.1.patch
    
    # compile inputattach utility
    cd utils
    make inputattach
    
    # copy the binary file somewhere
    sudo cp inputattach /usr/local/bin
```

* To test your device install run:
```
    sudo inputattach --easypen /dev/ttyS0
```

Instead of `/dev/ttyS0` use the serial port device which your physical
device is connected to.

