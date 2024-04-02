loadbin d3bc-pb
iodev tty telnet 12303
iodev dtape loadrw 1 tape1.dtp
iodev dtape loadrw 2 tape2.dtp
iodev dtape debug 1
swreg 03000
option set haltstop 1
reset 0203
run ; wait
if {([cpu get ac] != [swreg]) || ([cpu get pc] != 00224)} {
    puts "bad ac/pc [cpu get]"
    exit
}
puts "starting..."
run ; wait
