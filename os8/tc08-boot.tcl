reset [loadlink tc08boot.oct]                       ;# reset processor; load OS/8 boot program in memory; set pc = 7613
option set os8zap 1                                 ;# zap out the ISZ/JMP.-1 loops in os/8
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303                              ;# put tty on telnet port 12303
##iodev tty pipes -
iodev tc08 loadrw 0 dta0.tu56                       ;# bootable dectape
puts ""
puts "  telnet <thishost> 12303"
puts "  enter 'exit' command here when done using OS/8"
puts "  enter 'helpos8' command to print out commands for OS/8"
puts ""
proc helpos8 {} {
    puts ""
    puts "  tapes as set up by tc08-boot.tcl"
    puts "    DTA0 = boot/system tape"
    puts "    DTA1 = available"
    puts ""
    puts "  to change file used for tape DTA<n>:"
    puts "    iodev tc08 loadrw <n> <filename>.tu56"
    puts ""
    puts "  disks as set up by tc08-boot.tcl"
    puts "    RKA0,RKB0 = available"
    puts "    RKA1,RKB1 = available"
    puts ""
    puts "  to change file used for disk pair RKA<n>,RKB<n>:"
    puts "    iodev rk8je loadrw <n> <filename>.rk05"
    puts ""
    puts "  to import a text file into OS/8:"
    puts "    here: iodev ptape load reader -ascii -quick <textfilename>"
    puts "    in OS/8:"
    puts "      .R PIP"
    puts "      *os8file<PTR:"
    puts "      ^<carriage-return>"
    puts "      *<control-C>"
    puts ""
    puts "  to export a text file from OS/8:"
    puts "    here: iodev ptape load punch -ascii -quick <textfilename>"
    puts "    in OS/8: COPY PTP:<os8file"
    puts "    here: iodev ptape unload punch"
    puts ""
}
run                                                 ;# start clocking the tubes
