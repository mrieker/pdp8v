iodev tty pipes - -
iodev tty speed 1000000
set sa [loadlink life.oct]
reset $sa
run
wait
exit
