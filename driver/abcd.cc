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

/**
 * Decode/Encode ABCD and GPIO connector pins to/from various signals.
 */

#include <string.h>

#include "abcd.h"
#include "pindefs.h"

ABCD::ABCD ()
{
    memset (cons, 0, sizeof cons);
    decode ();
}

// decode {a,b,c,d}con => signals
void ABCD::decode ()
{
    acqzero     = (bcon & B_acqzero    ) != 0;
    _jump       = (bcon & B__jump      ) != 0;
    _alub_m1    = (bcon & B__alub_m1   ) != 0;

    _ac_sc      = (ccon & C__ac_sc     ) != 0;
    intak1q     = (ccon & C_intak1q    ) != 0;
    fetch1q     = (ccon & C_fetch1q    ) != 0;
    _ac_aluq    = (ccon & C__ac_aluq   ) != 0;
    _alu_add    = (ccon & C__alu_add   ) != 0;
    _alu_and    = (ccon & C__alu_and   ) != 0;
    _alua_m1    = (ccon & C__alua_m1   ) != 0;
    _alucout    = (ccon & C__alucout   ) != 0;
    _alua_ma    = (ccon & C__alua_ma   ) != 0;
    alua_mq0600 = (ccon & C_alua_mq0600) != 0;
    alua_mq1107 = (ccon & C_alua_mq1107) != 0;
    alua_pc0600 = (ccon & C_alua_pc0600) != 0;
    alua_pc1107 = (ccon & C_alua_pc1107) != 0;

    alub_1      = (dcon & D_alub_1     ) != 0;
    _alub_ac    = (dcon & D__alub_ac   ) != 0;
    clok2       = (dcon & D_clok2      ) != 0;
    fetch2q     = (dcon & D_fetch2q    ) != 0;
    _grpa1q     = (dcon & D__grpa1q    ) != 0;
    defer1q     = (dcon & D_defer1q    ) != 0;
    defer2q     = (dcon & D_defer2q    ) != 0;
    defer3q     = (dcon & D_defer3q    ) != 0;
    exec1q      = (dcon & D_exec1q     ) != 0;
    grpb_skip   = (dcon & D_grpb_skip  ) != 0;
    exec2q      = (dcon & D_exec2q     ) != 0;
    _dfrm       = (dcon & D__dfrm      ) != 0;
    inc_axb     = (dcon & D_inc_axb    ) != 0;
    _intak      = (dcon & D__intak     ) != 0;
    intrq       = (dcon & D_intrq      ) != 0;
    exec3q      = (dcon & D_exec3q     ) != 0;
    ioinst      = (dcon & D_ioinst     ) != 0;
    ioskp       = (dcon & D_ioskp      ) != 0;
    iot2q       = (dcon & D_iot2q      ) != 0;
    _ln_wrt     = (dcon & D__ln_wrt    ) != 0;
    _lnq        = (dcon & D__lnq       ) != 0;
    lnq         = (dcon & D_lnq        ) != 0;
    _ma_aluq    = (dcon & D__ma_aluq   ) != 0;
    mql         = (dcon & D_mql        ) != 0;
    _mread      = (dcon & D__mread     ) != 0;
    _mwrite     = (dcon & D__mwrite    ) != 0;
    _pc_aluq    = (dcon & D__pc_aluq   ) != 0;
    _pc_inc     = (dcon & D__pc_inc    ) != 0;
    reset       = (dcon & D_reset      ) != 0;
    _newlink    = (dcon & D__newlink   ) != 0;
    tad3q       = (dcon & D_tad3q      ) != 0;

    CLOCK = (gcon & G_CLOCK) != 0;
    RESET = (gcon & G_RESET) != 0;
    LINK  = (gcon & G_LINK) != 0;
    IOS   = (gcon & G_IOS) != 0;
    QENA  = (gcon & G_QENA) != 0;
    IRQ   = (gcon & G_IRQ) != 0;
    DENA  = (gcon & G_DENA) != 0;
    JUMP  = (gcon & G_JUMP) != 0;
    IOIN  = (gcon & G_IOIN) != 0;
    DFRM  = (gcon & G_DFRM) != 0;
    READ  = (gcon & G_READ) != 0;
    WRITE = (gcon & G_WRITE) != 0;
    IAK   = (gcon & G_IAK) != 0;

    acq   = 0;
    _aluq = 0;
    _maq  = 0;
    maq   = 0;
    mq    = 0;
    pcq   = 0;
    irq   = 0;

    if (acon & A_acq_11     ) acq      |= (1U << 11);
    if (acon & A__aluq_11   ) _aluq    |= (1U << 11);
    if (acon & A__maq_11    ) _maq     |= (1U << 11);
    if (acon & A_maq_11     ) maq      |= (1U << 11);
    if (acon & A_mq_11      ) mq       |= (1U << 11);
    if (acon & A_pcq_11     ) pcq      |= (1U << 11);
    if (acon & A_irq_11     ) irq      |= (1U << 11);
    if (acon & A_acq_10     ) acq      |= (1U << 10);
    if (acon & A__aluq_10   ) _aluq    |= (1U << 10);
    if (acon & A__maq_10    ) _maq     |= (1U << 10);
    if (acon & A_maq_10     ) maq      |= (1U << 10);
    if (acon & A_mq_10      ) mq       |= (1U << 10);
    if (acon & A_pcq_10     ) pcq      |= (1U << 10);
    if (acon & A_irq_10     ) irq      |= (1U << 10);
    if (acon & A_acq_09     ) acq      |= (1U <<  9);
    if (acon & A__aluq_09   ) _aluq    |= (1U <<  9);
    if (acon & A__maq_09    ) _maq     |= (1U <<  9);
    if (acon & A_maq_09     ) maq      |= (1U <<  9);
    if (acon & A_mq_09      ) mq       |= (1U <<  9);
    if (acon & A_pcq_09     ) pcq      |= (1U <<  9);
    if (acon & A_irq_09     ) irq      |= (1U <<  9);
    if (acon & A_acq_08     ) acq      |= (1U <<  8);
    if (acon & A__aluq_08   ) _aluq    |= (1U <<  8);
    if (acon & A__maq_08    ) _maq     |= (1U <<  8);
    if (acon & A_maq_08     ) maq      |= (1U <<  8);
    if (acon & A_mq_08      ) mq       |= (1U <<  8);
    if (acon & A_pcq_08     ) pcq      |= (1U <<  8);
    if (acon & A_acq_07     ) acq      |= (1U <<  7);
    if (acon & A__aluq_07   ) _aluq    |= (1U <<  7);
    if (acon & A__maq_07    ) _maq     |= (1U <<  7);

    if (bcon & B_maq_07     ) maq      |= (1U <<  7);
    if (bcon & B_mq_07      ) mq       |= (1U <<  7);
    if (bcon & B_pcq_07     ) pcq      |= (1U <<  7);
    if (bcon & B_acq_06     ) acq      |= (1U <<  6);
    if (bcon & B__aluq_06   ) _aluq    |= (1U <<  6);
    if (bcon & B__maq_06    ) _maq     |= (1U <<  6);
    if (bcon & B_maq_06     ) maq      |= (1U <<  6);
    if (bcon & B_mq_06      ) mq       |= (1U <<  6);
    if (bcon & B_pcq_06     ) pcq      |= (1U <<  6);
    if (bcon & B_acq_05     ) acq      |= (1U <<  5);
    if (bcon & B__aluq_05   ) _aluq    |= (1U <<  5);
    if (bcon & B__maq_05    ) _maq     |= (1U <<  5);
    if (bcon & B_maq_05     ) maq      |= (1U <<  5);
    if (bcon & B_mq_05      ) mq       |= (1U <<  5);
    if (bcon & B_pcq_05     ) pcq      |= (1U <<  5);
    if (bcon & B_acq_04     ) acq      |= (1U <<  4);
    if (bcon & B__aluq_04   ) _aluq    |= (1U <<  4);
    if (bcon & B__maq_04    ) _maq     |= (1U <<  4);
    if (bcon & B_maq_04     ) maq      |= (1U <<  4);
    if (bcon & B_mq_04      ) mq       |= (1U <<  4);
    if (bcon & B_pcq_04     ) pcq      |= (1U <<  4);
    if (bcon & B_acq_03     ) acq      |= (1U <<  3);
    if (bcon & B__aluq_03   ) _aluq    |= (1U <<  3);
    if (bcon & B__maq_03    ) _maq     |= (1U <<  3);
    if (bcon & B_maq_03     ) maq      |= (1U <<  3);
    if (bcon & B_mq_03      ) mq       |= (1U <<  3);
    if (bcon & B_pcq_03     ) pcq      |= (1U <<  3);

    if (ccon & C_acq_02     ) acq      |= (1U <<  2);
    if (ccon & C__aluq_02   ) _aluq    |= (1U <<  2);
    if (ccon & C__maq_02    ) _maq     |= (1U <<  2);
    if (ccon & C_maq_02     ) maq      |= (1U <<  2);
    if (ccon & C_mq_02      ) mq       |= (1U <<  2);
    if (ccon & C_pcq_02     ) pcq      |= (1U <<  2);
    if (ccon & C_acq_01     ) acq      |= (1U <<  1);
    if (ccon & C__aluq_01   ) _aluq    |= (1U <<  1);
    if (ccon & C__maq_01    ) _maq     |= (1U <<  1);
    if (ccon & C_maq_01     ) maq      |= (1U <<  1);
    if (ccon & C_mq_01      ) mq       |= (1U <<  1);
    if (ccon & C_pcq_01     ) pcq      |= (1U <<  1);
    if (ccon & C_acq_00     ) acq      |= (1U <<  0);
    if (ccon & C__aluq_00   ) _aluq    |= (1U <<  0);
    if (ccon & C__maq_00    ) _maq     |= (1U <<  0);
    if (ccon & C_maq_00     ) maq      |= (1U <<  0);
    if (ccon & C_mq_00      ) mq       |= (1U <<  0);
    if (ccon & C_pcq_00     ) pcq      |= (1U <<  0);

    DATA = (gcon & G_DATA) / G_DATA0;
}

