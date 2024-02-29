
// arithmetic logic unit - all combinational (no sequential) logic

//   alua_*: select the A operand (ma, mq, pc, 0, m1)
//   alub_*: select the B operand (ac, 0, 1, m1)

//   alu_add: add A + B, produce carry at alucout
//   alu_and: and A & B
//   grpa1q:  (A ^ B) + inc_axb, produce carry at alucout

// input:
//  acq = accumulator register contents
//  _alu_add = perform addition (alua + alub), no carry in, rotcout has carry out
//  _alu_and = perform bitwise and
//  _alua_m1 = gate 7777 to A operand [11:00]
//  _alua_ma = gate MA register to A operand [11:00]
//  alua_mq0600 = gate RASPI data bits [06:00] to A operand [06:00]
//  alua_mq1107 = gate RASPI data bits [11:07] to A operand [11:07]
//  alua_pc0600 = gate PC register [06:00] to A operand [06:00]
//  alua_pc1107 = gate PC register [11:07] to A operand [11:07]
//    note: A operand is 0 where no operand is selected
//  alub_1 = gate 0001 to B operand
//  _alub_ac = gate accumulator to B operand
//  _alub_m1 = gate 7777 to B operand
//    note: B operand is 0 where no operand is selected
//  _grpa1q = perform 'group 1' arithmetic according to opcode in MA register
//  inc_axb = compute (A ^ B) + 1, rotcout has carry
//  lnq = link register contents
//  _maq,maq = MA register contents
//  mq = data value from RASPI
//  pcq = PC register contents
// output:
//  _aluq = unrotated output
//  _alucout = carry output from _alu_add,group1 operation, 0 for _alu_and

