iodev tty pipes /dev/null - ;# telnet 12303
iodev tty speed 115200
swreg 01000
iodev tc08 loadrw 1 dttest.dt
iodev tc08 debug [getenv debug 0]
reset [loadlink dttest.oct]
puts "running..."
run ; wait
