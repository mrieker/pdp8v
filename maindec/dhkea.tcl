loadbin bins/08-DHKEA.bin
reset 0200
swreg 0
iodev tty pipes /dev/null -
iodev tty debug 0
option set haltstop 1
option set mintimes 1
puts "starting..."
iodev tty stopon "HIGHEST FIELD (0-7)?  "
run
wait
if {[string compare -length 10 [stopreason] "tty stopon"] != 0} {
  puts "bad stop [stopreason] [cpu get]"
  exit
}
iodev tty stopon ""
run
after 333
iodev tty inject -echo "7\r"
wait
puts "stopped: [stopreason] [cpu get]"
exit