module alucirc (
    in acq[11:00], in _alu_add, in _alu_and, in _alua_m1, in _alua_ma, in alua_mq0600, in alua_mq1107,
    in alua_pc0600, in alua_pc1107, in alub_1, in _alub_ac, in _alub_m1, out _alucout, out _aluq[11:00], in _grpa1q,
    in inc_axb, in _lnq, in lnq, in _maq[11:00], in maq[11:00], in mq[11:00], out _newlink, in pcq[11:00])
{
    wire _alua[11:00], _alub[11:00];
    wire alua_ma1107, alua_ma0600, alua_m1a, alua_m1b;
    wire alub_aca, alub_acb, alub_m1a, alub_m1b;
    wire alu_adda, alu_addb, alu_addc, alu_anda, alu_andb;
    wire cin_add_04, cin_add_08, cin_add_12;
    wire cin_inc_04, cin_inc_08, cin_inc_12;
    wire _cin_add_04, _cin_add_08, _cin_add_12;
    wire _cin_inc_04, _cin_inc_08, _cin_inc_12;
    wire aandb[11:00], axorb[11:00], axbxorc[11:00];
    wire cin[11:00];
    wire grpa1qa, grpa1qb, _oldlink, newnand;

    alua_m1a = ~ _alua_m1;
    alua_m1b = ~ _alua_m1;
    alua_ma1107 = ~ _alua_ma;
    alua_ma0600 = ~ _alua_ma;

    grpa1qa = ~ _grpa1q;
    grpa1qb = ~ _grpa1q;

    _alua[11:07] = ~ (alua_ma1107 & maq[11:07] | alua_mq1107 & mq[11:07] | alua_pc1107 & pcq[11:07] | alua_m1a);
    _alua[06:00] = ~ (alua_ma0600 & maq[06:00] | alua_mq0600 & mq[06:00] | alua_pc0600 & pcq[06:00] | alua_m1b);

    alub_aca = ~ _alub_ac;
    alub_acb = ~ _alub_ac;
    alub_m1a = ~ _alub_m1;
    alub_m1b = ~ _alub_m1;

    _alub[11:07] = ~ (alub_aca & acq[11:07] | alub_m1a);
    _alub[06:01] = ~ (alub_acb & acq[06:01] | alub_m1b);
    _alub[00:00] = ~ (alub_acb & acq[00:00] | alub_m1b | alub_1);

    alu_adda = ~ _alu_add;
    alu_addb = ~ _alu_add;
    alu_addc = ~ _alu_add;
    alu_anda = ~ _alu_and;
    alu_andb = ~ _alu_and;

    aandb = ~ (_alua | _alub);
    axorb = ~ (aandb | _alua & _alub);

    _cin_add_04 = ~ (aandb[03] | axorb[03] & aandb[02] | axorb[03] & axorb[02] & aandb[01] | axorb[03] & axorb[02] & axorb[01] & aandb[00]);
    _cin_add_08 = ~ (aandb[07] | axorb[07] & aandb[06] | axorb[07] & axorb[06] & aandb[05] | axorb[07] & axorb[06] & axorb[05] & aandb[04] | axorb[07] & axorb[06] & axorb[05] & axorb[04] & cin_add_04);
    _cin_add_12 = ~ (aandb[11] | axorb[11] & aandb[10] | axorb[11] & axorb[10] & aandb[09] | axorb[11] & axorb[10] & axorb[09] & aandb[08] | axorb[11] & axorb[10] & axorb[09] & axorb[08] & cin_add_08);

    _cin_inc_04 = ~ (axorb[03] & axorb[02] & axorb[01] & axorb[00] & inc_axb);
    _cin_inc_08 = ~ (axorb[07] & axorb[06] & axorb[05] & axorb[04] & cin_inc_04);
    _cin_inc_12 = ~ (axorb[11] & axorb[10] & axorb[09] & axorb[08] & cin_inc_08);

    cin_add_04 = ~ _cin_add_04;
    cin_add_08 = ~ _cin_add_08;
    cin_add_12 = ~ _cin_add_12;

    cin_inc_04 = ~ _cin_inc_04;
    cin_inc_08 = ~ _cin_inc_08;
    cin_inc_12 = ~ _cin_inc_12;

    cin[00] = inc_axb;
    cin[01] = alu_addc & aandb[00] | axorb[00] & cin[00];
    cin[02] = alu_addc & aandb[01] | axorb[01] & cin[01];
    cin[03] = alu_addc & aandb[02] | axorb[02] & cin[02];
    cin[04] = ~ (alu_addc & _cin_add_04 | grpa1qa & _cin_inc_04);
    cin[05] = alu_addc & aandb[04] | axorb[04] & cin[04];
    cin[06] = alu_addc & aandb[05] | axorb[05] & cin[05];
    cin[07] = alu_addc & aandb[06] | axorb[06] & cin[06];
    cin[08] = ~ (alu_addc & _cin_add_08 | grpa1qb & _cin_inc_08);
    cin[09] = alu_addb & aandb[08] | axorb[08] & cin[08];
    cin[10] = alu_addb & aandb[09] | axorb[09] & cin[09];
    cin[11] = alu_adda & aandb[10] | axorb[10] & cin[10];

    axbxorc = ~ (~ (axorb | cin) | axorb & cin);

    _aluq[11:06] = ~ (alu_anda & aandb[11:06] | alu_adda & axbxorc[11:06] | grpa1qa & axbxorc[11:06]);
    _aluq[05:00] = ~ (alu_andb & aandb[05:00] | alu_addb & axbxorc[05:00] | grpa1qb & axbxorc[05:00]);

    // flips link for add; zero for and
    //  alucout = 0: leave link as is (and, add/inc without carry)
    //            1: flip link (add/inc with carry)
    _alucout = ~ (alu_adda & cin_add_12 | grpa1qa & cin_inc_12);

    // compute new link
    //  oldlink = not group1: link register
    //            group1 no CLL CML: link register
    //            group1 CML only: ~ link register
    //            group1 CLL: CML bit
    //  newlink = oldlink ^ alucout
    _oldlink = ~ (_grpa1q & lnq | _maq[6] & _maq[4] & lnq | grpa1qb & _maq[6] & maq[4] & _lnq | grpa1qa & maq[6] & maq[4]);
    newnand  = ~ (_oldlink & _alucout);
    _newlink = ~ (_oldlink & newnand | newnand & _alucout);
}

module alu ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask gioiiiiiioiiiiiioiiiiiioiiiiiioi
#define bmask giiiiioiiiiiioiiiiiioiiiiiioiiii
#define cmask gioiiiiiioiiiiiioiiiiiiiiioiiiii
#define dmask giiiiiiiiiiiiiiiiiiiiiiiiiiiiioi
#include "cons.inc"

    alucirc: alucirc (
        acq[11:00], _alu_add, _alu_and, _alua_m1, _alua_ma, alua_mq0600, alua_mq1107,
        alua_pc0600, alua_pc1107, alub_1, _alub_ac, _alub_m1, _alucout, _aluq[11:00], _grpa1q,
        inc_axb, _lnq, lnq, _maq[11:00], maq[11:00], mq[11:00], _newlink, pcq[11:00]);
}
