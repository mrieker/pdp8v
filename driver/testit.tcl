##    Copyright (C) Mike Rieker, Beverly, MA USA
##    www.outerworldapps.com
##
##    This program is free software; you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation; version 2 of the License.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
##
##    You should have received a copy of the GNU General Public License
##    along with this program; if not, write to the Free Software
##    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##    http://www.gnu.org/licenses/gpl-2.0.html

#
#  processed by tclboardtest to test tubes
#  generates random instructions and checks paddles every cycle
#
#   ./tclboardtest          all testit.tcl [<sv>]  # run on tubes (raspi)
#   ./tclboardtest -csrclib all testit.tcl [<sv>]  # run on a PC (simulator)
#   ./tclboardtest -zynqlib all testit.tcl [<sv>]  # run on zynq (fpga sim)
#

set sv [expr {([llength $argv] > 1) && [lindex $argv 1]}]   ;# 0=quiet; 1=print states
set now 0                                                   ;# current simulation time
expr {srand (12345)}

set hidnir8 0                                               ;# IR<08> not avaiable on paddles

# output forced value to gpio connector
# wait for signals to soak into tubes
# read paddles and gpio connector
proc HalfCyc {} {
    global now
    halfcycle
    incr now
}

# print message
proc Echo {msg} {
    puts $msg
}

# send value coming out of RASPI going into the processor

proc ForceMQ {val} {
    global hidnir8

    force QENA 1 DENA 0 DATA 0[format %04o $val]

    ;# IRQ<08> not available on paddles so save value sent to tubes in FETCH2 cycle
    if {[exam fetch2q]} {set hidnir8 [expr {($val >> 8) & 1}]}
}

proc ForceMQL {val} {
    force QENA 1 DENA 0 LINK [expr {$val & 1}]
}

proc ForceMQOff {} {
    force QENA 0 DENA 1
}

proc ForceClock {on} {
    force CLOCK [expr {$on & 1}]
}

proc ForceReset {on} {
    force RESET [expr {$on & 1}]
}

proc ForceIoSkp {on} {
    force IOS [expr {$on & 1}]
}

proc ForceIntRq {on} {
    force IRQ [expr {$on & 1}]
}

# get register contents

proc GetACBits {} {
    return [Int2Bin [exam acq] 12]
}

proc GetALUQBits {} {
    return [Int2Bin [expr {[exam _aluq] ^ 0xFFF}] 12]
}

proc GetIRQ1108Bits {} {
    global hidnir8
    return [Int2Bin [expr {[exam irq] >> 9}] 3]$hidnir8
}

proc GetLink {} {
    return [exam lnq]
}

proc GetMABits {} {
    return [Int2Bin [exam maq] 12]
}

proc GetPCBits {} {
    return [Int2Bin [exam pcq] 12]
}