// encode signals => {a,b,c,d}con
void ABCD::encode ()
{
    acon = 0;
    bcon = 0;
    ccon = 0;
    dcon = 0;
    gcon = 0;

    if (acqzero    ) bcon |= B_acqzero    ;
    if (_jump      ) bcon |= B__jump      ;
    if (_alub_m1   ) bcon |= B__alub_m1   ;

    if (_ac_sc     ) ccon |= C__ac_sc     ;
    if (intak1q    ) ccon |= C_intak1q    ;
    if (fetch1q    ) ccon |= C_fetch1q    ;
    if (_ac_aluq   ) ccon |= C__ac_aluq   ;
    if (_alu_add   ) ccon |= C__alu_add   ;
    if (_alu_and   ) ccon |= C__alu_and   ;
    if (_alua_m1   ) ccon |= C__alua_m1   ;
    if (_alucout   ) ccon |= C__alucout   ;
    if (_alua_ma   ) ccon |= C__alua_ma   ;
    if (alua_mq0600) ccon |= C_alua_mq0600;
    if (alua_mq1107) ccon |= C_alua_mq1107;
    if (alua_pc0600) ccon |= C_alua_pc0600;
    if (alua_pc1107) ccon |= C_alua_pc1107;

    if (alub_1     ) dcon |= D_alub_1     ;
    if (_alub_ac   ) dcon |= D__alub_ac   ;
    if (clok2      ) dcon |= D_clok2      ;
    if (fetch2q    ) dcon |= D_fetch2q    ;
    if (_grpa1q    ) dcon |= D__grpa1q    ;
    if (defer1q    ) dcon |= D_defer1q    ;
    if (defer2q    ) dcon |= D_defer2q    ;
    if (defer3q    ) dcon |= D_defer3q    ;
    if (exec1q     ) dcon |= D_exec1q     ;
    if (grpb_skip  ) dcon |= D_grpb_skip  ;
    if (exec2q     ) dcon |= D_exec2q     ;
    if (_dfrm      ) dcon |= D__dfrm      ;
    if (inc_axb    ) dcon |= D_inc_axb    ;
    if (_intak     ) dcon |= D__intak     ;
    if (intrq      ) dcon |= D_intrq      ;
    if (exec3q     ) dcon |= D_exec3q     ;
    if (ioinst     ) dcon |= D_ioinst     ;
    if (ioskp      ) dcon |= D_ioskp      ;
    if (iot2q      ) dcon |= D_iot2q      ;
    if (_ln_wrt    ) dcon |= D__ln_wrt    ;
    if (_lnq       ) dcon |= D__lnq       ;
    if (lnq        ) dcon |= D_lnq        ;
    if (_ma_aluq   ) dcon |= D__ma_aluq   ;
    if (mql        ) dcon |= D_mql        ;
    if (_mread     ) dcon |= D__mread     ;
    if (_mwrite    ) dcon |= D__mwrite    ;
    if (_pc_aluq   ) dcon |= D__pc_aluq   ;
    if (_pc_inc    ) dcon |= D__pc_inc    ;
    if (reset      ) dcon |= D_reset      ;
    if (_newlink   ) dcon |= D__newlink   ;
    if (tad3q      ) dcon |= D_tad3q      ;

    if (CLOCK      ) gcon |= G_CLOCK      ;
    if (RESET      ) gcon |= G_RESET      ;
    if (LINK       ) gcon |= G_LINK       ;
    if (IOS        ) gcon |= G_IOS        ;
    if (QENA       ) gcon |= G_QENA       ;
    if (IRQ        ) gcon |= G_IRQ        ;
    if (DENA       ) gcon |= G_DENA       ;
    if (JUMP       ) gcon |= G_JUMP       ;
    if (IOIN       ) gcon |= G_IOIN       ;
    if (DFRM       ) gcon |= G_DFRM       ;
    if (READ       ) gcon |= G_READ       ;
    if (WRITE      ) gcon |= G_WRITE      ;
    if (IAK        ) gcon |= G_IAK        ;

    if (acq      & (1U << 11)) acon |= A_acq_11     ;
    if (_aluq    & (1U << 11)) acon |= A__aluq_11   ;
    if (_maq     & (1U << 11)) acon |= A__maq_11    ;
    if (maq      & (1U << 11)) acon |= A_maq_11     ;
    if (mq       & (1U << 11)) acon |= A_mq_11      ;
    if (pcq      & (1U << 11)) acon |= A_pcq_11     ;
    if (irq      & (1U << 11)) acon |= A_irq_11     ;
    if (acq      & (1U << 10)) acon |= A_acq_10     ;
    if (_aluq    & (1U << 10)) acon |= A__aluq_10   ;
    if (_maq     & (1U << 10)) acon |= A__maq_10    ;
    if (maq      & (1U << 10)) acon |= A_maq_10     ;
    if (mq       & (1U << 10)) acon |= A_mq_10      ;
    if (pcq      & (1U << 10)) acon |= A_pcq_10     ;
    if (irq      & (1U << 10)) acon |= A_irq_10     ;
    if (acq      & (1U <<  9)) acon |= A_acq_09     ;
    if (_aluq    & (1U <<  9)) acon |= A__aluq_09   ;
    if (_maq     & (1U <<  9)) acon |= A__maq_09    ;
    if (maq      & (1U <<  9)) acon |= A_maq_09     ;
    if (mq       & (1U <<  9)) acon |= A_mq_09      ;
    if (pcq      & (1U <<  9)) acon |= A_pcq_09     ;
    if (irq      & (1U <<  9)) acon |= A_irq_09     ;
    if (acq      & (1U <<  8)) acon |= A_acq_08     ;
    if (_aluq    & (1U <<  8)) acon |= A__aluq_08   ;
    if (_maq     & (1U <<  8)) acon |= A__maq_08    ;
    if (maq      & (1U <<  8)) acon |= A_maq_08     ;
    if (mq       & (1U <<  8)) acon |= A_mq_08      ;
    if (pcq      & (1U <<  8)) acon |= A_pcq_08     ;
    if (acq      & (1U <<  7)) acon |= A_acq_07     ;
    if (_aluq    & (1U <<  7)) acon |= A__aluq_07   ;
    if (_maq     & (1U <<  7)) acon |= A__maq_07    ;

    if (maq      & (1U <<  7)) bcon |= B_maq_07     ;
    if (mq       & (1U <<  7)) bcon |= B_mq_07      ;
    if (pcq      & (1U <<  7)) bcon |= B_pcq_07     ;
    if (acq      & (1U <<  6)) bcon |= B_acq_06     ;
    if (_aluq    & (1U <<  6)) bcon |= B__aluq_06   ;
    if (_maq     & (1U <<  6)) bcon |= B__maq_06    ;
    if (maq      & (1U <<  6)) bcon |= B_maq_06     ;
    if (mq       & (1U <<  6)) bcon |= B_mq_06      ;
    if (pcq      & (1U <<  6)) bcon |= B_pcq_06     ;
    if (acq      & (1U <<  5)) bcon |= B_acq_05     ;
    if (_aluq    & (1U <<  5)) bcon |= B__aluq_05   ;
    if (_maq     & (1U <<  5)) bcon |= B__maq_05    ;
    if (maq      & (1U <<  5)) bcon |= B_maq_05     ;
    if (mq       & (1U <<  5)) bcon |= B_mq_05      ;
    if (pcq      & (1U <<  5)) bcon |= B_pcq_05     ;
    if (acq      & (1U <<  4)) bcon |= B_acq_04     ;
    if (_aluq    & (1U <<  4)) bcon |= B__aluq_04   ;
    if (_maq     & (1U <<  4)) bcon |= B__maq_04    ;
    if (maq      & (1U <<  4)) bcon |= B_maq_04     ;
    if (mq       & (1U <<  4)) bcon |= B_mq_04      ;
    if (pcq      & (1U <<  4)) bcon |= B_pcq_04     ;
    if (acq      & (1U <<  3)) bcon |= B_acq_03     ;
    if (_aluq    & (1U <<  3)) bcon |= B__aluq_03   ;
    if (_maq     & (1U <<  3)) bcon |= B__maq_03    ;
    if (maq      & (1U <<  3)) bcon |= B_maq_03     ;
    if (mq       & (1U <<  3)) bcon |= B_mq_03      ;
    if (pcq      & (1U <<  3)) bcon |= B_pcq_03     ;

    if (acq      & (1U <<  2)) ccon |= C_acq_02     ;
    if (_aluq    & (1U <<  2)) ccon |= C__aluq_02   ;
    if (_maq     & (1U <<  2)) ccon |= C__maq_02    ;
    if (maq      & (1U <<  2)) ccon |= C_maq_02     ;
    if (mq       & (1U <<  2)) ccon |= C_mq_02      ;
    if (pcq      & (1U <<  2)) ccon |= C_pcq_02     ;
    if (acq      & (1U <<  1)) ccon |= C_acq_01     ;
    if (_aluq    & (1U <<  1)) ccon |= C__aluq_01   ;
    if (_maq     & (1U <<  1)) ccon |= C__maq_01    ;
    if (maq      & (1U <<  1)) ccon |= C_maq_01     ;
    if (mq       & (1U <<  1)) ccon |= C_mq_01      ;
    if (pcq      & (1U <<  1)) ccon |= C_pcq_01     ;
    if (acq      & (1U <<  0)) ccon |= C_acq_00     ;
    if (_aluq    & (1U <<  0)) ccon |= C__aluq_00   ;
    if (_maq     & (1U <<  0)) ccon |= C__maq_00    ;
    if (maq      & (1U <<  0)) ccon |= C_maq_00     ;
    if (mq       & (1U <<  0)) ccon |= C_mq_00      ;
    if (pcq      & (1U <<  0)) ccon |= C_pcq_00     ;

    gcon |= (DATA * G_DATA0) & G_DATA;
}

