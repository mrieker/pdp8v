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

##  script executed when tclboardtest starts up
##  can be overridden with envar tcltestini:
##  export tcltestini=somethingelse.tcl

# dump board and compare with shadow
proc dumpboard {board} {
    halfcycle                   ;# make sure state up-to-date
    array set v [exam $board]   ;# read tube state
    array set s [examsh $board] ;# read shadow state
    foreach key [array names v] {
        set x($key) [expr {($v($key) != $s($key)) ? "<<" : "  "}]
    }

    switch $board {
        acl {
            puts ""
            puts "inputs to acl board:"
            puts "     _ac_aluq = $v(_ac_aluq)$x(_ac_aluq)           _maq = [format %04o $v(_maq)]$x(_maq)"
            puts "       _ac_sc = $v(_ac_sc)$x(_ac_sc)            maq = [format %04o $v(maq)]$x(maq)"
            puts "        _aluq = [format %04o $v(_aluq)]$x(_aluq)         mql = $v(mql)$x(mql)"
            puts "        clok2 = $v(clok2)$x(clok2)       _newlink = $v(_newlink)$x(_newlink)"
            puts "      _grpa1q = $v(_grpa1q)$x(_grpa1q)          reset = $v(reset)$x(reset)"
            puts "        iot2q = $v(iot2q)$x(iot2q)          tad3q = $v(tad3q)$x(tad3q)"
            puts "      _ln_wrt = $v(_ln_wrt)$x(_ln_wrt)"
            puts ""
            puts "outputs from acl board:"
            puts "          acq = [format %04o $v(acq)]$x(acq)"
            puts "      acqzero = $v(acqzero)$x(acqzero)"
            puts "    grpb_skip = $v(grpb_skip)$x(grpb_skip)"
            puts "         _lnq = $v(_lnq)$x(_lnq)"
            puts "          lnq = $v(lnq)$x(lnq)"
            puts ""
        }
        alu {
            puts ""
            puts "inputs to alu board:"
            puts "          acq = [format %04o $v(acq)]$x(acq)    _alub_ac = $v(_alub_ac)$x(_alub_ac)"
            puts "     _alu_add = $v(_alu_add)$x(_alu_add)       _alub_m1 = $v(_alub_m1)$x(_alub_m1)"
            puts "     _alu_and = $v(_alu_and)$x(_alu_and)        _grpa1q = $v(_grpa1q)$x(_grpa1q)"
            puts "     _alua_m1 = $v(_alua_m1)$x(_alua_m1)        inc_axb = $v(inc_axb)$x(inc_axb)"
            puts "     _alua_ma = $v(_alua_ma)$x(_alua_ma)           _lnq = $v(_lnq)$x(_lnq)"
            puts "  alua_mq0600 = $v(alua_mq0600)$x(alua_mq0600)            lnq = $v(lnq)$x(lnq)"
            puts "  alua_mq1107 = $v(alua_mq1107)$x(alua_mq1107)           _maq = [format %04o $v(_maq)]$x(_maq)"
            puts "  alua_pc0600 = $v(alua_pc0600)$x(alua_pc0600)            maq = [format %04o $v(maq)]$x(maq)"
            puts "  alua_pc1107 = $v(alua_pc1107)$x(alua_pc1107)             mq = [format %04o $v(mq)]$x(mq)"
            puts "       alub_1 = $v(alub_1)$x(alub_1)            pcq = [format %04o $v(pcq)]$x(pcq)"
            puts ""
            puts "outputs from alu board:"
            puts "     _alucout = $v(_alucout)$x(_alucout)"
            puts "        _aluq = [format %04o $v(_aluq)]$x(_aluq)"
            puts "     _newlink = $v(_newlink)$x(_newlink)"
            puts ""
        }
        ma {
            puts ""
            puts "inputs to ma board:"
            puts "        _aluq = [format %04o $v(_aluq)]$x(_aluq)"
            puts "        clok2 = $v(clok2)$x(clok2)"
            puts "     _ma_aluq = $v(_ma_aluq)$x(_ma_aluq)"
            puts "        reset = $v(reset)$x(reset)"
            puts ""
            puts "outputs from ma board:"
            puts "         _maq = [format %04o $v(_maq)]$x(_maq)"
            puts "          maq = [format %04o $v(maq)]$x(maq)"
            puts ""
        }
        pc {
            puts ""
            puts "inputs to pc board:"
            puts "        _aluq = [format %04o $v(_aluq)]$x(_aluq)"
            puts "        clok2 = $v(clok2)$x(clok2)"
            puts "     _pc_aluq = $v(_pc_aluq)$x(_pc_aluq)"
            puts "      _pc_inc = $v(_pc_inc)$x(_pc_inc)"
            puts "        reset = $v(reset)$x(reset)"
            puts ""
            puts "outputs from pc board:"
            puts "          pcq = [format %04o $v(pcq)]$x(pcq)"
            puts ""
        }
        rpi {
            puts ""
            puts "inputs to rpi board:"
            puts "   from GPIO:          from bus:"
            puts "        CLOCK = $v(CLOCK)$x(CLOCK)        _aluq = [format %04o $v(_aluq)]$x(_aluq)"
            puts "         DENA = $v(DENA)$x(DENA)        _dfrm = $v(_dfrm)$x(_dfrm)"
            puts "          IOS = $v(IOS)$x(IOS)        _jump = $v(_jump)$x(_jump)"
            puts "          IRQ = $v(IRQ)$x(IRQ)       _intak = $v(_intak)$x(_intak)"
            puts "         QENA = $v(QENA)$x(QENA)       ioinst = $v(ioinst)$x(ioinst)"
            puts "        RESET = $v(RESET)$x(RESET)         _lnq = $v(_lnq)$x(_lnq)"
            if {$v(QENA)} {
                puts "         DATA = [format %04o $v(DATA)]$x(DATA)    _mread = $v(_mread)$x(_mread)"
                puts "         LINK = $v(LINK)$x(LINK)      _mwrite = $v(_mwrite)$x(_mwrite)"
            } else {
                puts "                          _mread = $v(_mread)$x(_mread)"
                puts "                         _mwrite = $v(_mwrite)$x(_mwrite)"
            }
            puts ""
            puts "outputs from rpi board:"
            puts "     to GPIO:            to bus:"
            puts "         DFRM = $v(DFRM)$x(DFRM)        clok2 = $v(clok2)$x(clok2)"
            puts "          IAK = $v(IAK)$x(IAK)        intrq = $v(intrq)$x(intrq)"
            puts "         IOIN = $v(IOIN)$x(IOIN)        ioskp = $v(ioskp)$x(ioskp)"
            puts "         JUMP = $v(JUMP)$x(JUMP)          mql = $v(mql)$x(mql)"
            puts "         READ = $v(READ)$x(READ)           mq = [format %04o $v(mq)]$x(mq)"
            puts "        WRITE = $v(WRITE)$x(WRITE)        reset = $v(reset)$x(reset)"
            if {$v(DENA)} {
                puts "         DATA = [format %04o $v(DATA)]$x(DATA)"
                puts "         LINK = $v(LINK)$x(LINK)"
            }
            puts ""
        }
        seq {
            puts ""
            puts "inputs to seq board:"
            puts "      acqzero = $v(acqzero)$x(acqzero)      grpb_skip = $v(grpb_skip)$x(grpb_skip)           _maq = [format %04o $v(_maq)]$x(_maq)"
            puts "     _alucout = $v(_alucout)$x(_alucout)          intrq = $v(intrq)$x(intrq)            maq = [format %04o $v(maq)]$x(maq)"
            puts "        clok2 = $v(clok2)$x(clok2)          ioskp = $v(ioskp)$x(ioskp)             mq = [format %04o $v(mq)]$x(mq)"
            puts "        reset = $v(reset)$x(reset)"
            puts ""
            puts "outputs from seq board:"
            puts "     _ac_aluq = $v(_ac_aluq)$x(_ac_aluq)         alub_1 = $v(alub_1)$x(alub_1)          iot2q = $v(iot2q)$x(iot2q)        fetch1q = $v(fetch1q)$x(fetch1q)"
            puts "       _ac_sc = $v(_ac_sc)$x(_ac_sc)       _alub_ac = $v(_alub_ac)$x(_alub_ac)        _ln_wrt = $v(_ln_wrt)$x(_ln_wrt)        fetch2q = $v(fetch2q)$x(fetch2q)"
            puts "     _alu_add = $v(_alu_add)$x(_alu_add)       _alub_m1 = $v(_alub_m1)$x(_alub_m1)       _ma_aluq = $v(_ma_aluq)$x(_ma_aluq)        defer1q = $v(defer1q)$x(defer1q)"
            puts "     _alu_and = $v(_alu_and)$x(_alu_and)        _grpa1q = $v(_grpa1q)$x(_grpa1q)         _mread = $v(_mread)$x(_mread)        defer2q = $v(defer2q)$x(defer2q)"
            puts "     _alua_m1 = $v(_alua_m1)$x(_alua_m1)          _dfrm = $v(_dfrm)$x(_dfrm)        _mwrite = $v(_mwrite)$x(_mwrite)        defer3q = $v(defer3q)$x(defer3q)"
            puts "     _alua_ma = $v(_alua_ma)$x(_alua_ma)          _jump = $v(_jump)$x(_jump)       _pc_aluq = $v(_pc_aluq)$x(_pc_aluq)         exec1q = $v(exec1q)$x(exec1q)"
            puts "  alua_mq0600 = $v(alua_mq0600)$x(alua_mq0600)        inc_axb = $v(inc_axb)$x(inc_axb)        _pc_inc = $v(_pc_inc)$x(_pc_inc)         exec2q = $v(exec2q)$x(exec2q)"
            puts "  alua_mq1107 = $v(alua_mq1107)$x(alua_mq1107)         _intak = $v(_intak)$x(_intak)          tad3q = $v(tad3q)$x(tad3q)         exec3q = $v(exec3q)$x(exec3q)"
            puts "  alua_pc0600 = $v(alua_pc0600)$x(alua_pc0600)        intak1q = $v(intak1q)$x(intak1q)            irq = [format %04o $v(irq)]$x(irq)"
            puts "  alua_pc1107 = $v(alua_pc1107)$x(alua_pc1107)         ioinst = $v(ioinst)$x(ioinst)"
            puts ""
        }
    }
}

