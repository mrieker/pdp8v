#
# test script used by both proc.sim and testde0.do
# generates random opcodes and operands and steps hdl code
# uses either netgen (proc.sim) or qvartvs/modelsim (testde0.do)
#

# verify various register contents

proc VerifyACBits {ss} {
    set ii [GetACBits]
    if {$ss != $ii} {
        Error "AC is $ii should be $ss"
    }
}

proc VerifyALU {sb} {
    set ii [GetALUQBits]
    set ss [Int2Bin $sb 12]
    if {$ss != $ii} {
        Error "ALU is $ii should be $ss"
    }
}

proc VerifyLink {sb} {
    set ii [GetLink]
    set ss $sb
    if {$ss != $ii} {
        Error "LINK is $ii should be $ss"
    }
}

proc VerifyMA {sb} {
    set ii [GetMABits]
    set ss [Int2Bin $sb 12]
    if {$ss != $ii} {
        Error "MA is $ii should be $ss"
    }
}

proc VerifyMD {sb} {
    set ii [GetALUQBits]
    set ss [Int2Bin $sb 12]
    if {$ss != $ii} {
        Error "MD is $ii should be $ss"
    }
}

proc VerifyPC {sb} {
    set ii [GetPCBits]
    set ss [Int2Bin $sb 12]
    if {$ss != $ii} {
        Error "PC is $ii should be $ss"
    }
}

proc VerifyIR {sb} {
    set ii [GetIRQ1108Bits]
    set ss [string range [Int2Bin $sb 12] 0 3]
    if {$ss != $ii} {
        Error "IR is $ii should be $ss"
    }
}

# print the message with current time if 'sv' is set
proc SVEcho {msg} {
    global now sv
    if {$sv} { echo "$now $msg" }
}

# convert a string of binary digits ('0' and '1') to integer
proc Bin2Int {bin} {
    set val 0
    set len [string length $bin]
    for {set i 0} {$i < $len} {incr i} {
        set dig [string index $bin $i]
        set val [expr {$val * 2 + $dig}]
    }
    return $val
}

# convert an integer to a string of binary digits
proc Int2Bin {int len} {
    set val ""
    for {set i 0} {$i < $len} {incr i} {
        set val [expr {$int & 1}]$val
        set int [expr {$int >> 1}]
    }
    return $val
}

# convert an integer to a string of octal digits
proc Int2Oct {int len} {
    set val ""
    for {set i 0} {$i < $len} {incr i} {
        set val [expr {$int & 7}]$val
        set int [expr {$int >> 3}]
    }
    return $val
}

# Verify currently in the given state
# call halfway through the state with clock just dropped

set allstates [list fetch1 fetch2 defer1 defer2 defer3 \
        arith1 and2 tad2 tad3 isz1 isz2 isz3 dca1 dca2 jmp1 \
        grpa1 grpb1 intak1 iot1 iot2 jms1 jms2 jms3]

set allctls [list \
        _mread _mwrite _intak ioinst _dfrm _jump _endinst _alu_add _alu_and inc_axb \
        _alua_ma alua_mq1107 alua_mq0600 alua_pc1107 alua_pc0600 _alua_m1 \
        alub_1 _alub_ac _alub_m1 _ac_aluq _ac_sc _ln_wrt _ma_aluq _pc_aluq _pc_inc]

