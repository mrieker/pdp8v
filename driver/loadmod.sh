#!/bin/bash
if [ "`uname -m`" == armv6l ]
then
    if ! lsmod | grep -q ^armmhz
    then
        cd `dirname $0`
        if [ ! -f km-armmhz/armmhz.`uname -r`.ko ]
        then
            cd km-armmhz
            rm -f armmhz.ko armmhz.mod* armmhz.o modules.order Module.symvers .armmhz* .modules* .Module*
            make
            mv armmhz.ko armmhz.`uname -r`.ko
            cd ..
        fi
        sudo insmod km-armmhz/armmhz.`uname -r`.ko
    fi
fi
if [ "`uname -m`" == armv7l ]
then
    if ! lsmod | grep -q ^enabtsc
    then
        cd `dirname $0`
        if [ ! -f km/enabtsc.`uname -r`.ko ]
        then
            cd km
            rm -f enabtsc.ko enabtsc.mod* enabtsc.o modules.order Module.symvers .enabtsc* .modules* .Module*
            make
            mv enabtsc.ko enabtsc.`uname -r`.ko
            cd ..
        fi
        sudo insmod km/enabtsc.`uname -r`.ko
    fi
fi
if grep -q 'Xilinx Zynq' /proc/cpuinfo
then
    if ! lsmod | grep -q ^zynqgpio
    then
        cd `dirname $0`
        if [ ! -f km-zynqgpio/zynqgpio.`uname -r`.ko ]
        then
            cd km-zynqgpio
            rm -f zynqgpio.ko zynqgpio.mod* zynqgpio.o modules.order Module.symvers .zynqgpio* .modules* .Module*
            make
            mv zynqgpio.ko zynqgpio.`uname -r`.ko
            cd ..
        fi
        sudo insmod km-zynqgpio/zynqgpio.`uname -r`.ko
    fi
fi
