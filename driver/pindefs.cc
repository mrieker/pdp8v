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

// tables relating A,B,C,D connector pins to corresponding module variable name

#include "pindefs.h"

PinDefs const apindefs[] = {
    { A_acq_11,      "acq[11]"      },
    { A__aluq_11,   "_aluq[11]"     },
    { A__maq_11,    "_maq[11]"      },
    { A_maq_11,      "maq[11]"      },
    { A_mq_11,       "mq[11]"       },
    { A_pcq_11,      "pcq[11]"      },
    { A_irq_11,      "irq[11]"      },
    { A_acq_10,      "acq[10]"      },
    { A__aluq_10,   "_aluq[10]"     },
    { A__maq_10,    "_maq[10]"      },
    { A_maq_10,      "maq[10]"      },
    { A_mq_10,       "mq[10]"       },
    { A_pcq_10,      "pcq[10]"      },
    { A_irq_10,      "irq[10]"      },
    { A_acq_09,      "acq[9]"       },
    { A__aluq_09,   "_aluq[9]"      },
    { A__maq_09,    "_maq[9]"       },
    { A_maq_09,      "maq[9]"       },
    { A_mq_09,       "mq[9]"        },
    { A_pcq_09,      "pcq[9]"       },
    { A_irq_09,      "irq[9]"       },
    { A_acq_08,      "acq[8]"       },
    { A__aluq_08,   "_aluq[8]"      },
    { A__maq_08,    "_maq[8]"       },
    { A_maq_08,      "maq[8]"       },
    { A_mq_08,       "mq[8]"        },
    { A_pcq_08,      "pcq[8]"       },
    { A_acq_07,      "acq[7]"       },
    { A__aluq_07,   "_aluq[7]"      },
    { A__maq_07,    "_maq[7]"       },
    { 0, NULL } };

PinDefs const bpindefs[] = {
    { B_maq_07,      "maq[7]"       },
    { B_mq_07,       "mq[7]"        },
    { B_pcq_07,      "pcq[7]"       },
    { B_acq_06,      "acq[6]"       },
    { B__aluq_06,   "_aluq[6]"      },
    { B__maq_06,    "_maq[6]"       },
    { B_maq_06,      "maq[6]"       },
    { B_mq_06,       "mq[6]"        },
    { B_pcq_06,      "pcq[6]"       },
    { B__jump,      "_jump"         },
    { B_acq_05,      "acq[5]"       },
    { B__aluq_05,   "_aluq[5]"      },
    { B__maq_05,    "_maq[5]"       },
    { B_maq_05,      "maq[5]"       },
    { B_mq_05,       "mq[5]"        },
    { B_pcq_05,      "pcq[5]"       },
    { B__alub_m1,   "_alub_m1"      },
    { B_acq_04,      "acq[4]"       },
    { B__aluq_04,   "_aluq[4]"      },
    { B__maq_04,    "_maq[4]"       },
    { B_maq_04,      "maq[4]"       },
    { B_mq_04,       "mq[4]"        },
    { B_pcq_04,      "pcq[4]"       },
    { B_acqzero,     "acqzero"      },
    { B_acq_03,      "acq[3]"       },
    { B__aluq_03,   "_aluq[3]"      },
    { B__maq_03,    "_maq[3]"       },
    { B_maq_03,      "maq[3]"       },
    { B_mq_03,       "mq[3]"        },
    { B_pcq_03,      "pcq[3]"       },
    { 0, NULL } };

PinDefs const cpindefs[] = {
    { C_acq_02,      "acq[2]"       },
    { C__aluq_02,   "_aluq[2]"      },
    { C__maq_02,    "_maq[2]"       },
    { C_maq_02,      "maq[2]"       },
    { C_mq_02,       "mq[2]"        },
    { C_pcq_02,      "pcq[2]"       },
    { C__ac_sc,     "_ac_sc"        },
    { C_acq_01,      "acq[1]"       },
    { C__aluq_01,   "_aluq[1]"      },
    { C__maq_01,    "_maq[1]"       },
    { C_maq_01,      "maq[1]"       },
    { C_mq_01,       "mq[1]"        },
    { C_pcq_01,      "pcq[1]"       },
    { C_intak1q,     "intak1q"      },
    { C_acq_00,      "acq[0]"       },
    { C__aluq_00,   "_aluq[0]"      },
    { C__maq_00,    "_maq[0]"       },
    { C_maq_00,      "maq[0]"       },
    { C_mq_00,       "mq[0]"        },
    { C_pcq_00,      "pcq[0]"       },
    { C_fetch1q,     "fetch1q"      },
    { C__ac_aluq,   "_ac_aluq"      },
    { C__alu_add,   "_alu_add"      },
    { C__alu_and,   "_alu_and"      },
    { C__alua_m1,   "_alua_m1"      },
    { C__alucout,   "_alucout"      },
    { C__alua_ma,   "_alua_ma"      },
    { C_alua_mq0600, "alua_mq0600"  },
    { C_alua_mq1107, "alua_mq1107"  },
    { C_alua_pc0600, "alua_pc0600"  },
    { C_alua_pc1107, "alua_pc1107"  },
    { 0, NULL } };