proc VfyState {stsb} {
    global allstates allctls autoidx grpb_skip intrq opcode shadac

    set err 0
    foreach st $allstates {
        set ss [string equal $st $stsb]
        set ii [GetState $st]
        set len [string length $ii]
        for {set i 0} {$i < $len} {incr i} {
            set iii [string index $ii $i]
            if {$iii != $ss} {
                echo "state $st is $ii should be $ss"
                set err 1
            }
        }
    }
    if {$err} { Error "bad state" }

    switch $stsb {
        fetch1 { set ctls [list alua_pc1107 alua_pc0600 _alub_m1 _alu_and _mread] }
        fetch2 { set ctls [list ?alua_mq1107 ?alua_pc1107 alua_mq0600 _alub_m1 _alu_and _ma_aluq _pc_inc] }
        defer1 {
            set ctls [list _alua_ma _alub_m1 _alu_and _mread]
            if {$autoidx} { lappend ctls _mwrite }
        }
        defer2 { set ctls [list alua_mq1107 alua_mq0600 _alub_m1 _alu_and _ma_aluq] }
        defer3 { set ctls [list _alua_ma alub_1 _alu_add _ma_aluq] }
        arith1 {
            set ctls [list _alua_ma _alub_m1 _alu_and _mread]
            if {$opcode & 0400} { lappend ctls _dfrm }
        }
        and2   {
            set ctls [list alua_mq1107 alua_mq0600 _alub_ac _alu_and _ac_aluq _endinst]
            if {$opcode & 01000} { lappend ctls _alub_m1 }
        }
        tad2   { set ctls [list alua_mq1107 alua_mq0600 _alub_m1 _alu_and _ma_aluq] }
        tad3   { set ctls [list _alua_ma _alub_ac _alu_add _ac_aluq _ln_wrt _endinst] }
        isz1   {
            set ctls [list _alua_ma _alub_m1 _alu_and _mread _mwrite]
            if {$opcode & 0400} { lappend ctls _dfrm }
        }
        isz2   { set ctls [list alua_mq1107 alua_mq0600 _alub_m1 _alu_and _ma_aluq] }
        isz3   { set ctls [list _alua_ma alub_1 _alu_add _endinst ?_pc_inc] }
        dca1   {
            set ctls [list _alua_ma _alub_m1 _alu_and _mwrite]
            if {$opcode & 0400} { lappend ctls _dfrm }
        }
        dca2   { set ctls [list _alua_m1 _alub_ac _alu_and _ac_aluq _ac_sc _endinst] }
        jmp1   {
            set ctls [list _alua_ma _alub_m1 _alu_and _pc_aluq _jump]
            if {! $intrq} {
                lappend ctls _mread
            }
        }
        grpa1  {
            set ctls [list _ac_aluq _ln_wrt _endinst]
            if {! ($opcode & 00200)} { lappend ctls _alub_ac }    ;# ! CLA
            if {   $opcode & 00040 } { lappend ctls _alua_m1 }    ;#   CMA
            if {   $opcode & 00001 } { lappend ctls inc_axb }     ;#   IAC
        }
        grpb1  {
            set ctls [list alua_pc1107 alua_pc0600 _alu_add _pc_aluq]
            if {$grpb_skip} { lappend ctls alub_1 }
            if {$opcode & 00200} { lappend ctls _ac_aluq _ac_sc }
            if {! $intrq} { lappend ctls _mread }
        }
        intak1 { set ctls [list _alu_and _ma_aluq _mwrite _intak] }
        iot1   { set ctls [list _alua_m1 _alub_ac _alu_and ioinst] }
        iot2   { set ctls [list alua_mq1107 alua_mq0600 _alub_m1 _alu_and _ac_aluq _ln_wrt _endinst ?_pc_inc] }
        jms1   { set ctls [list _alua_ma _alub_m1 _alu_and _mwrite _jump] }
        jms2   { set ctls [list alua_pc1107 alua_pc0600 _alub_m1 _alu_and] }
        jms3   {
            set ctls [list _alua_ma alub_1 _alu_add _pc_aluq]
            if {! $intrq} {
                lappend ctls _mread
            }
        }
    }

    foreach ccc $ctls {
        set ctl $ccc
        if {[string index $ctl 0] == "?"} { set ctl [string range $ctl 1 end] }
        if {[lsearch $allctls $ctl] < 0} {
            echo "$stsb bad ctl $ctl"
            set err 1
        }
    }

    foreach ctl $allctls {
        if {[lsearch $ctls "?$ctl"] < 0} {
            set ii [string index [GetCtrl $ctl] 0]
            set ss [expr {([lsearch $ctls $ctl] >= 0) ^ ([string index $ctl 0] == "_")}]
            if {$ii != $ss} {
                echo "$stsb control $ctl is $ii should be $ss"
                set err 1
            }
        }
    }
    if {$err} { Error "bad ctls" }
}


