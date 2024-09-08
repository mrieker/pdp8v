loadbin d3bc-pb
iodev tty pipes /dev/null -
iodev tc08 loadrw 1 tape1.tu56
iodev tc08 debug 0
swreg 02000
reset 0202
run ; wait
if {([cpu get ac] != [swreg]) || ([cpu get pc] != 00224)} {
    puts "bad ac/pc [cpu get]"
    exit
}
puts "starting..."

proc promptreply {pmt rep} {
    iodev tty stopon $pmt
    run ; wait
    if {[stopreason] != "tty stopon $pmt"} {
        puts "echo bad stop [stopreason] [cpu get]"
        exit
    }
    iodev tty inject -echo $rep
}

proc gofwd {} {
    global blkno
    promptreply "ALL OTHERS BKWD " "F"
    set ez 0
    if {$blkno > 02500} {
        promptreply "END ZONE" ""
        set ez 1
    }
    incr blkno
    promptreply [format "%04o FIRST" $blkno] ""
    promptreply "ALL OTHERS PRINT " "C"
    incr blkno 0200
    if $ez {set blkno 02701}
    promptreply [format "%04o LAST" $blkno] ""
    return $ez
}

proc gobkwd {} {
    global blkno
    promptreply "ALL OTHERS BKWD " "B"
    set ez 0
    if {$blkno < 00201} {
        promptreply "END ZONE" ""
        set ez 1
    }
    incr blkno -1
    promptreply [format "%04o FIRST" $blkno] ""
    promptreply "ALL OTHERS PRINT " "C"
    incr blkno -0200
    if $ez {set blkno 00000}
    promptreply [format "%04o LAST" $blkno] ""
    return $ez
}

set blkno -1
while true {
    if [gofwd] break
    if [gofwd] break
    gobkwd
}

set blkno 02702
while true {
    if [gobkwd] break
    if [gobkwd] break
    gofwd
}

puts ""
puts ""
puts "SUCCESS!"
puts ""
exit
