loadbin bins/8E-D0EB.bin
reset 0200
swreg 0
iodev tty pipes /dev/null -
option set mintimes 1
puts "starting..."
run
wait
puts "stopped: [stopreason] [cpu get]"
exit
