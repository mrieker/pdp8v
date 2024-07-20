reset [loadlink rk8jeboot.oct]                      ;# reset processor; load OS/8 boot program in memory; set pc = 0023
option set os8zap 1                                 ;# zap out the ISZ/JMP.-1 loops in os/8
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303                              ;# put tty on telnet port 12303
##iodev tty pipes -
iodev rk8je loadrw 0 rkab0.rk05                     ;# load drive 0 file
iodev rk8je loadrw 1 rkab1.rk05                     ;# load drive 1 file
iodev rk8je loadrw 2 rkab2.rk05                     ;# load drive 2 file
puts ""
puts "  telnet <thishost> 12303"
puts "  enter 'exit' command here when done using OS/8"
puts "  enter 'helpos8' command to print out commands for OS/8"
puts ""
proc helpos8 {} {
    puts ""
    puts "  disks as set up by ock-boot.tcl"
    puts "    RKA0,RKB0 = boot/system disk"
    puts "    RKA1,RKB1 = scratch/user disk"
    puts "    RKA2,RKB2 = OS/8 sources"
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
