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

#ifndef _PINDEFS_H
#define _PINDEFS_H

#include <string>

// - pin assignments from ../modules/cons.inc
#define A_acq_11      (1U << ( 2 - 1))
#define A__aluq_11    (1U << ( 3 - 1))
#define A__maq_11     (1U << ( 4 - 1))
#define A_maq_11      (1U << ( 5 - 1))
#define A_mq_11       (1U << ( 6 - 1))
#define A_pcq_11      (1U << ( 7 - 1))
#define A_irq_11      (1U << ( 8 - 1))
#define A_acq_10      (1U << ( 9 - 1))
#define A__aluq_10    (1U << (10 - 1))
#define A__maq_10     (1U << (11 - 1))
#define A_maq_10      (1U << (12 - 1))
#define A_mq_10       (1U << (13 - 1))
#define A_pcq_10      (1U << (14 - 1))
#define A_irq_10      (1U << (15 - 1))
#define A_acq_09      (1U << (16 - 1))
#define A__aluq_09    (1U << (17 - 1))
#define A__maq_09     (1U << (18 - 1))
#define A_maq_09      (1U << (19 - 1))
#define A_mq_09       (1U << (20 - 1))
#define A_pcq_09      (1U << (21 - 1))
#define A_irq_09      (1U << (22 - 1))
#define A_acq_08      (1U << (23 - 1))
#define A__aluq_08    (1U << (24 - 1))
#define A__maq_08     (1U << (25 - 1))
#define A_maq_08      (1U << (26 - 1))
#define A_mq_08       (1U << (27 - 1))
#define A_pcq_08      (1U << (28 - 1))
#define A_acq_07      (1U << (30 - 1))
#define A__aluq_07    (1U << (31 - 1))
#define A__maq_07     (1U << (32 - 1))

#define B_maq_07      (1U << ( 2 - 1))
#define B_mq_07       (1U << ( 3 - 1))
#define B_pcq_07      (1U << ( 4 - 1))
#define B_acq_06      (1U << ( 6 - 1))
#define B__aluq_06    (1U << ( 7 - 1))
#define B__maq_06     (1U << ( 8 - 1))
#define B_maq_06      (1U << ( 9 - 1))
#define B_mq_06       (1U << (10 - 1))
#define B_pcq_06      (1U << (11 - 1))
#define B__jump       (1U << (12 - 1))
#define B_acq_05      (1U << (13 - 1))
#define B__aluq_05    (1U << (14 - 1))
#define B__maq_05     (1U << (15 - 1))
#define B_maq_05      (1U << (16 - 1))
#define B_mq_05       (1U << (17 - 1))
#define B_pcq_05      (1U << (18 - 1))
#define B__alub_m1    (1U << (19 - 1))
#define B_acq_04      (1U << (20 - 1))
#define B__aluq_04    (1U << (21 - 1))
#define B__maq_04     (1U << (22 - 1))
#define B_maq_04      (1U << (23 - 1))
#define B_mq_04       (1U << (24 - 1))
#define B_pcq_04      (1U << (25 - 1))
#define B_acqzero     (1U << (26 - 1))
#define B_acq_03      (1U << (27 - 1))
#define B__aluq_03    (1U << (28 - 1))
#define B__maq_03     (1U << (29 - 1))
#define B_maq_03      (1U << (30 - 1))
#define B_mq_03       (1U << (31 - 1))
#define B_pcq_03      (1U << (32 - 1))

