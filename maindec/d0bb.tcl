loadbin bins/8E-D0BB.bin
reset 0200
swreg 0
iodev tty pipes -
iodev tty debug 1
option set haltstop 1
option set mintimes 1
puts "starting..."
run ; wait ; exit
