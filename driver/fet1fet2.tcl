# alternate FETCH1,FETCH2 states
# ./tclboardtest all fet1fet2.tcl
while {true} {
    # reset to FETCH1
    force RESET 1
    halfcycle
    force RESET 0
    halfcycle
    set f1 [examstate]
    set e1 [expr {($f1 == "FETCH1") ? "" : "**ERROR**"}]
    puts "FETCH1: $f1 $e1"
    # clock to FETCH2
    force CLOCK 1
    halfcycle
    force CLOCK 0
    halfcycle
    set f2 [examstate]
    set e2 [expr {($f2 == "FETCH2") ? "" : "**ERROR**"}]
    puts "                FETCH2: $f2 $e2"
}
