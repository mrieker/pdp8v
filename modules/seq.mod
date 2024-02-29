
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

    _clok1 = ~ clok2;
    clok0a = ~ _clok1;
    clok0b = ~ _clok1;
    clok0c = ~ _clok1;

    _reseta = ~ reset;
    _resetb = ~ reset;
    _resetc = ~ reset;

    _autoidx = ~ (_maq[11] & _maq[10] & _maq[09] & _maq[08] & _maq[07] & _maq[06] & _maq[05] & _maq[04] & maq[03]);
    autoidx  = ~ _autoidx;
    meminst  = ~ (mq[11] & mq[10]);     // valid at end of fetch2 only
    _meminst = ~ meminst;

    // determine next state based on current state and inputs

    _endinst = ~ (and2q | tad3q | dca2q | isz3q | iot2q | grpa1q);
    _intrq   = ~ intrq;

    _intak1d = ~ (~ _endinst & intrq | jmp1q & intrq | jms3q & intrq | grpb1q & intrq);
    fetch1d  = ~ (_endinst | intrq);
    _fetch2d = ~ (fetch1q  | jmp1q & _intrq | jms3q & _intrq | grpb1q & _intrq);

    _defer1d = ~ (fetch2qa & meminst & mq[08]);
    _defer2d = _defer1q;
    _defer3d = ~ (defer2q & autoidx);

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

    _exec1d  = ~ (fetch2qa & ~ mq[08] |                         // memory instuction direct addressing
                  defer2q & _autoidx |                          // deferred non-auto-index
                  defer3q |                                     // deferred auto-index
                  fetch2qa & _meminst);                         // non-memory instruction
    _exec2d  = ~ (exec1q & ~ (irq[11] & _ir_jms & _ir_iot) |    // arith2, isz2, dca2, jms2, iot2
                  intak1q);                                     // also jms2 follows intak1q
     exec3d  = ~ (_exec2q | _ir_tad & _ir_isz & _ir_jms);       // tad3, isz3, jms3

    // split exec1,2,3 states into instruction states

    arith1q = ~ (_exec1q | _ir_arth);
    arith2q = ~ (_exec2q | _ir_arth);
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

    _mread  = ~ (fetch1q | defer1q | arith1q | isz1q | jmp1q & _intrq | jms3q & _intrq | grpb1q & _intrq);  // read memory
    _mwrite = ~ (defer1q & autoidx | jms1q | dca1q | isz1q | intak1q);      // write memory
    _dfrm   = ~ (exec1q & _irq[11] & irq_8);                                // use data frame to access memory this cycle
    _jump   = ~ (exec1q & irq_11 & _irq[10]);                               // doing a JMP/JMS instruction
    _intak  = _intak1q;                                                     // interrupt acknowledge
    ioinst  = iot1q;                                                        // doing IOT/Group3 instruction

    _alu_add = ~ (defer3q | exec3q | grpb1q);                               // ALU should perform ADD A + B
    _alu_and = ~ (_alu_add & _grpa1q);                                      // ALU should perform AND A & B
    inc_axb  = ~ (_grpa1q | _maq[0]);                                       // ALU should perform INC (A ^ B) + 1

    _ac_sc   = ~ (dca2q | grpb1q & maq[7]);                                             // write zero to AC instead of from ALU at end of cycle
    _ac_aluq = ~ (and2q | tad3q | dca2q | grpa1q | grpb1q & maq[7] | iot2q);            // write AC from ALU output at end of cycle
    _ln_wrt  = ~ (tad3q | grpa1q | iot2q);                                              // write link from various sources at end of cycle
                                                                                        // - must be subset of _ac_aluq
    _ma_aluq = ~ (fetch2qa | defer2q | defer3q | tad2q | isz2q | intak1q);              // write MA from ALU output at end of cycle
    _pc_aluq = ~ (jmp1q | jms3q | grpb1q);                                              // write PC from ALU output at end of cycle
    _pc_inc  = ~ (fetch2qa | isz3q & ~ _alucout | iot2q & ioskp);                       // increment PC at end of cycle

    _alua_ma    = ~ (defer1q | defer3q | exec1q & _irq[11] | exec1q & _irq[10] | exec3q);  // gate MA into ALU A input
    _alua_m1    = ~ (dca2q | iot1q | grpa1q & maq[5]);                                  // gate 7777 into ALU A input
    alua_mq1107 = fetch2qa & _meminst | defer2q | arith2q | isz2q | iot2q;              // gate MQ[11:07] into ALU A[11:07]
    alua_mq0600 = fetch2qa | defer2q | arith2q | isz2q | iot2q;                         // gate MQ[06:00] into ALU A[06:00]
    alua_pc1107 = fetch1q | fetch2qa & meminst & mq[07] | jms2q | grpb1q;               // gate PC[11:07] into ALU A[11:07]
    alua_pc0600 = fetch1q | jms2q | grpb1q;                                             // gate PC[06:00] into ALU A[06:00]

    _alub_ac = ~ (and2q | tad3q | dca2q | iot1q | grpa1q & _maq[07]);                   // gate AC into ALU B
    _alub_m1 = ~ (fetch1q | fetch2q | defer1q | defer2q | jmp1q | jms1q | jms2q |       // gate 7777 into ALU B
                  arith1q | tad2q | dca1q | isz1q | isz2q | iot2q | and2q & irq[09]);
    alub_1   = defer3q | jms3q | isz3q | grpb1q & grpb_skip;                            // gate 0001 into ALU B

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

    // instruction register

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
