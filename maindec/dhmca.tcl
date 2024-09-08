loadbin bins/MAINDEC-08-DHMCA-b-pb.bin
reset 0200
swreg 00007
iodev tty pipes /dev/null -
iodev tty debug 1
option set mintimes 1
option set jmpdotstop 1
puts "starting..."
run
wait
puts "stopped: [stopreason] [cpu get]"
exit
