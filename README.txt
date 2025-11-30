Vacuum tube implementation of PDP-8 using a Raspberry PI for memory and I/O devices.

Website:  https://www.outerworldapps.com/pdp8v

To build one:

* Download software from github https://github.com/mrieker/pdp8v
  ...onto a Linux PC

* Compile netgen, the program that reads in the module files to create the PCBs
    cd netgen
    make

* Create the semi-routed crystal rectifier and front panel/Raspberry PI PCBs
    cd modules
    make

* Route the PCBs using KiCad
    order them and assemble
    look at top of each board's .rep file to get part counts

    Or use these routed PCBs that match this rev:
        https://www.outerworldapps.com/pdp8v/acl-2024-04-15.tgz
        https://www.outerworldapps.com/pdp8v/alu-2024-04-15.tgz
        https://www.outerworldapps.com/pdp8v/ma-2024-04-15.tgz
        https://www.outerworldapps.com/pdp8v/pc-2024-04-15.tgz
        https://www.outerworldapps.com/pdp8v/rpi-2024-04-15.tgz
        https://www.outerworldapps.com/pdp8v/seq-2024-04-15.tgz

* Order and assemble 41 of the 8-tube PCBs

* Order and assemble backplane PCB

* Order and assemble 4 paddle boards for testing
  ...including IO-Warrior 100 modules and USB cables

* Order power supplies:
    1)  5V @ 3A
    4) 48V @ 6A
    2) 160V variable @ 3A
       ...probably can get away with 120V variable @ 3A
          cuz actually need just 87.5V and 117.5V

* Get 5 20" box fans at hardware store

* Build Raspberry PI control programs

    copy files to Raspberry PI or NFS serve them to RasPI
    cd driver
    make
      note: these programs will also build on a Linux PC
            but must be on RaspPI when testing front panel/RasPI board (or you will get /dev/gpiomem not found)

  unit test a board plugged into paddles either on a RasPI or Linux PC:
    ./autoboardtest -verbose -page <boardname>
      <boardname> = any one or combination of: acl alu ma pc rpi seq
    - when testing RPI (raspi/front panel),
      it must be run on RasPI plugged into GPIO connector on front panel
    - when testing more than one board, plug them into backplane and plug paddles into backplane
    - generates random data sent out over paddles and, only if RPI board present, the GPIO connector
    - compares paddle pins to simulated paddle pins, prints error and stops if difference found

  test mode, generates random instructions, without tubes:
    ./raspictl -csrclib -randmem -mintimes
      generates a C++ equivalent of the module files for simulation

  test mode, generates random instructions, with tubes:
    ./raspictl -randmem -mintimes
      accesses the tubes using the GPIO connector
      compares GPIO input pins to simulator, prints error and stops if difference found

  run some .bin file on the simulator:
    ./raspictl -csrclib -binloader -startpc <startaddress> <nameofbinfile>

  run some .bin file on the tubes:
    ./raspictl -binloader -startpc <startaddress> <nameofbinfile>
      compares GPIO input pins to simulator, prints error and stops if difference found

See the website for other options for running programs.
