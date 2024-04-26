Vacuum tube implementation of PDP-8/V using a Raspberry PI for memory and I/O devices.

Website:  https://www.outerworldapps.com/pdp8v

To generate PCBs (can be run from a PC or a RasPI):
    cd netgen
    make
    cd ../modules
    make   << generates unrouted ACL, ALU, MA, PC, RPI, SEQ boards

    open each of the above in its directory in the kicads directory
    ... and route route route!

Or use these routed PCBs that match this rev:
    https://www.outerworldapps.com/pdp8v/acl-2024-04-15.tgz
    https://www.outerworldapps.com/pdp8v/alu-2024-04-15.tgz
    https://www.outerworldapps.com/pdp8v/ma-2024-04-15.tgz
    https://www.outerworldapps.com/pdp8v/pc-2024-04-15.tgz
    https://www.outerworldapps.com/pdp8v/rpi-2024-04-15.tgz
    https://www.outerworldapps.com/pdp8v/seq-2024-04-15.tgz

To run a PDP-8 program (from a RasPI plugged into RPI board):
    cd driver
    make raspictl.`uname -m`
    ./raspictl -binloader <name-of-some-bin-format-file>
            or -rimloader <name-of-some-rim-format-file>

See the website for other options for running programs.
