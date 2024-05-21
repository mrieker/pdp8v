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

# see if a particular board is connected
proc isboard {board} {
    set bds [listmods]
    set idx [lsearch -exact $bds $board]
    return [expr {$idx >= 0}]
}

proc dumpseq {} {
    halfcycle                   ;# make sure state up-to-date
    array set v [exam seq]      ;# read tube state
    array set s [examsh seq]    ;# read shadow state
    foreach key [array names v] {
        set x($key) [expr {($v($key) != $s($key)) ? "<<" : "  "}]
    }
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

# print help for commands defined herein
proc helpini {} {
    puts ""
    puts "  dumpseq - print seq board inputs and outputs"
    puts "  isboard <name> - see if given board connected"
    puts ""
}

# message displayed before first interactive prompt
return "  also, 'helpini' will print help for tcltestini.tcl commands"
