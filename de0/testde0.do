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

# This script operates as if it was the Raspberry PI
# connected to the pins of the de0.v module

# Compile in Qvartvs, top level module is de0
# Then do Tools->Run Simulation Tools->RTL Simulation
# When ModelSim finishes starting up, type 'do ../../testde0.do' at command prompt

# Simulate->Start Simulation...->work->de0->OK
vsim -i -t ns work.de0

##delete wave *
##add wave proc/uclk
##add wave clok2
##add wave reset
##add wave proc/DFF_fetch2_seq_0/_PS
##add wave proc/DFF_fetch2_seq_0/_PC
##add wave proc/DFF_fetch2_seq_0/a
##add wave proc/DFF_fetch2_seq_0/b
##add wave proc/DFF_fetch2_seq_0/c
##add wave proc/DFF_fetch2_seq_0/d
##add wave proc/DFF_fetch2_seq_0/e
##add wave proc/DFF_fetch2_seq_0/f
##add wave proc/Q_e_0_DFF_fetch1_seq
##add wave proc/Q_f_0_DFF_fetch2_seq
##add wave proc/Q_f_0_DFF_defer1_seq
##add wave proc/Q_f_0_DFF_defer2_seq
##add wave proc/Q_f_0_DFF_defer3_seq
##add wave proc/Q_f_0_DFF_exec1_seq
##add wave proc/Q_f_0_DFF_exec2_seq
##add wave proc/Q_f_0_DFF_exec3_seq
##add wave proc/Q_c_0_DLat_ireg11_seq
##add wave proc/Q_c_0_DLat_ireg10_seq
##add wave proc/Q_c_0_DLat_ireg09_seq
##add wave QENA

set sv 1         ;# 0=quiet; 1=print states
expr {srand (12345)}

force -repeat 20ns FIFTYMHZ 1 0ns, 0 10ns

proc HalfCyc {} {
    run 10000
}

proc Echo {msg} {
    echo $msg
}

# send value coming out of RASPI going into the processor
proc ForceMQ {val} {
    force DENA 0
    force QENA 1
    force DATA [string map {0 1 1 0} [Int2Bin $val 12]]
}

proc ForceMQL {bit} {
    force DENA 0
    force QENA 1
    force LINK [string map {0 1 1 0} $bit]
}

proc ForceMQOff {} {
    force DATA XXXXXXXXXXXX
    force LINK X
    force QENA 0
    force DENA 1
}

proc ForceClock {on} {
    force _CLOCK [string map {0 1 1 0} $on]
}

proc ForceIntRq {on} {
    force _INTRQ [string map {0 1 1 0} $on]
}

proc ForceIoSkp {on} {
    force _IOSKP [string map {0 1 1 0} $on]
}

proc ForceReset {on} {
    force _RESET [string map {0 1 1 0} $on]
}

# retrieve various register contents

proc examnost {name} {
    set val [examine $name]
    if {$val == "St0"} { return  0  }
    if {$val == "St1"} { return  1  }
    if {$val == "StX"} { return "X" }
    return $val
}

proc GetACBits {} {
    set achi  [examnost proc/Q_f_3_DFF_achi_acl][examnost proc/Q_f_2_DFF_achi_acl][examnost proc/Q_f_1_DFF_achi_acl][examnost proc/Q_f_0_DFF_achi_acl]
    set acmid [examnost proc/Q_f_3_DFF_acmid_acl][examnost proc/Q_f_2_DFF_acmid_acl][examnost proc/Q_f_1_DFF_acmid_acl][examnost proc/Q_f_0_DFF_acmid_acl]
    set aclo  [examnost proc/Q_f_3_DFF_aclo_acl][examnost proc/Q_f_2_DFF_aclo_acl][examnost proc/Q_f_1_DFF_aclo_acl][examnost proc/Q_f_0_DFF_aclo_acl]
    return $achi$acmid$aclo
}

proc GetALUQBits {} {
    set hibits ""
    set lobits ""
    for {set i 0} {$i < 6} {incr i} {
        set hibits "[examnost /de0/proc/Q_${i}__aluq_alu_11_6_5]$hibits"
        set lobits "[examnost /de0/proc/Q_${i}__aluq_alu_5_0_5]$lobits"
    }
    return [string map {0 1 1 0} $hibits$lobits]
}

proc GetIRQ1108Bits {} {
    set ir08 [examnost /de0/proc/Q_c_0_DLat_ireg08_seq]
    set ir09 [examnost /de0/proc/Q_c_0_DLat_ireg09_seq]
    set ir10 [examnost /de0/proc/Q_c_0_DLat_ireg10_seq]
    set ir11 [examnost /de0/proc/Q_c_0_DLat_ireg11_seq]
    return $ir11$ir10$ir09$ir08
}

proc GetLink {} {
    set _link [examnost _mdl]
    return [string map {0 1 1 0} $_link]
}

proc GetMABits {} {
    set lo  ""
    set mid ""
    set hi  ""
    for {set i 0} {$i < 4} {incr i} {
        set lo  "[examnost /de0/proc/Q_e_${i}_DFF_malo_ma]$lo"
        set mid "[examnost /de0/proc/Q_e_${i}_DFF_mamid_ma]$mid"
        set hi  "[examnost /de0/proc/Q_e_${i}_DFF_mahi_ma]$hi"
    }
    return $hi$mid$lo
}

