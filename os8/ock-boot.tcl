reset [loadlink rk8jeboot.oct]          ;# reset processor; load OS/8 boot program in memory; set pc = 0023
option set os8zap 1                     ;# zap out the ISZ/JMP.-1 loop in os/8
option set skipopt 1                    ;# block when doing ioskip/JMP.-1 loop
swreg 0                                 ;# set switchregister to 0
iodev tty lcucin 1                      ;# convert lowercase to uppercase on input
iodev tty telnet 12303                  ;# put tty on telnet port 12303
##iodev tty pipes -
iodev rk8je loadrw 0 rkab0.rk05         ;# load drive 0 file
iodev rk8je loadrw 1 rkab1.rk05         ;# load drive 1 file
iodev rk8je loadrw 2 rkab2.rk05         ;# load drive 2 file
run                                     ;# start clocking the tubes 
puts ""
puts "  telnet <thishost> 12303"
puts "  enter 'exit' command when done using OS/8"
puts ""
