loadbin d3ra-pb
option set haltstop 1
iodev tc08 loadrw 1 tape1.tu56
iodev tc08 loadrw 2 tape2.tu56
iodev tc08 loadrw 3 tape3.tu56
iodev tc08 debug [getenv debug 0]
iodev tty pipes /dev/null -
iodev tty stopon "ADDRS INC\r\n"
reset 00200
swreg 03400
run
wait
if {[cpu get pc] != 00210} {
    puts "bad pc [cpu get]"
    exit
}
swreg 00000
;# patch test to do DTSF/JMP.-1 while waiting instead of instruction tests
;# don't use HLT cuz it is used to indicate errors
writemem 03001 06001    ;# ION
writemem 03002 06771    ;# DTSF skip on flag
writemem 03003 05202    ;# JMP .-1
writemem 03004 05204    ;# JMP .
writemem 03051 05600    ;# JMPI WATINT
puts "d3ra: running..."
run
wait
puts "d3ra: [stopreason]"
puts "d3ra: [cpu get]"