# disassemble the given opcode
proc Disasm {opc pc} {
    switch [expr {$opc >> 9}] {
        0 { set mem "AND" }
        1 { set mem "TAD" }
        2 { set mem "ISZ" }
        3 { set mem "DCA" }
        4 { set mem "JMS" }
        5 { set mem "JMP" }
        6 { return "IOT  [Int2Oct $opc 4]" }
        7 {
            set ret ""
            if {! ($opc & 256)} {
                if {$opc & 128} { set ret "$ret CLA" }
                if {$opc &  64} { set ret "$ret CLL" }
                if {$opc &  32} { set ret "$ret CMA" }
                if {$opc &  16} { set ret "$ret CML" }
                if {$opc &   1} { set ret "$ret IAC" }
                switch [expr {($opc >> 1) & 7}] {
                    1 { set ret "$ret BSW" }
                    2 { set ret "$ret RAL" }
                    3 { set ret "$ret RTL" }
                    4 { set ret "$ret RAR" }
                    5 { set ret "$ret RTR" }
                    6 { set ret "$ret ??6" }
                    7 { set ret "$ret ??7" }
                }
            } elseif {! ($opc & 1)} {
                if {$opc &   8} { set ret "$ret !" }
                if {$opc &  64} { set ret "$ret SMA" }
                if {$opc &  32} { set ret "$ret SZA" }
                if {$opc &  16} { set ret "$ret SNL" }
                if {$ret == " !"} { set ret " SKP" }
                if {$opc & 128} { set ret "$ret CLA" }
                if {$opc &   4} { set ret "$ret OSR" }
                if {$opc &   2} { set ret "$ret HLT" }
            } else {
                set ret " GR3  [Int2Oct $opc 4]"
            }
            if {$ret == ""} { return "NOP" }
            return [string range $ret 1 end]
        }
    }
    if {$opc & 256} {
        set mem "${mem}I"
    } else {
        set mem "${mem} "
    }
    if {$opc & 128} {
        set adr [expr {$opc & 00177 | $pc & 07600}]
    } else {
        set adr [expr {$opc & 00177}]
    }
    return "$mem [Int2Oct $adr 4]"
}


