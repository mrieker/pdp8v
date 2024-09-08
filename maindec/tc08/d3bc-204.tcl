loadbin d3bc-pb
iodev tty telnet 12303
iodev tc08 loadrw 1 tape1.tu56
iodev tc08 debug 0
swreg 02000                 ;# use drive 1
reset 0204                  ;# select read/write test
run ; wait
if {([cpu get ac] != [swreg]) || ([cpu get pc] != 00224)} {
    puts "bad ac/pc [cpu get]"
    exit
}
set swregval ""
if {[info exists ::env(swreg)]} {
    set swregval $::env(swreg)
}
if {$swregval == ""} {
    set swregval 017
}
puts [format "starting with swreg %04o" $swregval]
swreg $swregval
run
wait
exit
