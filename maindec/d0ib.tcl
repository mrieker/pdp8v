loadbin bins/8E-D0IB.bin
reset 0200
swreg 0
iodev tty pipes /dev/null -
iodev tty debug 1
option set haltstop 1
option set mintimes 1
puts "starting..."
run
wait
puts "stopped: [stopreason] [cpu get]"
exit