# execute the given opcode, verify results
# call in middle of fetch1 with clock just dropped
# returns in middle of next fetch1 with clock just dropped
# can also return and/or be called in middle of jmp1 or jms3 if no interrupt request
# ... as jmp1 and jms3 act as fetch when no interrupt request
#  input:
#   opc = opcode
#   def = mem ref: pointer to be read from memory
#             IOT: ioskp value coming from IO device 0 or 1
#   val = AND,TAD,ISZ: value to be read from memory
#                 IOT: data value coming from IO device
#  output:
#   state updated
proc Execute {opc def val} {
    global autoidx grpb_skip hc intrq now opcode shadac shadln shadpc

    set opcode $opc

    run $hc                         ;# run the second (low) half of the cycle
    VerifyMD $shadpc                ;# just about to raise clock, should have PC on the bus

    SVEcho "starting FETCH2"
    ForceClock 1                    ;# tell the processor to transition to fetch2
    run $hc                         ;# give it time to settle into fetch2
    VerifyPC $shadpc                ;# pc should have instruction's address in it
    ForceClock 0                    ;# now we are halfway through the fetch2 cycle
    VfyState fetch2                 ;# it should be receiving the opcode from the raspi
    ForceMQ $opc                    ;# send opcode from raspi to the processor
    run $hc                         ;# run the second (low) half of the cycle
    VerifyIR $opc                   ;# IR should have top bits of opcode
    set shadpc [expr {($shadpc + 1) & 4095}]

    set maj [expr {$opc >> 9}]
    if {$maj < 6} {

        # memory reference instruction

        set addr [expr {$opc & 127}]
        if {$opc & 128} {
            set addr [expr {(($shadpc - 1) & 3968) | $addr}]
        }

        if {$opc & 256} {
            set autoidx [expr {($addr & 4088) == 8}]

            SVEcho "starting DEFER1"
            ForceClock 1
            run $hc
            ForceClock 0
            VfyState defer1
            run $hc
            VerifyMD $addr

            SVEcho "starting DEFER2"
            ForceClock 1
            run $hc
            ForceClock 0
            VfyState defer2
            ForceMQ $def
            run $hc
            VerifyALU $def                          ;# it's end of defer2, indirect address on output of ALU to be written to MA

            if {$autoidx} {
                SVEcho "starting DEFER3"
                ForceClock 1
                run $hc
                ForceClock 0
                VfyState defer3
                ForceMQOff
                run $hc
                set def [expr {($def + 1) & 4095}]
                VerifyALU $def
            }

            set addr $def
        } else {
            VerifyALU $addr                         ;# it's end of fetch2, direct address on output of ALU to be written to MA
        }

        switch $maj {
            0 {
                SVEcho "starting ARITH1/AND"
                ForceClock 1                        ;# tell the processor to transition to arith1
                run $hc                             ;# give it time to settle into arith1
                ForceClock 0                        ;# now we are halfway through the arith1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState arith1
                run $hc                             ;# run to end of arith1 cycle
                VerifyMD $addr                      ;# it should be sending the address to raspi

                SVEcho "starting AND2"
                ForceClock 1                        ;# tell the processor to transition to and2
                run $hc                             ;# give it time to settle into and2
                ForceClock 0                        ;# now we are halfway through the and2 cycle
                VfyState and2
                ForceMQ $val                        ;# send data from raspi to the processor
                run $hc                             ;# let it soak through the ALU to end of and2 cycle

                set andac [Int2Bin $val 12]
                set newac ""
                for {set i 0} {$i < 12} {incr i} {
                    set oldbit [string index $shadac $i]
                    set andbit [string index $andac $i]
                    if {$andbit == 0} {
                        set oldbit 0
                    }
                    set newac "$newac$oldbit"
                }
                set shadac $newac
            }

            1 {
                SVEcho "starting ARITH1/TAD"
                ForceClock 1                        ;# tell the processor to transition to arith1
                run $hc                             ;# give it time to settle into arith1
                ForceClock 0                        ;# now we are halfway through the arith1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState arith1
                run $hc                             ;# run to end of arith1 cycle
                VerifyMD $addr                      ;# it should be sending the address to raspi

                if {$shadac != 0} {
                    SVEcho "starting TAD2"
                    ForceClock 1                    ;# tell the processor to transition to tad2
                    run $hc                         ;# give it time to settle into tad2
                    ForceClock 0                    ;# now we are halfway through the tad2 cycle
                    VfyState tad2
                    ForceMQ $val                    ;# send data from raspi to the processor
                    run $hc                         ;# let it soak through the ALU to end of tad2 cycle

                    SVEcho "starting TAD3"
                    ForceClock 1                    ;# tell the processor to transition to tad3
                    run $hc                         ;# give it time to settle into tad3
                    ForceClock 0                    ;# now we are halfway through the tad3 cycle
                    VfyState tad3
                    ForceMQOff
                    VerifyMA $val                   ;# value should be in MA by now
                    run $hc                         ;# let addition soak through the ALU to end of tad3 cycle
                } else {
                    SVEcho "starting AND2/TAD2"
                    ForceClock 1                    ;# tell the processor to transition to and2
                    run $hc                         ;# give it time to settle into and2
                    ForceClock 0                    ;# now we are halfway through the and2 cycle
                    VfyState and2
                    ForceMQ $val                    ;# send data from raspi to the processor
                    run $hc                         ;# let it soak through the ALU to end of and2 cycle
                }

                set shadac [expr {[Bin2Int $shadac] + $val}]
                if {$shadac & 4096} {
                    set shadac [expr {$shadac & 4095}]
                    set shadln [string map {0 1 1 0} $shadln]
                }
                set shadac [Int2Bin $shadac 12]
            }

            2 {
                SVEcho "starting ISZ1"
                ForceClock 1                        ;# tell the processor to transition to isz1
                run $hc                             ;# give it time to settle into isz1
                ForceClock 0                        ;# now we are halfway through the isz1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState isz1
                run $hc                             ;# run to end of isz1 cycle
                VerifyMD $addr                      ;# it should be sending the address to raspi

                SVEcho "starting ISZ2"
                ForceClock 1                        ;# tell the processor to transition to isz2
                run $hc                             ;# give it time to settle into isz2
                ForceClock 0                        ;# now we are halfway through the isz2 cycle
                VfyState isz2
                ForceMQ $val                        ;# send data from raspi to the processor
                run $hc                             ;# let it soak through the ALU to end of isz2 cycle

                set newval [expr {($val + 1) & 4095}]

                SVEcho "starting ISZ3"
                ForceClock 1
                run $hc
                ForceClock 0
                VfyState isz3
                ForceMQOff
                run $hc
                VerifyMD $newval

                if {$newval == 0} {
                    set shadpc [expr {($shadpc + 1) & 4095}]
                }
            }

            3 {
                SVEcho "starting DCA1"
                ForceClock 1                        ;# tell the processor to transition to dca1
                run $hc                             ;# give it time to settle into dca1
                ForceClock 0                        ;# now we are halfway through the dca1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState dca1
                run $hc                             ;# run to end of dca1 cycle
                VerifyMD $addr                      ;# it should be sending the address to raspi

                SVEcho "starting DCA2"
                ForceClock 1                        ;# tell the processor to transition to dca2
                run $hc                             ;# give it time to settle into dca2
                ForceClock 0                        ;# now we are halfway through the dca2 cycle
                VfyState dca2
                run $hc                             ;# run to end of dca2 cycle
                VerifyMD [Bin2Int $shadac]          ;# it should be sending accumulator to raspi

                set shadac 000000000000             ;# it should clear AC
            }

            4 {
                SVEcho "starting JMS1"
                ForceClock 1                        ;# tell the processor to transition to jms1
                run $hc                             ;# give it time to settle into jms1
                ForceClock 0                        ;# now we are halfway through the jms1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState jms1
                run $hc                             ;# run to end of jms1 cycle
                VerifyMD $addr                      ;# it should be sending the address to raspi

                ExecJMS23Seq $addr
                if {! $intrq} return
            }

            5 {
                set shadpc $addr

                SVEcho "starting JMP1"
                ForceClock 1                        ;# tell the processor to transition to jmp1
                run $hc                             ;# give it time to settle into jmp1
                ForceClock 0                        ;# now we are halfway through the jmp1 cycle
                VerifyMA $addr                      ;# address should be clocked into MA by now
                VfyState jmp1
                if {! $intrq} {                     ;# if no interrupt request,
                    ForceMQOff                      ;# ...jmp1 acts like fetch1
                    VerifyACBits $shadac            ;# verify accumulator contents
                    VerifyLink $shadln              ;# verify link contents
                    return
                }
                run $hc                             ;# run to end of jmp1 cycle
                VerifyALU $shadpc                   ;# should be writing new pc value
            }
        }
    } else {
        switch $maj {
            6 {
                ExecIOTSeq IOT $opc $def $val
            }

            7 {
                if {! ($opc & 256)} {
                    SVEcho "starting GRPA1"
                    ForceClock 1
                    run $hc
                    ForceClock 0
                    VfyState grpa1
                    run $hc

                    if {$opc & 128} { set shadac 000000000000 }
                    if {$opc &  64} { set shadln 0 }
                    if {$opc &  32} { set shadac [string map {0 1 1 0} $shadac] }
                    if {$opc &  16} { set shadln [string map {0 1 1 0} $shadln] }
                    if {$opc &   1} {
                        set shadac [Int2Bin [expr {[Bin2Int $shadac] + 1}] 12]
                        if {$shadac == "000000000000"} {
                            set shadln [string map {0 1 1 0} $shadln]
                        }
                    }
                    switch [expr {($opc >> 1) & 7}] {
                        1 { set shadac [string range $shadac 6 11][string range $shadac 0 5] }
                        2 {
                            set newln [string index $shadac 0]
                            set shadac [string range $shadac 1 11]$shadln
                            set shadln $newln
                        }
                        3 {
                            set newln [string index $shadac 1]
                            set shadac [string range $shadac 2 11]$shadln[string index $shadac 0]
                            set shadln $newln
                        }
                        4 {
                            set newln [string index $shadac 11]
                            set shadac $shadln[string range $shadac 0 10]
                            set shadln $newln
                        }
                        5 {
                            set newln [string index $shadac 10]
                            set shadac [string index $shadac 11]$shadln[string range $shadac 0 9]
                            set shadln $newln
                        }
                        6 {
                            # invalid group1 shift 6
                            set shadac "111111111111"
                            set shadln "1"
                        }
                        7 {
                            # invalid group1 shift 7
                            set shadac "111111111111"
                            set shadln "1"
                        }
                    }
                } elseif {! ($opc & 7)} {
                    SVEcho "starting GRPB1"
                    ForceClock 1
                    run $hc
                    ForceClock 0

                    set grpb_skip 0
                    if {$opc &  64} { set grpb_skip [expr {$grpb_skip | [string index $shadac 0]}] }  ;# SMA
                    if {$opc &  32} { set grpb_skip [expr {$grpb_skip | ([Bin2Int $shadac] == 0)}] }  ;# SZA
                    if {$opc &  16} { set grpb_skip [expr {$grpb_skip | $shadln}] }                   ;# SNL
                    if {$opc &   8} { set grpb_skip [expr {1 - $grpb_skip}] }                         ;# reverse

                    VfyState grpb1

                    if {$opc & 128} { set shadac 000000000000 }                             ;# CLA

                    if {$grpb_skip} {
                        set shadpc [expr {($shadpc + 1) & 4095}]
                    }
                    VerifyALU $shadpc

                    if {! $intrq} {                     ;# if no interrupt request,
                        return                          ;# ...grpb1 acts like fetch1
                    }
                    run $hc
                } else {
                    ExecIOTSeq IOT $opc $def $val       ;# group 2 with OSR or HLT and group 3 treated like io instruction
                }
            }
        }
    }

    if {$intrq} {
        SVEcho "starting INTAK1"
        ForceClock 1
        run $hc
        ForceClock 0
        VfyState intak1
        ForceIntRq 0
        run $hc

        set intrq 0                     ;# intack turns intrq off

        ExecJMS23Seq 0                  ;# finish JMS 0
        return
    }

    SVEcho "starting FETCH1"
    ForceClock 1                        ;# tell the processor to transition to fetch1
    run $hc                             ;# give it time to settle into fetch1
    ForceClock 0                        ;# now we are halfway through the fetch1 cycle
    VfyState fetch1
    ForceMQOff
    VerifyACBits $shadac                ;# verify accumulator contents
    VerifyLink $shadln                  ;# verify link contents
    VerifyPC $shadpc
}

