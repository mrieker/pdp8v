loadbin d3eb-pb
iodev tty telnet 12303
iodev dtape loadrw 1 tape1.dtp
iodev dtape debug 0
swreg 01074
option set haltstop 1
reset 0200
run ; wait
if {[cpu get pc] != 00224} {
    puts "bad ac/pc [cpu get]"
    exit
}
puts "starting..."
run ; wait
