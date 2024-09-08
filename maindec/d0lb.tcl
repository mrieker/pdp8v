loadbin bins/8E-D0LB.bin
reset 0200
swreg 05000
iodev tty pipes /dev/null -
iodev tty debug 0
option set mintimes 1
puts "starting..."
run
wait
puts "stopped: [stopreason] [cpu get]"
exit