#define C_acq_02      (1U << ( 2 - 1))
#define C__aluq_02    (1U << ( 3 - 1))
#define C__maq_02     (1U << ( 4 - 1))
#define C_maq_02      (1U << ( 5 - 1))
#define C_mq_02       (1U << ( 6 - 1))
#define C_pcq_02      (1U << ( 7 - 1))
#define C__ac_sc      (1U << ( 8 - 1))
#define C_acq_01      (1U << ( 9 - 1))
#define C__aluq_01    (1U << (10 - 1))
#define C__maq_01     (1U << (11 - 1))
#define C_maq_01      (1U << (12 - 1))
#define C_mq_01       (1U << (13 - 1))
#define C_pcq_01      (1U << (14 - 1))
#define C_intak1q     (1U << (15 - 1))
#define C_acq_00      (1U << (16 - 1))
#define C__aluq_00    (1U << (17 - 1))
#define C__maq_00     (1U << (18 - 1))
#define C_maq_00      (1U << (19 - 1))
#define C_mq_00       (1U << (20 - 1))
#define C_pcq_00      (1U << (21 - 1))
#define C_fetch1q     (1U << (22 - 1))
#define C__ac_aluq    (1U << (23 - 1))
#define C__alu_add    (1U << (24 - 1))
#define C__alu_and    (1U << (25 - 1))
#define C__alua_m1    (1U << (26 - 1))
#define C__alucout    (1U << (27 - 1))
#define C__alua_ma    (1U << (28 - 1))
#define C_alua_mq0600 (1U << (29 - 1))
#define C_alua_mq1107 (1U << (30 - 1))
#define C_alua_pc0600 (1U << (31 - 1))
#define C_alua_pc1107 (1U << (32 - 1))

#define D_alub_1      (1U << ( 2 - 1))
#define D__alub_ac    (1U << ( 3 - 1))
#define D_clok2       (1U << ( 4 - 1))
#define D_fetch2q     (1U << ( 5 - 1))
#define D__grpa1q     (1U << ( 6 - 1))
#define D_defer1q     (1U << ( 7 - 1))
#define D_defer2q     (1U << ( 8 - 1))
#define D_defer3q     (1U << ( 9 - 1))
#define D_exec1q      (1U << (10 - 1))
#define D_grpb_skip   (1U << (11 - 1))
#define D_exec2q      (1U << (12 - 1))
#define D__dfrm       (1U << (13 - 1))
#define D_inc_axb     (1U << (14 - 1))
#define D__intak      (1U << (15 - 1))
#define D_intrq       (1U << (16 - 1))
#define D_exec3q      (1U << (17 - 1))
#define D_ioinst      (1U << (18 - 1))
#define D_ioskp       (1U << (19 - 1))
#define D_iot2q       (1U << (20 - 1))
#define D__ln_wrt     (1U << (21 - 1))
#define D__lnq        (1U << (22 - 1))
#define D_lnq         (1U << (23 - 1))
#define D__ma_aluq    (1U << (24 - 1))
#define D_mql         (1U << (25 - 1))
#define D__mread      (1U << (26 - 1))
#define D__mwrite     (1U << (27 - 1))
#define D__pc_aluq    (1U << (28 - 1))
#define D__pc_inc     (1U << (29 - 1))
#define D_reset       (1U << (30 - 1))
#define D__newlink    (1U << (31 - 1))
#define D_tad3q       (1U << (32 - 1))

#define acl_ains  (A__aluq_11 | A__maq_11 | A_maq_11 | A__aluq_10 | A__maq_10 | A_maq_10 | A__aluq_09 | A__maq_09 | A_maq_09 | A__aluq_08 | A__maq_08 | A_maq_08 | A__aluq_07 | A__maq_07)
#define acl_bins  (B_maq_07 | B__aluq_06 | B__maq_06 | B_maq_06 | B__aluq_05 | B__maq_05 | B_maq_05 | B__aluq_04 | B__maq_04 | B_maq_04 | B__aluq_03 | B__maq_03 | B_maq_03)
#define acl_cins  (C__aluq_02 | C__maq_02 | C_maq_02 | C__ac_sc | C__aluq_01 | C__maq_01 | C_maq_01 | C__aluq_00 | C__maq_00 | C_maq_00 | C__ac_aluq)
#define acl_dins  (D_clok2 | D__grpa1q | D_iot2q | D__ln_wrt | D_mql | D_reset | D__newlink | D_tad3q)
#define acl_aouts (A_acq_11 | A_acq_10 | A_acq_09 | A_acq_08 | A_acq_07)
#define acl_bouts (B_acq_06 | B_acq_05 | B_acq_04 | B_acqzero | B_acq_03)
#define acl_couts (C_acq_02 | C_acq_01 | C_acq_00)
#define acl_douts (D_grpb_skip | D__lnq | D_lnq)

