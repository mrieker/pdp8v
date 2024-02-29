
# experiment board

module exp ()
{
    pwr1: PwrConn VF1 VF1 GND GND VF2 VF2 ();
    pwr2: PwrConn VPP GND VGG GND VCC VCC ();

    wire _aluq[1:0], _dfrm, _jump, _intak, ioinst, _lnq, _mread, _mwrite;
    wire clok2, intrq, ioskp, mql, mq[1:0], reset;

    rpi: RasPi test2 (
              _MD:_aluq,
            _DFRM:_dfrm,
            _JUMP:_jump,
             _IAK:_intak,
             IOIN:ioinst,
             _MDL:_lnq,
             _MRD:_mread,
             _MWT:_mwrite,

              CLK:clok2,
              IRQ:intrq,
              IOS:ioskp,
              MQL:mql,
               MQ:mq,
              RES:reset);

    wire aluq[1:0], clok0, _clok1, _reset;

    _clok1 = ~ clok2;
    clok0 = ~ _clok1;
    _reset = ~ reset;

    r00ff: DFF   (_PS:_reset, _PC:1, D:mq[1], Q:aluq[0], _Q:_aluq[0], T:clok0);
    r01ff: DFF   (_PS:_reset, _PC:1, D:mq[0], Q:aluq[1], _Q:_aluq[1], T:clok0);
    intff: DFF   (_PC:_reset, _PS:1, D:intrq, _Q:_intak, T:clok0);
    jmpff: DFF   (_PC:_reset, _PS:1, D:_mwrite, Q:_jump, T:clok0);

    _dfrm   = ~ (mql & ioskp & aluq[1] | aluq[0]);
    ioinst  = ~ (_mwrite | ioskp & mql);                # ioinst = ioskp ^ mql
    _lnq    = ~ (aluq[0] & _mread | _mread & aluq[1]);  # _lnq = ~ (aluq[0] ^ aluq[1])
    _mread  = ~ (aluq[0] & aluq[1]);
    _mwrite = ~ (ioskp | mql);

     _LNQ: BufLED (_lnq);
     ACQ0: BufLED (aluq[0]);
    _ACQ1: BufLED (_aluq[1]);
}
