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

##  script executed before script passed on command line to raspictl
##  can be overridden with envar scriptini:
##  export scriptini=somethingelse.tcl

# disassemble block of memory
# - disas               ;# disassemble in vacinity of current 15-bit pc
# - disass addr         ;# disassemble in vacinity of given 15-bit address
# - disass start stop   ;# disassemble the given range
proc disas {{start ""} {stop ""}} {
    # get current 15-bit pc
    set iff [iodev memext get if]
    set cpupc [expr {($iff << 12) | [cpu get pc]}]
    # default the first arg to current 15-bit pc
    if {$start == ""} {
        set start $cpupc
    }
    # if just one arg, make the range start-8..start+8
    if {$stop == ""} {
        set stop  [expr {(($start & 007777) > 07767) ? ($start | 000007) : ($start + 010)}]
        set start [expr {(($start & 007777) < 00010) ? ($start & 077770) : ($start - 010)}]
    }
    # loop through the range, inclusive
    for {set pc $start} {$pc <= $stop} {incr pc} {
        # read opcode from memory and get basic disassembly
        set op [readmem $pc]
        set as [disasop $op $pc]
        set line [format "%05o  %04o  %s" $pc $op $as]
        # see if it is the instruction about to be executed
        if {$pc == $cpupc} {
            # chop the disassembly at 50 characters
            if {[string length $line] > 50} {set line [string range $line 0 49]}
            # append some basic cpu state onto the end of the line
            set ie [expr {[iodev memext get ie] & ! [iodev memext get iduj]}]
            set line [format "%-50s  < L.AC=%o.%04o IE=%o" $line [cpu get link] [cpu get ac] $ie]
            # if memory ref instruction, append memory location to end of line
            if {$op < 06000} {
                set ma [expr {($iff << 12) | (($op & 00200) ? ($pc & 07600) : 0) | ($op & 0177)}]
                set line [format "%s  %05o" $line $ma]                  ;# direct operand address
                if {$op & 00400} {                                      ;# check for indirect
                    if {$op > 03777} {set iff [iodev memext get ifaj]}  ;# ifield after jump
                    set dff  [iodev memext get df]
                    set mff  [expr {($op < 04000) ? $dff : $iff}]       ;# get operand field
                    set ptr  [expr {($mff << 12) | [readmem $ma]}]      ;# get 15-bit pointer
                    set line [format "%s/%05o" $line $ptr]
                    if {($ma & 07770) == 00010} {                       ;# check for auto-inc
                        set line "$line+1"
                        set ptr [expr {($ptr & 070000) | (($ptr + 1) & 007777)}]
                    }
                    set ma $ptr
                }
                if {$op < 04000} {
                    set line [format "%s>%04o" $line [readmem $ma]]
                }
            }
        }
        puts $line
    }
}

# get environment variable, return default value if not defined
proc getenv {varname {defvalu ""}} {
    return [expr {[info exists ::env($varname)] ? $::env($varname) : $defvalu}]
}

# step one instruction then disassemble surrounding instructions
proc stepdis {} {
    stepins
    disas
}

# print help for commands defined herein
proc helpini {} {
    puts ""
    puts "  disas <start> <stop>       - disassemble instructions in given range"
    puts "  getenv <varname> <defvalu> - get environment variable"
    puts "  stepdis                    - step one instruction then disassemble"
    puts ""
}

# message displayed before first interactive prompt
return "  also, 'helpini' will print help for scriptini.tcl commands"