#define EOL ESC_EREOL "\n"
#define EOP ESC_EREOP "\n"

void ABCD::printfull (int (*xprintf) (void *out, char const *fmt, ...), void *out, bool hasacl, bool hasalu, bool hasma, bool haspc, bool hasrpi, bool hasseq)
{
#define AH(field) (field ? '*' : '-')
#define AL(field) (field ? '-' : '*')
#define BC(field,bitno) (((field >> bitno) & 1) ? '*' : '-')
#define CB(field,bitno) (((field >> bitno) & 1) ? '-' : '*')

    char const *const input   = ESC_YELBG;  // signals input by one or more selected boards that aren't output by any of the selected boards
    char const *const output  = ESC_REDBG;  // signals output by one of the selected boards
    char const *const neutral = ESC_NORMV;  // normal video for signals not used by the selected boards

    char const *const seqout  = hasseq ? output : neutral;

    char const *const acl_alu        = hasacl ? output : (hasalu                          ? input : neutral);   // signals output by acl, input by alu
    char const *const acl_alurpi     = hasacl ? output : (hasalu | hasrpi                 ? input : neutral);   // signals output by acl, input by alu, rpi
    char const *const acl_seq        = hasacl ? output : (hasseq                          ? input : neutral);   // ...
    char const *const alu_acl        = hasalu ? output : (hasacl                          ? input : neutral);
    char const *const alu_aclmapcrpi = hasalu ? output : (hasacl | hasma | haspc | hasrpi ? input : neutral);
    char const *const alu_seq        = hasalu ? output : (hasseq                          ? input : neutral);
    char const *const ma_aclaluseq   = hasma  ? output : (hasacl | hasalu | hasseq        ? input : neutral);
    char const *const pc_alu         = haspc  ? output : (hasalu                          ? input : neutral);
    char const *const rpi_acl        = hasrpi ? output : (hasacl                          ? input : neutral);
    char const *const rpi_aclmapcseq = hasrpi ? output : (hasacl | hasma | haspc | hasseq ? input : neutral);
    char const *const rpi_aluseq     = hasrpi ? output : (hasalu | hasseq                 ? input : neutral);
    char const *const rpi_seq        = hasrpi ? output : (hasseq                          ? input : neutral);
    char const *const seq_acl        = hasseq ? output : (hasacl                          ? input : neutral);
    char const *const seq_aclalu     = hasseq ? output : (hasacl | hasalu                 ? input : neutral);
    char const *const seq_alu        = hasseq ? output : (hasalu                          ? input : neutral);
    char const *const seq_ma         = hasseq ? output : (hasma                           ? input : neutral);
    char const *const seq_pc         = hasseq ? output : (haspc                           ? input : neutral);
    char const *const seq_rpi        = hasseq ? output : (hasrpi                          ? input : neutral);

    xprintf (out, "    A:%08X  B:%08X  C:%08X  D:%08X", acon, bcon, ccon, dcon);
    if (hasrpi) xprintf (out, "  G:%08X", gcon);
    xprintf (out, EOL EOL);
    xprintf (out, "   %s FETCH %c %c  DEFER %c %c %c  EXEC %c %c %c  INTAK %c %s" EOL, seqout, AH(fetch1q), AH(fetch2q), AH(defer1q), AH(defer2q), AH(defer3q), AH(exec1q), AH(exec2q), AH(exec3q), AH(intak1q), neutral);
    xprintf (out, "    %s IR   %c %c %c   %s GRPA1 %c %s IOT2 %c  TAD3 %c %s" EOL, seqout, BC(irq,11), BC(irq,10), BC(irq,9), seq_aclalu, AL(_grpa1q), seq_acl, AH(iot2q), AH(tad3q), neutral);
    xprintf (out, EOL);
    xprintf (out, "    %s MQ   %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o %s MQL %c %s" EOL, rpi_aluseq, BC(mq,11), BC(mq,10), BC(mq,9), BC(mq,8), BC(mq,7), BC(mq,6), BC(mq,5), BC(mq,4), BC(mq,3), BC(mq,2), BC(mq,1), BC(mq,0), mq, rpi_acl, AH(mql), neutral);
    xprintf (out, "   %s ACQ   %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o %s LNQ %c %s" EOL, acl_alu, BC(acq,11), BC(acq,10), BC(acq,9), BC(acq,8), BC(acq,7), BC(acq,6), BC(acq,5), BC(acq,4), BC(acq,3), BC(acq,2), BC(acq,1), BC(acq,0), acq, acl_alurpi, AH(lnq), neutral);
    xprintf (out, "   %s MAQ   %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o %s" EOL, ma_aclaluseq, BC(maq,11), BC(maq,10), BC(maq,9), BC(maq,8), BC(maq,7), BC(maq,6), BC(maq,5), BC(maq,4), BC(maq,3), BC(maq,2), BC(maq,1), BC(maq,0), maq, neutral);
    xprintf (out, "   %s PCQ   %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o %s" EOL, pc_alu, BC(pcq,11), BC(pcq,10), BC(pcq,9), BC(pcq,8), BC(pcq,7), BC(pcq,6), BC(pcq,5), BC(pcq,4), BC(pcq,3), BC(pcq,2), BC(pcq,1), BC(pcq,0), pcq, neutral);
    xprintf (out, EOL);
    xprintf (out, " %s ACQZERO %c  GRPB_SKIP %c %s INTRQ %c  IOSKP %c %s CLOK2 %c  RESET %c %s" EOL, acl_seq, AH(acqzero), AH(grpb_skip), rpi_seq, AH(intrq), AH(ioskp), rpi_aclmapcseq, AH(clok2), AH(reset), neutral);
    xprintf (out, EOL);
    xprintf (out, " %s ALUA_M1 %c  ALUA_MA %c  ALUA_MQ0600 %c  1107 %c  ALUA_PC0600 %c  1107 %c %s" EOL, seq_alu, AL(_alua_m1), AL(_alua_ma), AH(alua_mq0600), AH(alua_mq1107), AH(alua_pc0600), AH(alua_pc1107), neutral);
    xprintf (out, " %s ALU_ADD %c  ALU_AND %c  INC_AXB %c  ALUB_AC %c  ALUB_M1 %c  ALUB_1 %c %s" EOL, seq_alu, AL(_alu_add), AL(_alu_and), AH(inc_axb), AL(_alub_ac), AL(_alub_m1), AH(alub_1), neutral);
    xprintf (out, "   %s ALUQ  %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o %s ALUCOUT %c %s NEWLINK %c %s" EOL, alu_aclmapcrpi, CB(_aluq,11), CB(_aluq,10), CB(_aluq,9), CB(_aluq,8), CB(_aluq,7), CB(_aluq,6), CB(_aluq,5), CB(_aluq,4), CB(_aluq,3), CB(_aluq,2), CB(_aluq,1), CB(_aluq,0), _aluq ^ 07777, alu_seq, AL(_alucout), alu_acl, AL(_newlink), neutral);

    char aluasrc[6] = { 0, 0, '.', 0, 0, 0 };
    if (! _alua_m1)  { aluasrc[0] |= 'M'; aluasrc[1] |= '1'; aluasrc[3] |= 'M'; aluasrc[4] |= '1'; }
    if (! _alua_ma)  { aluasrc[0] |= 'M'; aluasrc[1] |= 'A'; aluasrc[3] |= 'M'; aluasrc[4] |= 'A'; }
    if (alua_mq1107) { aluasrc[0] |= 'M'; aluasrc[1] |= 'Q';                                       }
    if (alua_mq0600) {                                       aluasrc[3] |= 'M'; aluasrc[4] |= 'Q'; }
    if (alua_pc1107) { aluasrc[0] |= 'P'; aluasrc[1] |= 'C';                                       }
    if (alua_pc0600) {                                       aluasrc[3] |= 'P'; aluasrc[4] |= 'C'; }
    if (aluasrc[0] == 0) { aluasrc[0] |= '0'; aluasrc[1] |= '0'; }
    if (aluasrc[3] == 0) { aluasrc[3] |= '0'; aluasrc[4] |= '0'; }

    char alubsrc[3] = { 0, 0, 0 };
    if (! _alub_ac)  { alubsrc[0] |= 'A'; alubsrc[1] |= 'C'; }
    if (! _alub_m1)  { alubsrc[0] |= 'M'; alubsrc[1] |= '1'; }
    if (   alub_1)   { alubsrc[0] |= '0'; alubsrc[1] |= '1'; }
    if (alubsrc[0] == 0) { alubsrc[0] |= '0'; alubsrc[1] |= '0'; }

    uint16_t _aluashouldbe = 07777 ^ ((_alua_m1 ? 0 : 07777) | (_alua_ma ? 0 : maq)   | (alua_mq1107 ? mq & 07600 : 0) | (alua_mq0600 ? mq & 00177 : 0) | (alua_pc1107 ? pcq & 07600 : 0) | (alua_pc0600 ? pcq & 00177 : 0));
    uint16_t _alubshouldbe = 07777 ^ ((_alub_ac ? 0 : acq)   | (_alub_m1 ? 0 : 07777) | (alub_1 ? 00001 : 0));

    xprintf (out, "                                    %o.%04o  =  %s/%04o  ", ! _alucout, _aluq ^ 07777, aluasrc, _aluashouldbe ^ 07777);
    if (! _alu_add) xprintf (out, "%c", '+');
    if (! _alu_and) xprintf (out, "%c", '&');
    if (! _grpa1q)  xprintf (out, "%c", '^');
    xprintf (out, "  %s/%04o", alubsrc, _alubshouldbe ^ 07777);
    if (! _grpa1q)  xprintf (out, "  +  %o", inc_axb);
    xprintf (out, EOL);

    xprintf (out, EOL);
    xprintf (out, " %s DFRM %c  JUMP %c  INTAK %c  IOINST %c  MREAD %c  MWRITE %c %s" EOL, seq_rpi, AL(_dfrm), AL(_jump), AL(_intak), AH(ioinst), AL(_mread), AL(_mwrite), neutral);
    xprintf (out, EOL);
    xprintf (out, " %s AC_ALUQ %c  AC_SC %c  LN_WRT %c %s MA_ALUQ %c %s PC_ALUQ %c  PC_INC %c %s" EOL, seq_acl, AL(_ac_aluq), AL(_ac_sc), AL(_ln_wrt), seq_ma, AL(_ma_aluq), seq_pc, AL(_pc_aluq), AL(_pc_inc), neutral);
    if (hasrpi) {

        // DENA set : G_LINK,G_DATA signals being output by RPI board via GPIO connector to the RasPI
        // QENA set : G_LINK,G_DATA signals being input by RPI board via GPIO connector from the RasPI

        char const *const __data = DENA ? output : (QENA ? input : neutral);

        // JUMP,IOIN,... signals are always output by RPI board via GPIO connector to the RasPI
        // CLOCK,RESET,... signals are always input by RPI board via GPIO connector from the RasPI

        xprintf (out, EOL);
        xprintf (out, "   %s JUMP %c  IOIN %c  DFRM %c  READ %c  WRITE %c  IAK %c %s" EOL, output, AH(JUMP), AH(IOIN), AH(DFRM), AH(READ), AH(WRITE), AH(IAK), neutral);
        xprintf (out, "   %s DATA  %c %c %c  %c %c %c  %c %c %c  %c %c %c  %04o  LINK %c %s" EOL, __data, BC(DATA,11), BC(DATA,10), BC(DATA,9), BC(DATA,8), BC(DATA,7), BC(DATA,6), BC(DATA,5), BC(DATA,4), BC(DATA,3), BC(DATA,2), BC(DATA,1), BC(DATA,0), DATA, AH(LINK), neutral);
        xprintf (out, "   %s CLOCK %c  RESET %c  IOS %c  QENA %c  IRQ %c  DENA %c %s" EOL, input, AH(CLOCK), AH(RESET), AH(IOS), AH(QENA), AH(IRQ), AH(DENA), neutral);
    }

#undef AH
#undef AL
#undef BC
}
