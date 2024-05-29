reset [loadlink rk8jeboot.oct]                      ;# reset processor; load OS/8 boot program in memory; set pc = 0023
option set os8zap 1                                 ;# zap out the ISZ/JMP.-1 loop in os/8
set ontubes [expr {[gpio libname] == "physlib"}]    ;# 1=tubes; 0=simulator
option set skipopt [getenv skipopt 1]               ;# optimize IOSKIP/JMP.-1 to act like halt
option set tubesaver [getenv tubesaver $ontubes]    ;# for tubes, cycle random opcodes/data for halts
puts "ontubes=$ontubes skipopt=[option get skipopt] tubesaver=[option get tubesaver]"
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303                              ;# put tty on telnet port 12303
##iodev tty pipes -
iodev rk8je loadrw 0 rkab0.rk05                     ;# load drive 0 file
iodev rk8je loadrw 1 rkab1.rk05                     ;# load drive 1 file
iodev rk8je loadrw 2 rkab2.rk05                     ;# load drive 2 file
run                                                 ;# start clocking the tubes
puts ""
puts "  telnet <thishost> 12303"
puts "  enter 'exit' command when done using OS/8"
puts ""