PinDefs const dpindefs[] = {
    { D_alub_1,      "alub_1"       },
    { D__alub_ac,   "_alub_ac"      },
    { D_clok2,       "clok2"        },
    { D_fetch2q,     "fetch2q"      },
    { D__grpa1q,    "_grpa1q"       },
    { D_defer1q,     "defer1q"      },
    { D_defer2q,     "defer2q"      },
    { D_defer3q,     "defer3q"      },
    { D_exec1q,      "exec1q"       },
    { D_grpb_skip,   "grpb_skip"    },
    { D_exec2q,      "exec2q"       },
    { D__dfrm,      "_dfrm"         },
    { D_inc_axb,     "inc_axb"      },
    { D__intak,     "_intak"        },
    { D_intrq,       "intrq"        },
    { D_exec3q,      "exec3q"       },
    { D_ioinst,      "ioinst"       },
    { D_ioskp,       "ioskp"        },
    { D_iot2q,       "iot2q"        },
    { D__ln_wrt,    "_ln_wrt"       },
    { D__lnq,       "_lnq"          },
    { D_lnq,         "lnq"          },
    { D__ma_aluq,   "_ma_aluq"      },
    { D_mql,         "mql"          },
    { D__mread,     "_mread"        },
    { D__mwrite,    "_mwrite"       },
    { D__pc_aluq,   "_pc_aluq"      },
    { D__pc_inc,    "_pc_inc"       },
    { D_reset,       "reset"        },
    { D__newlink,   "_newlink"      },
    { D_tad3q,       "tad3q"        },
    { 0, NULL } };

PinDefs const gpindefs[] = {
    { G_CLOCK,       "CLOCK"    },
    { G_RESET,       "RESET"    },
    { G_DATA0 <<  0, "DATA[0]"  },
    { G_DATA0 <<  1, "DATA[1]"  },
    { G_DATA0 <<  2, "DATA[2]"  },
    { G_DATA0 <<  3, "DATA[3]"  },
    { G_DATA0 <<  4, "DATA[4]"  },
    { G_DATA0 <<  5, "DATA[5]"  },
    { G_DATA0 <<  6, "DATA[6]"  },
    { G_DATA0 <<  7, "DATA[7]"  },
    { G_DATA0 <<  8, "DATA[8]"  },
    { G_DATA0 <<  9, "DATA[9]"  },
    { G_DATA0 << 10, "DATA[10]" },
    { G_DATA0 << 11, "DATA[11]" },
    { G_LINK,        "LINK"     },
    { G_IOS,         "IOS"      },
    { G_QENA,        "QENA"     },
    { G_IRQ,         "IRQ"      },
    { G_DENA,        "DENA"     },
    { G_JUMP,        "JUMP"     },
    { G_IOIN,        "IOIN"     },
    { G_DFRM,        "DFRM"     },
    { G_READ,        "READ"     },
    { G_WRITE,       "WRITE"    },
    { G_IAK,         "IAK"      },
    { 0, NULL } };

PinDefs const *const pindefss[5] = { apindefs, bpindefs, cpindefs, dpindefs, gpindefs };

// convert pin bits to string
// each pin set gets corresponding string, separated by a space
std::string pinstring (uint32_t pins, PinDefs const *defs)
{
    std::string str;
    bool first = true;
    for (int i = 32; -- i >= 0;) {
        uint32_t m = 1U << i;
        if (m & pins) {
            for (PinDefs const *p = defs; p->pinmask != 0; p ++) {
                if (p->pinmask == m) {
                    if (! first) str.push_back (' ');
                    char temp[10];
                    sprintf (temp, "<PIN%02d>", i + 1);
                    str.append (temp);
                    str.append (p->varname);
                    first = false;
                }
            }
        }
    }
    return str;
}