#define alu_ains  (A__maq_07 | A__maq_08 | A__maq_09 | A__maq_10 | A__maq_11 | A_acq_07 | A_acq_08 | A_acq_09 | A_acq_10 | A_acq_11 | A_maq_08 | A_maq_09 | A_maq_10 | A_maq_11 | A_mq_08 | A_mq_09 | A_mq_10 | A_mq_11 | A_pcq_08 | A_pcq_09 | A_pcq_10 | A_pcq_11)
#define alu_bins  (B__alub_m1 | B__maq_03 | B__maq_04 | B__maq_05 | B__maq_06 | B_acq_03 | B_acq_04 | B_acq_05 | B_acq_06 | B_maq_03 | B_maq_04 | B_maq_05 | B_maq_06 | B_maq_07 | B_mq_03 | B_mq_04 | B_mq_05 | B_mq_06 | B_mq_07 | B_pcq_03 | B_pcq_04 | B_pcq_05 | B_pcq_06 | B_pcq_07)
#define alu_cins  (C__alu_add | C__alu_and | C__alua_m1 | C__alua_ma | C__maq_00 | C__maq_01 | C__maq_02 | C_acq_00 | C_acq_01 | C_acq_02 | C_alua_mq0600 | C_alua_mq1107 | C_alua_pc0600 | C_alua_pc1107 | C_maq_00 | C_maq_01 | C_maq_02 | C_mq_00 | C_mq_01 | C_mq_02 | C_pcq_00 | C_pcq_01 | C_pcq_02)
#define alu_dins  (D__alub_ac | D__grpa1q | D__lnq | D_alub_1 | D_inc_axb | D_lnq)
#define alu_aouts (A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07)
#define alu_bouts (B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03)
#define alu_couts (C__aluq_02 | C__aluq_01 | C__aluq_00 | C__alucout)
#define alu_douts (D__newlink)

#define ma_ains   (A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07)
#define ma_bins   (B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03)
#define ma_cins   (C__aluq_02 | C__aluq_01 | C__aluq_00)
#define ma_dins   (D_clok2 | D__ma_aluq | D_reset)
#define ma_aouts  (A__maq_11 | A_maq_11 | A__maq_10 | A_maq_10 | A__maq_09 | A_maq_09 | A__maq_08 | A_maq_08 | A__maq_07)
#define ma_bouts  (B_maq_07 | B__maq_06 | B_maq_06 | B__maq_05 | B_maq_05 | B__maq_04 | B_maq_04 | B__maq_03 | B_maq_03)
#define ma_couts  (C__maq_02 | C_maq_02 | C__maq_01 | C_maq_01 | C__maq_00 | C_maq_00)
#define ma_douts  (0)

#define pc_ains   (A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07)
#define pc_bins   (B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03)
#define pc_cins   (C__aluq_02 | C__aluq_01 | C__aluq_00)
#define pc_dins   (D_clok2 | D__pc_aluq | D__pc_inc | D_reset)
#define pc_aouts  (A_pcq_11 | A_pcq_10 | A_pcq_09 | A_pcq_08)
#define pc_bouts  (B_pcq_07 | B_pcq_06 | B_pcq_05 | B_pcq_04 | B_pcq_03)
#define pc_couts  (C_pcq_02 | C_pcq_01 | C_pcq_00)
#define pc_douts  (0)

