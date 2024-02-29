
# ../netgen/netgen.sh /dev/null -sim testdff.tcl

proc VerifyQ {qbitsb} {
    set qbitis [examine Q]
    echo "qbit is $qbitis sb $qbitsb"
    if {$qbitis != $qbitsb} {
        pause
    }
}

module DFF

force _PC 0
force _PS 1
force _SC 1
force T 0
force D 0

echo "preclear"

run 100
VerifyQ 0

force _PC 1
run 100
VerifyQ 0

echo "preset"

force _PS 0
run 100
VerifyQ 1

force _PS 1
run 100
VerifyQ 1

echo "clock in a zero"

force T 1
run 100
VerifyQ 0

force D 1
run 100
VerifyQ 0

force T 0
run 100
VerifyQ 0

force D 0
run 100
VerifyQ 0

force D 1
run 100
VerifyQ 0

echo "clock in a one"

force T 1
run 100
VerifyQ 1

force D 0
run 100
VerifyQ 1

force T 0
run 100
VerifyQ 1

force D 1
run 100
VerifyQ 1

force D 0
run 100
VerifyQ 1

echo "SUCCESS"

