set ontubes [expr {[gpio libname] == "physlib"}]    ;# 1=tubes; 0=simulator
option set skipopt [getenv skipopt 1]               ;# optimize IOSKIP/JMP.-1 to act like halt
option set tubesaver [getenv tubesaver $ontubes]    ;# for tubes, cycle random opcodes/data for halts
puts "ontubes=$ontubes skipopt=[option get skipopt] tubesaver=[option get tubesaver]"
option set quiet 1                                  ;# suppress 'unsupported i/o opcode 6634,6674' messages
swreg 0                                             ;# set switchregister to 0
iodev tty lcucin 1                                  ;# convert lowercase to uppercase on input
iodev tty telnet 12303
iodev tty40 telnet 12340
iodev tty42 telnet 12342
iodev tty44 telnet 12344
iodev tty46 telnet 12346
iodev ptape load reader edu20c.pt
reset [loadlink rim-hispeed.oct]
run
puts ""
puts "  telnet into 12303,12340,12342,12344,12346"
puts ""
puts "  enter 'exit' command when done"
puts ""
puts "  takes about 1min 30sec to get question prompts on telnet 12303 port"
puts "  example answers to questions for 3 terminals (12303,12340,12342):"
puts ""
puts "    EDUSYSTEM 20  BASIC"
puts ""
puts "    NUMBER OF USERS (1 TO 8)?3"
puts "    PDP-8/L COMPUTER (Y OR N)?N"
puts "    STANDARD REMOTE TELETYPE CODES (Y OR N)?Y"
puts "    ANY UNUSED TERMINALS (Y OR N)?N"
puts "    DO YOU HAVE A HIGH SPEED PUNCH (Y OR N)?Y"
puts "    DO YOU HAVE A HIGH SPEED READER (Y OR N)?Y"
puts "    SAME AMOUNT OF STORAGE FOR ALL USERS?Y"
puts "    IS THE ABOVE CORRECT (Y OR N)?Y"
puts ""
puts "  then takes about 30sec to get READY prompts on all 3 terminals"
puts ""
