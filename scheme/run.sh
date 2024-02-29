export iodevtty='-|-'
export iodevtty_debug=0
exec ../driver/raspictl "$@" -startpc 0200 scheme.oct
