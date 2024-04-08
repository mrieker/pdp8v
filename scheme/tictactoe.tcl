reset [loadlink scheme.oct]
option set cmdargs scheme.oct {(load "tictactoe.scm")}
iodev tty speed 115200
iodev tty pipes -
run ; wait ; exit