# execute an IOT or Group 3 instruction via raspi
# called just about to go clock up for IOT1 state
# returns at end of clock low for IOT2 state
proc ExecIOTSeq {mne opc def val} {
    global hc shadac shadln shadpc

    SVEcho "starting ${mne}1"           ;# processor sends 12-bit accumulator to raspi
    ForceClock 1
    run $hc
    ForceClock 0
    VfyState iot1
    run $hc
    VerifyMD [Bin2Int $shadac]

    SVEcho "starting ${mne}2"           ;# processor receives updated accumulator, link and ioskip from raspi
    set shadac [Int2Bin $val 12]
    set shadln [expr {($def >> 1) & 1}]
    ForceClock 1
    run $hc
    ForceClock 0
    VfyState iot2
    ForceMQ $val
    ForceMQL $shadln
    ForceIoSkp [expr {$def & 1}]
    run $hc

    if {$def & 1} {
        set shadpc [expr {($shadpc + 1) & 4095}]
    }
}


# execute last 2/3rds of a JMS instruction
# called just about to go clock up for JMS2 state
# if intrq, returns at end of clock low for JMS3 state
# if !intrq, returns in middle of JMS3 with mread going, just like FETCH1
proc ExecJMS23Seq {addr} {
    global hc intrq shadpc

    SVEcho "starting JMS2"
    ForceClock 1                        ;# tell the processor to transition to jms2
    run $hc                             ;# give it time to settle into jms2
    ForceClock 0                        ;# now we are halfway through the jms2 cycle
    VfyState jms2
    run $hc                             ;# run to end of jms2 cycle
    VerifyMD $shadpc                    ;# it should be sending the incremented pc to raspi

    set shadpc [expr {($addr + 1) & 4095}]

    SVEcho "starting JMS3"
    ForceClock 1                        ;# tell the processor to transition to jms3
    run $hc                             ;# give it time to settle into jms3
    ForceClock 0                        ;# now we are halfway through the jms3 cycle
    VfyState jms3

    if {! $intrq} {                     ;# if no interrupt request,
        ForceMQOff                      ;# ...jsr3 acts like fetch1
        return
    }

    run $hc                             ;# run to end of jms3 cycle
    VerifyALU $shadpc                   ;# should be calculating incremented pc
}


## SCRIPT STARTS HERE ##

SVEcho "starting FETCH1"
ForceClock 0
ForceIntRq 0
ForceReset 1
run $hc
ForceReset 0

set intrq 0
set shadpc 0
set shadac "xxxxxxxxxxxx"
set shadln "x"

Execute [Bin2Int 111011000000] 0 0          ;# CLA CLL

for {set i 0} {$i < 4096*4096} {incr i} {
    set opc [expr {int (rand () * 4096)}]
    set def [expr {int (rand () * 4096)}]
    set val [expr {int (rand () * 4096)}]

    if {[GetState fetch1]} {
        set intrq [expr {int (rand () * 8) == 0}]
        ForceIntRq $intrq                   ;# request interrupt at end of instruction
    }

    echo "$i [expr {$now - $hc}]  IRQ=$intrq  L=$shadln AC=$shadac PC=[Int2Bin $shadpc 12] OPC=[Int2Bin $opc 12]  DEF=[Int2Bin $def 12]  VAL=[Int2Bin $val 12]  [Disasm $opc $shadpc]"

    Execute $opc $def $val
}

Execute [Bin2Int 111000000000] 0 0          ;# NOP

echo "SUCCESS"
