loadbin d3ra-pb
option set haltstop 1
iodev tc08 loadrw 1 tape1.dtp
iodev tc08 loadrw 2 tape2.dtp
iodev tc08 loadrw 3 tape3.dtp
iodev tc08 debug [getenv debug 0]
iodev tty pipes /dev/null -
reset 00200
swreg 03400
run ; wait
if {[cpu get pc] != 00210} {
    puts "bad pc [cpu get]"
    exit
}
swreg 00000
puts "starting..."
run ; wait
