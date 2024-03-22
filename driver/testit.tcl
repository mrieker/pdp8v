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
#  processed by raspictl to test tubes
#  generates random instructions and checks paddles every cycle
#
#   ./raspictl          -script testit.tcl [<sv>]   # run on tubes
#   ./raspictl -csrclib -script testit.tcl [<sv>]   # run on a PC
#   ./raspictl -zynqlib -script testit.tcl [<sv>]   # run on zynq
#

set cmdargs [option get cmdargs]
set sv [expr {([llength $cmdargs] > 1) && [lindex $cmdargs 1]}] ;# 0=quiet; 1=print states
set now 0                                                       ;# current simulation time
expr {srand (12345)}

option set paddles 1        ;# make sure we have access to paddles

set gpiouts [gpio get g]    ;# read initial gpio connector state
set gpioins [gpio get]      ;# read initial paddle and gpio state
set hidnir8 0               ;# IR<08> not avaiable on paddles

# output forced value to gpio connector
# wait for signals to soak into tubes
# read paddles and gpio connector
proc HalfCyc {} {
    global gpioins gpiouts now
    gpio set g $gpiouts
    halfcycle
    incr now
    set gpioins [gpio get]
}

# print message
proc Echo {msg} {
    puts $msg
}

# send value coming out of RASPI going into the processor

proc ForceMQ {val} {
    global gpiouts hidnir8
    ;# [15:04] = DATA
    ;# [19] = QENA = on
    ;# [21] = DENA = off
    set gpiouts [expr {($gpiouts & 0xFDF000F) | 0x80000 | (($val & 0xFFF) << 4)}]

    if {[gpio decode [gpio get] fetch2q]} {set hidnir8 [expr {($val >> 8) & 1}]}
}

proc ForceMQL {val} {
    global gpiouts
    ;# [16] = LINK
    ;# [19] = QENA = on
    ;# [21] = DENA = off
    set gpiouts [expr {($gpiouts & 0xFDEFFFF) | 0x80000 | (($val & 1) << 16)}]
}

proc ForceMQOff {} {
    global gpiouts
    ;# [19] = QENA = off
    ;# [21] = DENA = on
    set gpiouts [expr {($gpiouts & 0xFF7FFFF) | 0x200000}]
}

proc ForceClock {on} {
    global gpiouts
    ;# [02] = CLOCK
    set gpiouts [expr {($gpiouts & 0xFFFFFFB) | (($on & 1) << 2)}]
}

proc ForceReset {on} {
    global gpiouts
    ;# [03] = RESET
    set gpiouts [expr {($gpiouts & 0xFFFFFF7) | (($on & 1) << 3)}]
}

proc ForceIoSkp {on} {
    global gpiouts
    ;# [17] = IOSKP
    set gpiouts [expr {($gpiouts & 0xFFDFFFF) | (($on & 1) << 17)}]
}

proc ForceIntRq {on} {
    global gpiouts
    ;# [20] = INTRQ
    set gpiouts [expr {($gpiouts & 0xFEFFFFF) | (($on & 1) << 20)}]
}

# get register contents

proc GetACBits {} {
    global gpioins
    return [Int2Bin [gpio decode $gpioins acq] 12]
}

proc GetALUQBits {} {
    global gpioins
    return [Int2Bin [expr {[gpio decode $gpioins _aluq] ^ 0xFFF}] 12]
}

proc GetIRQ1108Bits {} {
    global gpioins hidnir8
    return [Int2Bin [expr {[gpio decode $gpioins irq] >> 9}] 3]$hidnir8
}

proc GetLink {} {
    global gpioins
    return [gpio decode $gpioins lnq]
}

proc GetMABits {} {
    global gpioins
    return [Int2Bin [gpio decode $gpioins maq] 12]
}

proc GetPCBits {} {
    global gpioins
    return [Int2Bin [gpio decode $gpioins pcq] 12]
}