# see if the processor is in the given state
# fetch1,2, defer1,2,3, intak1 are directly on the paddles cuz they go to the LEDs
# also grpa1, iot2, tad3 are directly on the paddles
# the other various exec states must be decoded from the IR and MA contents read from paddles
proc GetState {name} {
    switch $name {
        arith1  {
            set opcode   [exam irq]
            set isexec1  [exam exec1q]
            return       [expr {$isexec1 && (($opcode & 06000) == 00000)}]
        }

        and2    {
            set opcode   [exam irq]
            set acqzero  [exam acqzero]
            set isexec2  [exam exec2q]
            set op0xxx   [expr {($opcode & 07000) == 00000}]
            set op1xxx   [expr {($opcode & 07000) == 01000}]
            return       [expr {$isexec2 && ($op0xxx || $op1xxx && $acqzero)}]
        }

        tad2    {
            set opcode   [exam irq]
            set acqzero  [exam acqzero]
            set isexec2  [exam exec2q]
            set op1xxx   [expr {($opcode & 07000) == 01000}]
            return       [expr {$isexec2 && $op1xxx && ! $acqzero}]
        }

        isz1    {
            set opcode   [exam irq]
            set isexec1  [exam exec1q]
            set op2xxx   [expr {($opcode & 07000) == 02000}]
            return       [expr {$isexec1 && $op2xxx}]
        }

        isz2    {
            set opcode   [exam irq]
            set isexec2  [exam exec2q]
            set op2xxx   [expr {($opcode & 07000) == 02000}]
            return       [expr {$isexec2 && $op2xxx}]
        }

        isz3    {
            set opcode   [exam irq]
            set isexec3  [exam exec3q]
            set op2xxx   [expr {($opcode & 07000) == 02000}]
            return       [expr {$isexec3 && $op2xxx}]
        }

        dca1    {
            set opcode   [exam irq]
            set isexec1  [exam exec1q]
            set op3xxx   [expr {($opcode & 07000) == 03000}]
            return       [expr {$isexec1 && $op3xxx}]
        }

        dca2    {
            set opcode   [exam irq]
            set isexec2  [exam exec2q]
            set op3xxx   [expr {($opcode & 07000) == 03000}]
            return       [expr {$isexec2 && $op3xxx}]
        }

        jms1    {
            set opcode   [exam irq]
            set isexec1  [exam exec1q]
            set op4xxx   [expr {($opcode & 07000) == 04000}]
            return       [expr {$isexec1 && $op4xxx}]
        }

        jms2    {
            set opcode   [exam irq]
            set isexec2  [exam exec2q]
            set op4xxx   [expr {($opcode & 07000) == 04000}]
            return       [expr {$isexec2 && $op4xxx}]
        }

        jms3    {
            set opcode   [exam irq]
            set isexec3  [exam exec3q]
            set op4xxx   [expr {($opcode & 07000) == 04000}]
            return       [expr {$isexec3 && $op4xxx}]
        }

        jmp1    {
            set opcode   [exam irq]
            set isexec1  [exam exec1q]
            set op5xxx   [expr {($opcode & 07000) == 05000}]
            return       [expr {$isexec1 && $op5xxx}]
        }

        iot1    {
            set opcode   [expr {[exam irq] | ([exam maq] & 00777)}]
            set isexec1  [exam exec1q]
            set op6xxx   [expr {($opcode & 07000) == 06000}]
            set op74xx   [expr {($opcode & 07400) == 07400}]
            set op74x7   [expr {$op74xx && (($opcode & 7) != 0)}]
            return       [expr {$isexec1 && ($op6xxx || $op74x7)}]
        }

        grpa1   {
            return       [expr {[exam _grpa1q] ^ 1}]
        }

        grpb1   {
            set opcode   [expr {[exam irq] | ([exam maq] & 00777)}]
            set isexec1  [exam exec1q]
            set op74xx   [expr {($opcode & 07400) == 07400}]
            set op74x0   [expr {$op74xx && (($opcode & 7) == 0)}]
            return       [expr {$isexec1 && $op74x0}]
        }

        default {
            return [exam ${name}q]
        }
    }
}

proc GetCtrl {name} {
    if {$name == "_endinst"} {
        set isexec2  [exam exec2q]
        set isexec3  [exam exec3q]
        set opcode   [exam irq]
        set acqzero  [exam acqzero]
        set op0xxx   [expr {($opcode & 07000) == 00000}]
        set op1xxx   [expr {($opcode & 07000) == 01000}]
        set isand2   [expr {$isexec2 && ($op0xxx || $op1xxx && $acqzero)}]
        set istad3   [exam tad3q]
        set isisz3   [expr {$isexec3 && (($opcode & 07000) == 02000)}]
        set isdca2   [expr {$isexec2 && (($opcode & 07000) == 03000)}]
        set isiot2   [exam iot2q]
        set isgrpa1  [expr {[exam _grpa1q] ^ 1}]
        return [expr {! ($isand2 | $istad3 | $isdca2 | $isisz3 | $isiot2 | $isgrpa1)}]
    } else {
        return [exam $name]
    }
}

proc Error {msg} {
    puts $msg
    dumpboard acl
    dumpboard alu
    dumpboard ma
    dumpboard pc
    dumpboard rpi
    dumpboard seq
    exit 1
}

#
# use test script common with de0 testing
#
source "../modules/commontest.tcl"

#
# run test until aborted with control-C
#
TestInit
set i 0
while {true} {
    incr i
    TestStep $i
}
