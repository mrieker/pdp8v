#!/bin/bash
#
#  Set owner of iowarrior devices to current user
#
if [ -d /dev/usb ]
then
    find /dev/usb -name iowarrior\* ! -uid $UID | xargs -r sudo chown $UID
fi