proc GetState {name} {
    global gpioins hidnir8

    set isexec1  [gpio decode $gpioins exec1q]
    set isexec2  [gpio decode $gpioins exec2q]
    set isexec3  [gpio decode $gpioins exec3q]

    set opcode   [expr {[gpio decode $gpioins irq] | ($hidnir8 << 8) | ([gpio decode $gpioins maq] & 00377)}]

    set acqzero  [gpio decode $gpioins acqzero]

    set op0xxx   [expr {($opcode & 07000) == 00000}]
    set op1xxx   [expr {($opcode & 07000) == 01000}]
    set op2xxx   [expr {($opcode & 07000) == 02000}]
    set op3xxx   [expr {($opcode & 07000) == 03000}]
    set op4xxx   [expr {($opcode & 07000) == 04000}]
    set op5xxx   [expr {($opcode & 07000) == 05000}]
    set op6xxx   [expr {($opcode & 07000) == 06000}]
    set op70xx   [expr {($opcode & 07400) == 07000}]
    set op74xx   [expr {($opcode & 07400) == 07400}]
    set op74x0   [expr {$op74xx && (($opcode & 7) == 0)}]
    set op74x7   [expr {$op74xx && (($opcode & 7) != 0)}]

    set isarith1 [expr {$isexec1 && (($opcode & 06000) == 00000)}]
    set isand2   [expr {$isexec2 && ($op0xxx || $op1xxx && $acqzero)}]
    set istad2   [expr {$isexec2 && $op1xxx && ! $acqzero}]
    set isisz1   [expr {$isexec1 && $op2xxx}]
    set isisz2   [expr {$isexec2 && $op2xxx}]
    set isisz3   [expr {$isexec3 && $op2xxx}]
    set isdca1   [expr {$isexec1 && $op3xxx}]
    set isdca2   [expr {$isexec2 && $op3xxx}]
    set isjms1   [expr {$isexec1 && $op4xxx}]
    set isjms2   [expr {$isexec2 && $op4xxx}]
    set isjms3   [expr {$isexec3 && $op4xxx}]
    set isjmp1   [expr {$isexec1 && $op5xxx}]
    set isiot1   [expr {$isexec1 && ($op6xxx || $op74x7)}]
    set isgrpa1  [expr {[gpio decode $gpioins _grpa1q] ^ 1}]
    set isgrpb1  [expr {$isexec1 && $op74x0}]

    switch $name {
        arith1  {return $isarith1}
        and2    {return $isand2}
        tad2    {return $istad2}
        isz1    {return $isisz1}
        isz2    {return $isisz2}
        isz3    {return $isisz3}
        dca1    {return $isdca1}
        dca2    {return $isdca2}
        jms1    {return $isjms1}
        jms2    {return $isjms2}
        jms3    {return $isjms3}
        jmp1    {return $isjmp1}
        iot1    {return $isiot1}
        grpa1   {return $isgrpa1}
        grpb1   {return $isgrpb1}
        default {return [gpio decode $gpioins ${name}q]}
    }
}

proc GetCtrl {name} {
    global gpioins
    if {$name == "_endinst"} {
        set isexec2  [gpio decode $gpioins exec2q]
        set isexec3  [gpio decode $gpioins exec3q]
        set opcode   [gpio decode $gpioins irq]
        set acqzero  [gpio decode $gpioins acqzero]
        set op0xxx   [expr {($opcode & 07000) == 00000}]
        set op1xxx   [expr {($opcode & 07000) == 01000}]
        set isand2   [expr {$isexec2 && ($op0xxx || $op1xxx && $acqzero)}]
        set istad3   [gpio decode $gpioins tad3q]
        set isisz3   [expr {$isexec3 && (($opcode & 07000) == 02000)}]
        set isdca2   [expr {$isexec2 && (($opcode & 07000) == 03000)}]
        set isiot2   [gpio decode $gpioins iot2q]
        set isgrpa1  [expr {[gpio decode $gpioins _grpa1q] ^ 1}]
        ;# puts "isand2=$isand2 istad3=$istad3 isdca2=$isdca2 isisz3=$isisz3 isiot2=$isiot2 isgrpa1=$isgrpa1"
        return [expr {! ($isand2 | $istad3 | $isdca2 | $isisz3 | $isiot2 | $isgrpa1)}]
    } else {
        return [gpio decode $gpioins $name]
    }
}

proc Error {msg} {
    global gpioins gpiouts
    puts $msg
    puts "gpioins=$gpioins"
    puts "gpiouts=$gpiouts"
    throw "ERROR" "$msg"
}

#
# use test script common with de0 testing
#
source "../modules/commontest.tcl"

#
# run test until aborted with control-C
#
option set ctrlc 0
TestInit
set i 0
while {[expr {! [option get ctrlc]}]} {
    incr i
    TestStep $i
}
