MACH   = $(shell uname -m)

default: iowkit_1.7_$(MACH)/libiowkit-1.7.0/insdir IOW56_Datasheet.pdf

iowkit_1.7_$(MACH)/libiowkit-1.7.0/insdir: iowkit_1.7_$(MACH)
	cd iowkit_1.7_$(MACH)/libiowkit-1.7.0 ; ./configure --prefix=`pwd`/insdir
	cd iowkit_1.7_$(MACH)/libiowkit-1.7.0 ; sed 's/CFLAGS = -O2 -Wall -g/CFLAGS = -fPIC -O2 -Wall -g/g' src/Makefile > src/Makefile.tmp
	cd iowkit_1.7_$(MACH)/libiowkit-1.7.0 ; mv -f src/Makefile.tmp src/Makefile
	cd iowkit_1.7_$(MACH)/libiowkit-1.7.0 ; make CFLAGS=-fPIC
	cd iowkit_1.7_$(MACH)/libiowkit-1.7.0 ; make install

iowkit_1.7_$(MACH): linux_sdk.tar.gz
	rm -rf iowkit_1.7
	tar xzf linux_sdk.tar.gz
	mv iowkit_1.7 iowkit_1.7_$(MACH)

linux_sdk.tar.gz: IO-Warrior_SDK_linux.zip
	unzip IO-Warrior_SDK_linux.zip

IO-Warrior_SDK_linux.zip:
	wget https://www.codemercs.com/downloads/iowarrior/IO-Warrior_SDK_linux.zip

IOW56_Datasheet.pdf:
	wget https://www.codemercs.com/downloads/iowarrior/IOW56_Datasheet.pdf

