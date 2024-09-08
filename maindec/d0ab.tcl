loadbin bins/8E-D0AB.bin
reset 0200
swreg 07777
iodev tty pipes /dev/null -
iodev tty debug 1
option set mintimes 1
run ; wait
if {([cpu get pc] != 00147)} {
    puts "bad pc [cpu get]"
    exit
}
puts "starting..."
run
wait
puts "stopped: [stopreason] [cpu get]"
exit
