reset [loadlink scheme.oct]
iodev tty pipes - -
iodev tty speed [getenv ttyspeed 1000000]
run
wait
puts "stopreason=[stopreason]"
exit
