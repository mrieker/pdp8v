iodev tty telnet 12303
iodev dtape loadrw 1 dttest.dt
swreg 01000
##iodev dtape debug 1
reset [loadlink dttest.oct]
puts "running..."
run ; wait
