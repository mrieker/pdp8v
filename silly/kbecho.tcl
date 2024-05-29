reset [loadlink kbecho.oct]                         ;# reset processor; load OS/8 boot program in memory; set pc = 0023
set ontubes [expr {[gpio libname] == "physlib"}]    ;# 1=tubes; 0=simulator
option set skipopt [getenv skipopt $ontubes]        ;# optimize IOSKIP/JMP.-1 to act like halt
option set tubesaver [getenv tubesaver $ontubes]    ;# for tubes, cycle random opcodes/data for halts
puts "ontubes=$ontubes skipopt=[option get skipopt] tubesaver=[option get tubesaver]"
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303                              ;# put tty on telnet port 12303
run