# return source of alu A operand
proc examalua {} {
    set x [exam _alua_m1 _alua_ma alua_mq1107 alua_pc1107 alua_mq0600 alua_pc0600]
    set i 0
    set s ""
    foreach n [list M1 MA] {
        if {! [lindex $x $i]} {set s "$s$n"}
        incr i
    }
    foreach n [list MQ1107 PC1107 MQ0600 PC0600] {
        if {[lindex $x $i]} {set s "$s$n"}
        incr i
    }
    if {$s == ""} {set $s "0"}
    return $s
}

# return source of alu B operand
proc examalub {} {
    set x [exam alub_1 _alub_ac _alub_m1]
    set s ""
    if {[lindex $x 0]} {set s "1"}
    set i 1
    foreach n [list AC M1] {
        if {! [lindex $x $i]} {set s "$s$n"}
        incr i
    }
    if {$s == ""} {set $s "0"}
    return $s
}

# examine the state - returns state in uppercase
proc examstate {} {
    set x [exam fetch1q fetch2q defer1q defer2q defer3q exec1q exec2q exec3q intak1q]
    set i 0
    set s ""
    foreach n [list FETCH1 FETCH2 DEFER1 DEFER2 DEFER3 EXEC1 EXEC2 EXEC3 INTAK1] {
        if {[lindex $x $i]} {set s "$s$n"}
        incr i
    }
    return $s
}

# see if a particular board is connected
proc isboard {board} {
    set bds [listmods]
    set idx [lsearch -exact $bds $board]
    return [expr {$idx >= 0}]
}

# print help for commands defined herein
proc helpini {} {
    puts ""
    puts "  dumpboard <name> - print board inputs and outputs"
    puts "  examalua         - examine source of alu A operand"
    puts "  examalub         - examine source of alu B operand"
    puts "  examstate        - examine state"
    puts "  isboard <name>   - see if given board connected"
    puts ""
}

# message displayed before first interactive prompt
return "  also, 'helpini' will print help for tcltestini.tcl commands"
