iodev tty telnet 12303
iodev tty speed 115200
iodev dtape loadrw 1 dttest.dt
iodev dtape gofast 1
swreg 01000
##iodev dtape debug 1
reset [loadlink dttest.oct]
puts "running..."
run ; wait
