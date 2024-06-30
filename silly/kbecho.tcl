reset [loadlink kbecho.oct]                         ;# reset processor; load OS/8 boot program in memory; set pc = 0023
puts "ontubes=$ontubes skipopt=[option get skipopt] tubesaver=[option get tubesaver]"
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303                              ;# put tty on telnet port 12303
run
