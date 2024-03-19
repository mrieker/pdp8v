export iodevtty=:12303
export iodevtty_cps=960
export iodevtty_debug=0
exec ../driver/raspictl "$@" -startpc 0200 scheme.oct
