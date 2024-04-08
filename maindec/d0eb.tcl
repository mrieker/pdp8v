loadbin bins/8E-D0EB.bin
reset 0200
swreg 0
iodev tty pipes -
option set haltstop 1
option set mintimes 1
puts "starting..."
run ; wait ; exit
