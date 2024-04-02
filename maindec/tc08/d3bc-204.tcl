loadbin d3bc-pb
iodev tty telnet 12303
iodev dtape loadrw 1 tape1.dtp
iodev dtape loadrw 2 tape2.dtp
iodev dtape loadrw 3 tape3.dtp
iodev dtape debug 1
swreg 03400                 ;# use drives 1,2,3
reset 0204                  ;# select read/write test
option set haltstop 1
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
##option set stopat 00600     ;# beginning of search testprogram
##run ; wait
##option set printinstr 1     ;# turn on tracing
##option set stopat 01245     ;# where it starts printing error message
run ; wait
