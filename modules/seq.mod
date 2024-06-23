//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// sequencer

module seqcirc (
    out _ac_aluq,       // load AC from ALU at end of cycle
    out _ac_sc,         // load AC with zeroes at end of cycle
    in acqzero,         // AC contains all zeroes
    out _alu_add,       // ALU should ADD A + B
    out _alu_and,       // ALU should AND A & B
    out _alua_m1,       // ALUA = 7777
    out _alua_ma,       // ALUA = MA
    out alua_mq0600,    // ALUA<06:00> = raspi data<06:00>
    out alua_mq1107,    // ALUA<11:07> = raspi data<11:07>
    out alua_pc0600,    // ALUA<06:00> = PC<06:00>
    out alua_pc1107,    // ALUA<11:07> = PC<11:07>
    out alub_1,         // ALUB = 0001
    out _alub_ac,       // ALUB = AC
    out _alub_m1,       // ALUB = 7777
    in _alucout,        // ALU carry output
    in clok2,           // clock signal from raspi
    out _grpa1q,        // in EXEC1 for group 1 instruction
    in grpb_skip,       // assuming MA contains group 2 skip codes, that skip is true
    out _dfrm,          // memory cycle is in DFRAME else it is in IFRAME
    out _jump,          // in EXEC1 for JMP or JMS instruction
    out inc_axb,        // ALU should do A ^ B + 1
    out _intak,         // interrupt request being acknowledged
    in intrq,           // raspi is requesting an interrupt
    out intak1q,        // front panel INTAK LED
    out ioinst,         // in EXEC1 for i/o instruction (incl group 3 and group 2 HLT,OSR)
    in ioskp,           // raspi is saying the i/o instr is saying to do a skip
    out iot2q,          // in EXEC2 of an i/o instruction
    out _ln_wrt,        // write LINK register at end of cycle
    out _ma_aluq,       // write MA from ALU output at end of cycle
    in _maq[11:00],     // MA register contents
    in maq[11:00],
    in mq[11:00],       // data coming from raspi
    out _mread,         // sending address to raspi via ALU to start reading memory
    out _mwrite,        // sending address to raspi via ALU to start writing memory
    out _pc_aluq,       // load PC with ALU outpit at end of cycle
    out _pc_inc,        // load PC with incremented PC at end of cycle
    in reset,           // reset signal from raspi (forces to FETCH1 state)
    out tad3q,          // in EXEC3 of a TAD instruction
    out fetch1q,        // front panel LEDs...
    out fetch2q,
    out defer1q,
    out defer2q,
    out defer3q,
    out exec1q,
    out exec2q,
    out exec3q,
    out irq[11:09])
{
    wire _autoidx, autoidx, _meminst, meminst, _endinst, _intrq;
    wire _intak1d, fetch1d, _fetch2d, _defer1d, _defer2d, _defer3d, _exec1d, _exec2d, exec3d;
    wire _ir_arth, _ir_and, _ir_tad, _ir_isz, _ir_dca, _ir_jms, _ir_jmp, _ir_iot, _ir_opr;
    wire _clok1, clok0a, clok0b, clok0c;
    wire _reseta, _resetb, _resetc;
    wire _irq[11:09], irq_8, irq_11;

    wire _fetch1q, _fetch2q, fetch2qa, _defer1q, _exec1q, _exec2q, _exec3q, _intak1q;
    wire arith1q, arith2q, and2q, tad2q, isz1q, isz2q, isz3q, dca1q, dca2q, jms1q, jms2q, jms3q, jmp1q;
    wire grpb1q, iot1q, opr1q, grpa1q;

    // buffer the clock and reset signals for fanout
    _clok1 = ~ clok2;
    clok0a = ~ _clok1;
    clok0b = ~ _clok1;
    clok0c = ~ _clok1;

    _reseta = ~ reset;
    _resetb = ~ reset;
    _resetc = ~ reset;

    // the MA register holds the address of an auto-increment location (0010..0017)
    // valid from beginning of defer1 cuz it gets loaded from opcode coming from raspi at end of fetch2
    //   to end of defer2 cuz it gets loaded with pointer coming from raspi at end of defer2
    _autoidx = ~ (_maq[11] & _maq[10] & _maq[09] & _maq[08] & _maq[07] & _maq[06] & _maq[05] & _maq[04] & maq[03]);
    autoidx  = ~ _autoidx;

    // the raspi is sending us a memory reference instruction (incl JMP, JMS)
    // valid at end of fetch2 only cuz that's when the opcode is on the mq bus coming from raspi
    meminst  = ~ (mq[11] & mq[10]);
    _meminst = ~ meminst;

    // determine next state based on current state and inputs

    // - determine if this is the last cycle of the instruction
    //   AND has 2 cycles
    //   TAD has 3 cycles
    //   DCA has 2 cycles
    //   ISZ has 3 cycles
    //   IOT has 2 cycles
    //   Group 1 has 1 cycle
    //  JMP,JMS,Group 2 overlap FETCH1 with their last cycle so don't count here
    _endinst = ~ (and2q | tad3q | dca2q | isz3q | iot2q | grpa1q);

    // buffer interrupt request for fanout
    _intrq   = ~ intrq;

    // - INTAK1: interrupt acknowledge
    //   follows the last cycle of an instruction where there is an interrupt request
    //     end of non-JMP,JMS,Group 2
    //     JMP1
    //     JMS3
    //     Group 2
    _intak1d = ~ (~ _endinst & intrq | jmp1q & intrq | jms3q & intrq | grpb1q & intrq);

    // - FETCH1: send PC to raspi to fetch instruction
    //   follows the last cycle of an instruction when there is no interrupt request
    //     end of non-JMP,JMS
    //   does not follow JMP,JMS,Group 2 because they overlap fetching with their last cycle
    fetch1d  = ~ (_endinst | intrq);

    // - FETCH2: receive opcode from raspi into instruction register and memory address register
    //   follows FETCH1, or JMP1 with no int request, or JMS3 with no int request, or Group 2 with no int request
    _fetch2d = ~ (fetch1q  | jmp1q & _intrq | jms3q & _intrq | grpb1q & _intrq);

    // - DEFER1: send pointer address to raspi to read pointer
    //   follows FETCH2 for memory reference instructions with indirect bit set
    _defer1d = ~ (fetch2qa & meminst & mq[08]);

    // - DEFER2: read pointer value from raspi into MA
    //   follows DEFER1
    _defer2d = _defer1q;

    // - DEFER3: add 1 to MA via the ALU and send to raspi and write to MA at end of cycle
    //   follows DEFER2 if an autoincrement location was used
    _defer3d = ~ (defer2q & autoidx);

    // decode the opcode based on contents of instruction register
    _ir_arth = ~ (_irq[11] & _irq[10]);
    _ir_and  = ~ (_irq[11] & _irq[10] & _ir_tad);
    _ir_tad  = ~ (_irq[11] & _irq[10] &  irq[09] & ~ acqzero);
    _ir_isz  = ~ (_irq[11] &  irq[10] & _irq[09]);
    _ir_dca  = ~ (_irq[11] &  irq[10] &  irq[09]);
    _ir_jms  = ~ ( irq_11  & _irq[10] & _irq[09]);
    _ir_jmp  = ~ ( irq_11  & _irq[10] &  irq[09]);
    _ir_iot  = ~ ( irq_11  &  irq[10] & _irq[09] |
                   irq_11  &  irq[10] & maq[08] & maq[02] |     // includes group 2 with OSR
                   irq_11  &  irq[10] & maq[08] & maq[01] |     // includes group 2 with HLT
                   irq_11  &  irq[10] & maq[08] & maq[00]);     // includes group 3
    _ir_opr  = ~ ( irq_11  &  irq[10] &  irq[09] & _ir_iot);    // excludes group 2 with OSR and/or HLT and group 3

    // - EXEC1: first instruction execution cycle
    //   follows
    //     FETCH2 for direct address memory reference instructions
    //     DEFER2 for non-autoindex indirect mem ref instructions
    //     DEFER3 for autoindex indirect memory ref instructions
    //     FETCH2 for all non-memory reference instructions
    _exec1d  = ~ (fetch2qa & ~ mq[08] |                         // memory instuction direct addressing
                  defer2q & _autoidx |                          // deferred non-auto-index
                  defer3q |                                     // deferred auto-index
                  fetch2qa & _meminst);                         // non-memory instruction

    // - EXEC2: second instruction execution cycle
    //   follows
    //    EXEC1 for AND,TAD,ISZ,DCA,JMS,IOT
    //    INTAK1  (INTAK1 is like JMS1)
    _exec2d  = ~ (exec1q & ~ (irq[11] & _ir_jms & _ir_iot) |    // arith2, isz2, dca2, jms2, iot2
                  intak1q);                                     // also jms2 follows intak1q

    // - EXEC3: third instruction execution cycle
    //   follows
    //     EXEC2 for TAD,ISZ,JMS
     exec3d  = ~ (_exec2q | _ir_tad & _ir_isz & _ir_jms);       // tad3, isz3, jms3

    // split exec1,2,3 states into instruction states

    arith1q = ~ (_exec1q | _ir_arth);           // AND and TAD
    arith2q = ~ (_exec2q | _ir_arth);           // AND and TAD
    and2q   = ~ (_exec2q | _ir_and);
    tad2q   = ~ (_exec2q | _ir_tad);
    tad3q   = ~ (_exec3q | _ir_tad);
    isz1q   = ~ (_exec1q | _ir_isz);
    isz2q   = ~ (_exec2q | _ir_isz);
    isz3q   = ~ (_exec3q | _ir_isz);
    dca1q   = ~ (_exec1q | _ir_dca);
    dca2q   = ~ (_exec2q | _ir_dca);
    jms1q   = ~ (_exec1q | _ir_jms);
    jms2q   = ~ (_exec2q | _ir_jms);
    jms3q   = ~ (_exec3q | _ir_jms);
    jmp1q   = ~ (_exec1q | _ir_jmp);
    iot1q   = ~ (_exec1q | _ir_iot);            // includes group2 with OSR or HLT and group 3
    iot2q   = ~ (_exec2q | _ir_iot);
    opr1q   = ~ (_exec1q | _ir_opr);            // excludes group2 with OSR or HLT and group 3
    _grpa1q = ~ (opr1q & _maq[8]);              // group 1
    grpa1q  = ~ _grpa1q;
    grpb1q  =    opr1q &  maq[8];               // group 2 without OSR or HLT (group 3 already excluded from opr1q)

    // generate control signals from state

    // MREAD: memory address is being sent to raspi via ALU, raspi responds with contents next cycle
    //   set when FETCH1 - to send PC for fetching instruction
    //            DEFER1 - to send pointer address for reading pointer value
    //            ARITH1 - to send operand address for reading operand value
    //            ISZ1 - to send operand address for reading and writing operand value
    //            JMP1 with no int req - to send new PC for fetching instruction
    //            JMS3 with no int req - to send new PC for fetching instruction
    //            Group 2 with no int req - to send PC (possibly incremented by skip) for fetching instruction
    _mread  = ~ (fetch1q | defer1q | arith1q | isz1q | jmp1q & _intrq | jms3q & _intrq | grpb1q & _intrq);

    // MWRITE: memory address is being sent to raspi via ALU, data will be sent to raspi via ALU next cycle
    //   set when DEFER1 and autoindex - to write incremented pointer back to memory
    //            JMS1 - to write program counter to memory
    //            DCA1 - to write ALU contents to memory
    //            ISZ1 - to write incremented value to memory
    //            INTAK1 - to write program counter to memory
    _mwrite = ~ (defer1q & autoidx | jms1q | dca1q | isz1q | intak1q);

    // DFRM: tell raspi to use data frame instead of instruction frame for this memory access
    //   set when doing AND,TAD,ISZ,DCA with indirect address
    _dfrm   = ~ (exec1q & _irq[11] & irq_8);

    // JUMP: tell raspi that memory references from here on use new instruction frame
    //   set in first execution cycle of JMP,JMP
    _jump   = ~ (exec1q & irq_11 & _irq[10]);

    // INTAK: interrupt is being acknowledged, raspi should stop requesting interrupt
    //   set when in interrupt acknowledge state
    _intak  = _intak1q;

    // IOINST: tell raspi to do an IO instruction
    //   set when doing first cycle of an IO instruction (including Group 2 with HLT,OSR and Group 3)
    ioinst  = iot1q;

    // ALU_ADD: ALU should perform an addition
    //   set when in DEFER3 to increment the pointer
    //               EXEC3 for TAD to perform addition
    //                         ISZ to increment counter
    //                         JMS to increment MA to point to first instruction of subroutine
    //               Group 2 to increment the PC if skipping
    //   note that EXEC3 is used only for TAD,ISZ,JMS so it is sufficient to decode EXEC3
    _alu_add = ~ (defer3q | exec3q | grpb1q);

    // ALU_AND: ALU should perform an anding
    //   set when not doing an ADD and not doing Group 1
    _alu_and = ~ (_alu_add & _grpa1q);

    // INC_AXB: ALU should increment the A ^ B value
    //   set when doing a Group 1 with the IAC bit
    inc_axb  = ~ (_grpa1q | _maq[0]);

    // AC_SC: synchronously clear accumulator at end of cycle
    //   set when in DCA2 to clear AC after sending its contents to raspi
    //               Group 2 with CLA bit set
    _ac_sc   = ~ (dca2q | grpb1q & maq[7]);

    // AC_ALUQ: load accumulator with ALU output at end of cycle
    //   set when in AND2 - result of anding is at ALU output
    //               TAD3 - result of addition is at ALU output
    //               DCA2 - needed to clock the zero set up by AC_SC
    //            Group 1 - result of CLA,CMA,IAC,shifts gets clocked into AC
    //            Group 2 - needed to clock the zero set up by AC_SC
    //               IOT2 - clock the new AC value generated by the I/O instruction
    _ac_aluq = ~ (and2q | tad3q | dca2q | grpa1q | grpb1q & maq[7] | iot2q);

    // LN_WRT: write link from various sources at end of cycle (must be subset of AC_ALUQ)
    //   set when in TAD3 - result of addition
    //            Group 1 - update link from CLL, CML, IAC, rotates
    //               IOT2 - clock the new link value generated by I/O instruction
    //                      (specifically Group 3 is handled as I/O instruction)
    _ln_wrt  = ~ (tad3q | grpa1q | iot2q);

    // MA_ALUQ: write MA from ALU output at end of cycle
    //   set when in FETCH2 - for mem ref instrs: memory address calculated from instruction bits being received from raspi
    //                        for non-mem ref: captures all instruction bits being received from raspi
    //               DEFER2 - captures pointer value received from raspi
    //               DEFER3 - captures incremented pointer value being incremented by ALU and sent to raspi
    //                 TAD2 - captures memory value to be added to accumulator next cycle
    //                 ISZ2 - captures memory value to be incremented via ALU in next cycle
    //               INTAK1 - captures 0 being output by ALU like it is the address from a JMS 0 instruction
    _ma_aluq = ~ (fetch2qa | defer2q | defer3q | tad2q | isz2q | intak1q);              // write MA from ALU output at end of cycle

    // PC_ALUQ: write PC from ALU output at end of cycle
    //   set when in JMP1 - captures jump-to address
    //               JMS3 - captures incremented jump-to address
    //            Group 2 - captures possibly incremented address from skip instruction
    _pc_aluq = ~ (jmp1q | jms3q | grpb1q);

    // PC_INC: increment PC at end of cycle (independent of ALU)
    //   set when in FETCH2 - increment PC to point to next instruction in line
    //                 ISZ3 and result zero - increment to skip next instruction
    //                 IOT2 and raspi is saying to skip - increment to skip next instruction
    _pc_inc  = ~ (fetch2qa | isz3q & ~ _alucout | iot2q & ioskp);

    // ALU_MA: gate MA into ALU A input
    //   set when in DEFER1 - sends mem-ref pointer address to raspi
    //               DEFER3 - sends not-yet-incremented pointer value to ALU for incrementing and writing to memory via raspi
    //                EXEC1 for AND,TAD,ISZ,DCA,JMS - sends memory address to raspi via ALU for reading and/or writing
    //                EXEC1 for JMP - sends memory address to PC via ALU
    //                EXEC3 - sends memory value to ALU for addition with accumulator
    _alua_ma    = ~ (defer1q | defer3q | exec1q & _irq[11] | exec1q & _irq[10] | exec3q);

    // ALU_M1: gate 7777 to ALU A input
    //   set when in DCA2 - to pass accumulator sent to ALU B input through to raspi to write to memory
    //               IOT1 - to pass accumulator sent to ALU B input through to raspi for processing I/O
    //            Group 1 with CMA - to complement accumulator set to ALU B input by XORing with all ones
    _alua_m1    = ~ (dca2q | iot1q | grpa1q & maq[5]);

    // ALUA_MQ1107: gate MQ[11:07] (MQ=value from raspi) to ALU A[11:07]  (upper 5 bits)
    //   set when in FETCH2 and IOT,OPR - route upper instruction bits through ALU for writing to MA
    //               DEFER2 - route pointer value bits through ALU for writing to MA
    //               ARITH2 - route operand value bits through ALU for arithmetic
    //                 ISZ2 - route operand value bits through ALU for writing to MA
    //                 IOT2 - route return value from I/O instruction throug ALU for writing to AC
    alua_mq1107 = fetch2qa & _meminst | defer2q | arith2q | isz2q | iot2q;

    // ALUA_MQ0600: gate MQ[06:00] (MQ=value from raspi) to ALU A[06:00]  (lower 5 bits)
    //   set when in FETCH2 - route lower instruction bits through ALU for writing to MA
    //               DEFER2 - route pointer value bits through ALU for writing to MA
    //               ARITH2 - route operand value bits through ALU for arithmetic
    //                 ISZ2 - route operand value bits through ALU for writing to MA
    //                 IOT2 - route return value from I/O instruction throug ALU for writing to AC
    alua_mq0600 = fetch2qa | defer2q | arith2q | isz2q | iot2q;

    // ALUA_PC1107: gate PC[11:07] to ALU A[11:07]  (upper 5 bits)
    //   set when in FETCH1 - route upper PC bits through ALU to send to raspi for fetching instruction
    //               FETCH2 & meminst & page-n : route upper PC bits through ALU for writing to MA
    //                 JMS2 - route upper PC bits through ALU to raspi for writing to memory
    //              Group 2 - route upper PC bits through ALU for possible incrementing for skip instruction
    alua_pc1107 = fetch1q | fetch2qa & meminst & mq[07] | jms2q | grpb1q;

    // ALU_PC0600: gate PC[06:00] to ALU A[06:00]  (lower 7 bits)
    //   set when in FETCH1 - route lower PC bits through ALU to send to raspi for fetching instruction
    //                 JMS2 - route lower PC bits through ALU to send to raspi for writing to memory
    //              Group 2 - route lower PC bits through ALU for possible incrementing for skip instruction
    alua_pc0600 = fetch1q | jms2q | grpb1q;


    // ALUB_AC: gate AC to ALU B input
    //   set when in AND2 - route AC to ALU B input for anding with value from memory being sent to ALU A
    //               TAD3 - route AC to ALU B input for addition with value from MA being sent to ALU A
    //               DCA2 - route AC to ALU B input for writing to memory via raspi
    //               IOT1 - route AC to ALU B input for I/O processing via raspi
    //            Group 1 and not CLA - route AC to ALU B for arithmetic
    _alub_ac = ~ (and2q | tad3q | dca2q | iot1q | grpa1q & _maq[07]);

    // ALUB_M1: gate 7777 to ALU B input
    //   set when in FETCH1 - so PC being routed to ALU A input goes unaltered to raspi
    //               FETCH2 - so instr being routed to ALU A input goes unaltered to MA
    //               DEFER1 - so MA being routed to ALU A input goes unaltered to raspi
    //               DEFER2 - so MQ value routed from raspi to ALU A input goes unlatered to MA
    //                 JMP1 - so PC being routed to ALU A input goes unaltered to PC and raspi for fetching
    //                 JMS1 - so MA being routed to ALU A input goes unaltered to raspi
    //                 JMS2 - so PC being routed to ALU A input goes unaltered to raspi
    //               ARITH1 - so MA being routed to ALU A input goes unaltered to raspi
    //                 TAD2 - so MQ being routed to ALU A input goes unaltered to MA
    //                 DCA1 - so MA being routed to ALU A input goes unaltered to raspi
    //                 ISZ1 - so MA being routed to ALU A input goes unaltered to raspi
    //                 ISZ2 - so MQ being routed to ALU A input goes unaltered to MA
    //                 IOT2 - so MQ being routed to ALU A input goes unaltered to AC
    //        AND2 & IR<09> - so MQ being routed to ALU A input goes unaltered to AC (TAD to 0000 optimized to AND with 7777)
    _alub_m1 = ~ (fetch1q | fetch2q | defer1q | defer2q | jmp1q | jms1q | jms2q |
                  arith1q | tad2q | dca1q | isz1q | isz2q | iot2q | and2q & irq[09]);

    // ALUB_1: gate 0001 to ALU B input
    //   set when in DEFER3 - to increment pointer value and send to raspi and MA
    //                 JMS3 - to increment address to write to PC and send to raspi for fetching
    //                 ISZ3 - to increment value read from memory and send to raspi
    //              Group 2 with skip true - to increment PC and write to PC and send to raspi for fetching
    alub_1   = defer3q | jms3q | isz3q | grpb1q & grpb_skip;

    // state flip flops

    fetch1: DFF (_PS:_reseta, _PC:1, T:clok0a, D:fetch1d,  Q:fetch1q,  _Q:_fetch1q);
    fetch2: DFF (_PS:_reseta, _PC:1, T:clok0a, D:_fetch2d, Q:_fetch2q, _Q:fetch2q);
    intak1: DFF (_PS:_reseta, _PC:1, T:clok0a, D:_intak1d, Q:_intak1q, _Q:intak1q);

    fetch2qa = ~ _fetch2q;

    defer1: DFF (_PS:_resetb, _PC:1, T:clok0b, D:_defer1d, _Q:defer1q, Q:_defer1q);
    defer2: DFF (_PS:_resetb, _PC:1, T:clok0b, D:_defer2d, _Q:defer2q);
    defer3: DFF (_PS:_resetb, _PC:1, T:clok0b, D:_defer3d, _Q:defer3q);

    exec1:  DFF (_PS:_resetc, _PC:1, T:clok0c, D:_exec1d, Q:_exec1q, _Q:exec1q);
    exec2:  DFF (_PS:_resetc, _PC:1, T:clok0c, D:_exec2d, Q:_exec2q, _Q:exec2q);
    exec3:  DFF (_PC:_resetc, _PS:1, T:clok0c, D:exec3d,  Q:exec3q, _Q:_exec3q);

    // instruction register (latch)

    irq_11 = ~ _irq[11];

    ireg11: DLat (_PC:1, _PS:_intak1q, D:mq[11], Q:irq[11], _Q:_irq[11], G:fetch2q);
    ireg10: DLat (_PS:1, _PC:_intak1q, D:mq[10], Q:irq[10], _Q:_irq[10], G:fetch2q);
    ireg09: DLat (_PS:1, _PC:_intak1q, D:mq[09], Q:irq[09], _Q:_irq[09], G:fetch2q);
    ireg08: DLat (_PS:1, _PC:1,        D:mq[08], Q:irq_8,                G:fetch2qa);
}

module seq ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask giiiiiioiiiiiioiiiiiioiiiiiiiiii
#define bmask giiiiiiiiiioiiiiiioiiiiiiiiiiiii
#define cmask giiiiiioiiiiiioiiiiiioooooiooooo
#define dmask gooiooooooiooooiooiooiioiooooiio
#include "cons.inc"

    seqcirc: seqcirc (
        _ac_aluq, _ac_sc, acqzero, _alu_add, _alu_and, _alua_m1, _alua_ma, alua_mq0600, alua_mq1107,
        alua_pc0600, alua_pc1107, alub_1, _alub_ac, _alub_m1, _alucout, clok2, _grpa1q,
        grpb_skip, _dfrm, _jump, inc_axb, _intak, intrq, intak1q, ioinst, ioskp, iot2q,
        _ln_wrt, _ma_aluq, _maq[11:00], maq[11:00], mq[11:00], _mread, _mwrite, _pc_aluq, _pc_inc,
        reset, tad3q,
        fetch1q, fetch2q, defer1q, defer2q, defer3q, exec1q, exec2q, exec3q, irq[11:09]);
}