proc GetPCBits {} {
    set bits ""
    for {set i 0} {$i < 12} {incr i} {
        set ii [format "%02d" $i]
        set bits "[examnost /de0/proc/Q_f_0_DFF_pc${ii}_pc]$bits"
    }
    return $bits
}

proc GetState {name} {
    set wire "bad-state-$name"
    switch $name {
        fetch1 { set wire "Q_f_0_DFF_fetch1_seq"  }
        fetch2 { set wire "Q_e_0_DFF_fetch2_seq"  }
        defer1 { set wire "Q_e_0_DFF_defer1_seq"  }
        defer2 { set wire "Q_e_0_DFF_defer2_seq"  }
        defer3 { set wire "Q_e_0_DFF_defer3_seq"  }
        arith1 { set wire "Q_0_arith1q_seq_0_0_2" }
        and2   { set wire "Q_0_and2q_seq_0_0_2"   }
        tad2   { set wire "Q_0_tad2q_seq_0_0_2"   }
        tad3   { set wire "Q_0_tad3q_seq_0_0_2"   }
        isz1   { set wire "Q_0_isz1q_seq_0_0_2"   }
        isz2   { set wire "Q_0_isz2q_seq_0_0_2"   }
        isz3   { set wire "Q_0_isz3q_seq_0_0_2"   }
        dca1   { set wire "Q_0_dca1q_seq_0_0_2"   }
        dca2   { set wire "Q_0_dca2q_seq_0_0_2"   }
        jms1   { set wire "Q_0_jms1q_seq_0_0_2"   }
        jms2   { set wire "Q_0_jms2q_seq_0_0_2"   }
        jms3   { set wire "Q_0_jms3q_seq_0_0_2"   }
        jmp1   { set wire "Q_0_jmp1q_seq_0_0_2"   }
        iot1   { set wire "Q_0_iot1q_seq_0_0_2"   }
        iot2   { set wire "Q_0_iot2q_seq_0_0_2"   }
        iot3   { set wire "Q_0_iot3q_seq_0_0_2"   }
        grpa1  { set wire "Q_0_grpa1q_seq_0_0_1"  }
        grpb1  { set wire "Q_0_grpb1q_seq_0_0_1"  }
        intak1 { set wire "Q_e_0_DFF_intak1_seq"  }
    }
    return [examnost "/de0/proc/$wire"]
}

proc GetCtrl {name} {
    set wire ""
    switch $name {
        _ac_aluq    { set wire "Q_0__ac_aluq_seq_0_0_3"    }
        _ac_sc      { set wire "Q_0__ac_sc_seq_0_0_3"      }
        _alu_add    { set wire "Q_0__alu_add_seq_0_0_2"    }
        _alu_and    { set wire "Q_0__alu_and_seq_0_0_2"    }
        _alua_m1    { set wire "Q_0__alua_m1_seq_0_0_3"    }
        _alua_ma    { set wire "Q_0__alua_ma_seq_0_0_4"    }
        _alub_ac    { set wire "Q_0__alub_ac_seq_0_0_3"    }
        _alub_m1    { set wire "Q_0__alub_m1_seq_0_0_3"    }
        _dfrm       { set wire "Q_0__dfrm_seq_0_0_2"       }
        _endinst    { set wire "Q_0__endinst_seq_0_0_2"    }
        _intak      { set wire "Q_f_0_DFF_intak1_seq"      }
        _jump       { set wire "Q_0__jump_seq_0_0_2"       }
        _ln_wrt     { set wire "Q_0__ln_wrt_seq_0_0_2"     }
        _ma_aluq    { set wire "Q_0__ma_aluq_seq_0_0_2"    }
        _mread      { set wire "Q_0__mread_seq_0_0_5"      }
        _mwrite     { set wire "Q_0__mwrite_seq_0_0_3"     }
        _pc_aluq    { set wire "Q_0__pc_aluq_seq_0_0_2"    }
        _pc_inc     { set wire "Q_0__pc_inc_seq_0_0_5"     }
        alua_mq0600 { set wire "Q_0_alua_mq0600_seq_0_0_1" }
        alua_mq1107 { set wire "Q_0_alua_mq1107_seq_0_0_2" }
        alua_pc0600 { set wire "Q_0_alua_pc0600_seq_0_0_1" }
        alua_pc1107 { set wire "Q_0_alua_pc1107_seq_0_0_2" }
        alub_1      { set wire "Q_0_alub_1_seq_0_0_2"      }
        inc_axb     { set wire "Q_0_inc_axb_seq_0_0_2"     }
        ioinst      { set wire "Q_0_iot1q_seq_0_0_2"       }
    }
    if {$wire == ""} {
        echo "GetCtrl: bad $name"
        pause
    }
    return [examnost "/de0/proc/$wire"]
}

proc Error {msg} {
    echo $msg
    pause
}

#
# use common test script with modules
#
source ../../../modules/commontest.tcl

TestInit
for {set i 0} {1} {incr i} {
    TestStep $i
}