#define rpi_ains (A_acq_11 | A__aluq_11 | A_maq_11 | A_pcq_11 | A_irq_11 | A_acq_10 | A__aluq_10 | A_maq_10 | A_pcq_10 | A_irq_10 | A_acq_09 | A__aluq_09 | A_maq_09 | A_pcq_09 | A_irq_09 | A_acq_08 | A__aluq_08 | A_maq_08 | A_pcq_08 | A_acq_07 | A__aluq_07)
#define rpi_bins (B_maq_07 | B_pcq_07 | B_acq_06 | B__aluq_06 | B_maq_06 | B_pcq_06 | B__jump | B_acq_05 | B__aluq_05 | B_maq_05 | B_pcq_05 | B_acq_04 | B__aluq_04 | B_maq_04 | B_pcq_04 | B_acq_03 | B__aluq_03 | B_maq_03 | B_pcq_03)
#define rpi_cins (C_acq_02 | C__aluq_02 | C_maq_02 | C_pcq_02 | C_acq_01 | C__aluq_01 | C_maq_01 | C_pcq_01 | C_intak1q | C_acq_00 | C__aluq_00 | C_maq_00 | C_pcq_00 | C_fetch1q)
#define rpi_dins (D_fetch2q | D_defer1q | D_defer2q | D_defer3q | D_exec1q | D_exec2q | D__dfrm | D__intak | D_exec3q | D_ioinst | D__lnq | D_lnq | D__mread | D__mwrite)
#define rpi_aouts (A_mq_08 | A_mq_09 | A_mq_10 | A_mq_11)
#define rpi_bouts (B_mq_03 | B_mq_04 | B_mq_05 | B_mq_06 | B_mq_07)
#define rpi_couts (C_mq_00 | C_mq_01 | C_mq_02)
#define rpi_douts (D_clok2 | D_intrq | D_ioskp | D_mql | D_reset)

#define seq_ains  (A__maq_07 | A__maq_08 | A__maq_09 | A__maq_10 | A__maq_11 | A_maq_08 | A_maq_09 | A_maq_10 | A_maq_11 | A_mq_08 | A_mq_09 | A_mq_10 | A_mq_11)
#define seq_bins  (B__maq_03 | B__maq_04 | B__maq_05 | B__maq_06 | B_acqzero | B_maq_03 | B_maq_04 | B_maq_05 | B_maq_06 | B_maq_07 | B_mq_03 | B_mq_04 | B_mq_05 | B_mq_06 | B_mq_07)
#define seq_cins  (C__alucout | C__maq_00 | C__maq_01 | C__maq_02 | C_maq_00 | C_maq_01 | C_maq_02 | C_mq_00 | C_mq_01 | C_mq_02)
#define seq_dins  (D_clok2 | D_grpb_skip | D_intrq | D_ioskp | D_reset)
#define seq_aouts (A_irq_11 | A_irq_10 | A_irq_09)
#define seq_bouts (B__jump | B__alub_m1)
#define seq_couts (C__ac_aluq | C__ac_sc | C__alu_add | C__alu_and | C__alua_m1 | C__alua_ma | C_alua_mq0600 | C_alua_mq1107 | C_alua_pc0600 | C_alua_pc1107 | C_fetch1q | C_intak1q)
#define seq_douts (D__alub_ac | D__dfrm | D__grpa1q | D__intak | D__ln_wrt | D__ma_aluq | D__mread | D__mwrite | D__pc_aluq | D__pc_inc | D_alub_1 | D_defer1q | D_defer2q | D_defer3q | D_exec1q | D_exec2q | D_exec3q | D_fetch2q | D_inc_axb | D_ioinst | D_iot2q | D_tad3q)

#include "miscdefs.h"

struct PinDefs { uint32_t pinmask; char const *varname; };

extern PinDefs const apindefs[];
extern PinDefs const bpindefs[];
extern PinDefs const cpindefs[];
extern PinDefs const dpindefs[];
extern PinDefs const gpindefs[];
extern PinDefs const *const pindefss[5];

std::string pinstring (uint32_t pins, PinDefs const *defs);

#endif
