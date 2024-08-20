reset [loadlink scheme.oct]
iodev tty pipes - -
iodev tty speed [getenv ttyspeed 1000000]
dyndis enable 1
run
wait
puts [stopreason]
dyndis dump test.dyn
exit
