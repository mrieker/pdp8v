
// AC and LINK registers

module aclcirc (
    in _ac_aluq, in _ac_sc, out acq[11:00], out acqzero, in _aluq[11:00], in clok2, in _grpa1q, out grpb_skip,
    in iot2q, in _ln_wrt, out _lnq, out lnq, in _maq[11:00], in maq[11:00], in mql,
    in _newlink, in reset, in tad3q)
{
    wire _acg, acta, actb, actc, lnt, acqzer2;
    wire _lnd, _acq[11:00], ac_sca, _ac_sca;
    wire _clok1, _reset;
    wire grpa1q, grpb_skip_base, grpb_skip_nand;
    wire _rotcin, rotcout, rotq[11:00];

    // group 1 rotator
    // - passthru for everything else
    //   grpalnq = (lnq & ~ CLL) ^ CML

    wire _rot_nop, _rot_bsw, _rot_ral, _rot_rtl, _rot_rar, _rot_rtr;
    wire rot_nopa, rot_bswa, rot_rala, rot_rtla, rot_rara, rot_rtra;
    wire rot_nopb, rot_bswb, rot_ralb, rot_rtlb, rot_rarb, rot_rtrb;

    grpa1q   = ~ _grpa1q;

    _rotcin  = _newlink;
    _rot_nop = ~ (_maq[3] & _maq[2] & _maq[1] | _grpa1q);
    _rot_bsw = ~ (_maq[3] & _maq[2] &  maq[1] & grpa1q);
    _rot_ral = ~ (_maq[3] &  maq[2] & _maq[1] & grpa1q);
    _rot_rtl = ~ (_maq[3] &  maq[2] &  maq[1] & grpa1q);
    _rot_rar = ~ ( maq[3] & _maq[2] & _maq[1] & grpa1q);
    _rot_rtr = ~ ( maq[3] & _maq[2] &  maq[1] & grpa1q);

    rot_nopa = ~ _rot_nop;
    rot_nopb = ~ _rot_nop;
    rot_bswa = ~ _rot_bsw;
    rot_bswb = ~ _rot_bsw;
    rot_rala = ~ _rot_ral;
    rot_ralb = ~ _rot_ral;
    rot_rtla = ~ _rot_rtl;
    rot_rtlb = ~ _rot_rtl;
    rot_rara = ~ _rot_rar;
    rot_rarb = ~ _rot_rar;
    rot_rtra = ~ _rot_rtr;
    rot_rtrb = ~ _rot_rtr;

    rotq[00] = ~ (rot_nopa & _aluq[00] | rot_bswa & _aluq[06] | rot_rala & _rotcin   | rot_rtla & _aluq[11] | rot_rara & _aluq[01] | rot_rtra & _aluq[02]);
    rotq[01] = ~ (rot_nopa & _aluq[01] | rot_bswa & _aluq[07] | rot_rala & _aluq[00] | rot_rtla & _rotcin   | rot_rara & _aluq[02] | rot_rtra & _aluq[03]);
    rotq[02] = ~ (rot_nopa & _aluq[02] | rot_bswa & _aluq[08] | rot_rala & _aluq[01] | rot_rtla & _aluq[00] | rot_rara & _aluq[03] | rot_rtra & _aluq[04]);
    rotq[03] = ~ (rot_nopb & _aluq[03] | rot_bswb & _aluq[09] | rot_ralb & _aluq[02] | rot_rtlb & _aluq[01] | rot_rarb & _aluq[04] | rot_rtrb & _aluq[05]);
    rotq[04] = ~ (rot_nopb & _aluq[04] | rot_bswb & _aluq[10] | rot_ralb & _aluq[03] | rot_rtlb & _aluq[02] | rot_rarb & _aluq[05] | rot_rtrb & _aluq[06]);
    rotq[05] = ~ (rot_nopb & _aluq[05] | rot_bswb & _aluq[11] | rot_ralb & _aluq[04] | rot_rtlb & _aluq[03] | rot_rarb & _aluq[06] | rot_rtrb & _aluq[07]);
    rotq[06] = ~ (rot_nopa & _aluq[06] | rot_bswa & _aluq[00] | rot_rala & _aluq[05] | rot_rtla & _aluq[04] | rot_rara & _aluq[07] | rot_rtra & _aluq[08]);
    rotq[07] = ~ (rot_nopa & _aluq[07] | rot_bswa & _aluq[01] | rot_rala & _aluq[06] | rot_rtla & _aluq[05] | rot_rara & _aluq[08] | rot_rtra & _aluq[09]);
    rotq[08] = ~ (rot_nopa & _aluq[08] | rot_bswa & _aluq[02] | rot_rala & _aluq[07] | rot_rtla & _aluq[06] | rot_rara & _aluq[09] | rot_rtra & _aluq[10]);
    rotq[09] = ~ (rot_nopb & _aluq[09] | rot_bswb & _aluq[03] | rot_ralb & _aluq[08] | rot_rtlb & _aluq[07] | rot_rarb & _aluq[10] | rot_rtrb & _aluq[11]);
    rotq[10] = ~ (rot_nopb & _aluq[10] | rot_bswb & _aluq[04] | rot_ralb & _aluq[09] | rot_rtlb & _aluq[08] | rot_rarb & _aluq[11] | rot_rtrb & _rotcin);
    rotq[11] = ~ (rot_nopb & _aluq[11] | rot_bswb & _aluq[05] | rot_ralb & _aluq[10] | rot_rtlb & _aluq[09] | rot_rarb & _rotcin   | rot_rtrb & _aluq[00]);
    rotcout  = ~ (rot_nopa & _rotcin   | rot_bswa & _rotcin   | rot_rala & _aluq[11] | rot_rtla & _aluq[10] | rot_rara & _aluq[00] | rot_rtra & _aluq[01]);

    // registers

    _clok1 = ~ clok2;
    _reset = ~ reset;

    acwff: DFF (_PS:_reset, _PC:1, D:_ac_aluq, T:_clok1, Q:_acg);

    acta = ~ (_acg | _clok1);
    actb = ~ (_acg | _clok1);
    actc = ~ (_acg | _clok1);

     ac_sca = ~ _ac_sc;
    _ac_sca = ~ ac_sca;

    //  _ac_aluq   _ln_wrt   _acg   _clok1  =>  lnt
    //      1        1         1       1         0                      2nd half of cycle not writing AC
    //      1        1         1       0         0                      start of new cycle that writes L,AC
    //      x        x         1       0         0                      still in first half, _ac_aluq, _ln_wrt in transition
    //      0        0         1       0         0                      still in first half, _ac_aluq, _ln_wrt settled
    //      0        0         1       1         0                      start 2nd half of cycle
    //      0        0         0       1         0                      still in second half, _acg updated
    //      0        0         0       0         0      t=0             start new cycle
    //      0        0         0       0         1      t=1*tg          start writing link
    //      x        1         0       0         1      t=3*tg+1*td     new cycle does not write link
    //      x        1         0       0         0      t=4*tg+1*td     quickly stop writing link
    //      1        1         0       0         0                      new cycle does not write accum
    //      1        1         0       1         0                      start 2nd half of cycle
    //      1        1         1       1         0                      _acg updated
    lnt = ~ (_acg | _ln_wrt | _clok1);

    _lnd = ~ (tad3q & rotcout |         // TAD
              grpa1q & rotcout |        // group 1
              iot2q & mql);             // iot (for group 3 EAE)

    aclo:  DFF 4 _SC (_PC:1, _PS:1, D:rotq[03:00], Q:acq[03:00], _Q:_acq[03:00], T:acta, _SC:_ac_sc);
    acmid: DFF 4 _SC (_PC:1, _PS:1, D:rotq[07:04], Q:acq[07:04], _Q:_acq[07:04], T:actb, _SC:_ac_sca);
    achi:  DFF 4 _SC (_PC:1, _PS:1, D:rotq[11:08], Q:acq[11:08], _Q:_acq[11:08], T:actc, _SC:_ac_sca);
    lnreg: DFF       (_PC:1, _PS:1, D:_lnd, Q:_lnq, _Q:lnq, T:lnt);

    // group 2 skip
    //  [3] = reverse sense
    //  [4] = skip if link set
    //  [5] = skip if AC zero
    //  [6] = skip if AC neg

    acqzdao: DAO 12 (_acq, acqzero);    // used for TAD optimization
    acqzda2: DAO 12 (_acq, acqzer2);    // used for SZA instruction
    grpb_skip_base = ~ (lnq & maq[4] | maq[5] & acqzer2 | maq[6] & acq[11]);
    grpb_skip_nand = ~ (maq[3] & grpb_skip_base);
    grpb_skip      = ~ (maq[3] & grpb_skip_nand | grpb_skip_nand & grpb_skip_base);
}

module acl ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask goiiiiiioiiiiiioiiiiiioiiiiiioii
#define bmask giiiioiiiiiioiiiiiioiiiiiooiiiii
#define cmask goiiiiiioiiiiiioiiiiiiiiiiiiiiii
#define dmask giiiiiiiiioiiiiiiiiiiooiiiiiiiii
#include "cons.inc"

    aclcirc: aclcirc (
        _ac_aluq, _ac_sc, acq[11:00], acqzero, _aluq[11:00], clok2, _grpa1q, grpb_skip,
        iot2q, _ln_wrt, _lnq, lnq, _maq[11:00], maq[11:00], mql,
        _newlink, reset, tad3q);
}
