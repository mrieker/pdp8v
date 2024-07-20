iodev tty pipes /dev/null - ;# telnet 12303
iodev tty speed 115200
swreg 01000
iodev dtape loadrw 1 dttest.dt
iodev dtape gofast [getenv gofast 0]
iodev dtape debug [getenv debug 0]
reset [loadlink dttest.oct]
puts "running..."
run ; wait
