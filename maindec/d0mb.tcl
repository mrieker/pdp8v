loadbin bins/8E-D0MB.bin
reset 0200
swreg 0
iodev tty pipes /dev/null -
iodev tty debug 1
option set mintimes 1
run ; wait
if {[cpu get pc] != 00202} {
    puts "bad pc [cpu get]"
    exit
}
puts "starting..."
while {true} {
    run
    wait
    if {[cpu get pc] != 00252} break
    puts "successful pass"
}
puts "stopped: [stopreason] [cpu get]"
exit
