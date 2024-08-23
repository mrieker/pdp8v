loadbin d3bc-pb
iodev tty pipes /dev/null -
iodev tc08 loadrw 1 tape1.tu56
swreg 02000
option set haltstop 1
reset 0203
run ; wait
if {([cpu get ac] != [swreg]) || ([cpu get pc] != 00224)} {
    puts "bad ac/pc [cpu get]"
    exit
}
puts "starting..."
run ; wait ; exit
