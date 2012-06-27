/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 *
 *      TMS320C6xx - VLIW (very long instruction word) architecture
 *
 */

#include "tms6.hpp"

//--------------------------------------------------------------------------
struct tmsinsn_t
{
  uchar itype;
  uchar src1;
  uchar src2;
  uchar dst;
};

// operand types
#define t_none           0
#define t_sint           1
#define t_xsint          2
#define t_uint           3
#define t_xuint          4
#define t_slong          5
#define t_xslong         6
#define t_ulong          7
#define t_xulong         8
#define t_scst5          9
#define t_ucst5         10
#define t_slsb16        11
#define t_xslsb16       12
#define t_ulsb16        13
#define t_xulsb16       14
#define t_smsb16        15
#define t_xsmsb16       16
#define t_umsb16        17
#define t_xumsb16       18
#define t_irp           19
#define t_cregr         20
#define t_cregw         21
#define t_ucst1         22

//--------------------------------------------------------------------------
static void swap_op1_and_op2(void)
{
  if ( (cmd.cflags & aux_pseudo) == 0 )
  {
    op_t tmp = cmd.Op1;
    cmd.Op1 = cmd.Op2;
    cmd.Op2 = tmp;
    cmd.Op1.n = 0;
    cmd.Op2.n = 1;
  }
}

//--------------------------------------------------------------------------
inline bool second_unit(void)
{
  return cmd.funit == FU_L2
      || cmd.funit == FU_S2
      || cmd.funit == FU_M2
      || cmd.funit == FU_D2;
}

//--------------------------------------------------------------------------
static int make_reg(long v, bool isother)
{
  if ( second_unit() == isother )
    return int(v) & 0xF;
  else
    return (int(v) & 0xF) + 0x10;
}

//--------------------------------------------------------------------------
static int make_op(op_t &x, uchar optype, sval_t v, bool isother)
{
  switch ( optype )
  {
    case t_none:
      break;
    case t_sint:
    case t_uint:
      isother = 0;
    case t_xsint:
    case t_xuint:
      x.type = o_reg;
      x.dtyp = dt_dword;
      x.reg = make_reg(v,isother);
      break;
    case t_slsb16 :
    case t_ulsb16 :
    case t_smsb16 :
    case t_umsb16 :
      isother = 0;
    case t_xslsb16:
    case t_xulsb16:
    case t_xsmsb16:
    case t_xumsb16:
      x.type = o_reg;
      x.dtyp = dt_word;
      x.reg = make_reg(v,isother);
      break;
    case t_slong:
    case t_ulong:
      isother = 0;
    case t_xslong:
    case t_xulong:
      x.type = o_regpair;
      x.dtyp = dt_fword;        // this is not correct...
      x.reg = make_reg(v,isother);
      break;
    case t_ucst1:
      if ( v != 0 && v != 1 ) return 0;
      /* fall thru */
    case t_scst5:
      if ( v & 0x10 ) v |= ~0x1FL;              // extend sign
      /* fall thru */
    case t_ucst5:
      x.type = o_imm;
      x.dtyp = dt_dword;
      x.value = v;
      break;
    case t_irp:
      x.type = o_reg;
      x.dtyp = dt_word;
           if ( v == 6 ) x.reg = rIRP;
      else if ( v == 7 ) x.reg = rNRP;
      else return 0;
      break;
    case t_cregr:
    case t_cregw:
      {
        static const uchar cregs[] =
        {
          rAMR, rCSR,  rISR,   rICR,
          rIER, rISTP, rIRP,   rNRP,
          0,    0,     rACR,   rADR,
          0,    0,     0,      0,
          rPCE1,0,     rFADCR, rFAUCR,
          rFMCR
        };
        if ( v >= qnumber(cregs) ) return 0;
        x.reg  = cregs[int(v)];
        if ( x.reg == 0 ) return 0;
        if ( x.reg == rISR && optype == t_cregr ) x.reg = rIFR;
        x.type = o_reg;
        x.dtyp = dt_dword;
      }
      break;
    default:
      error("tms interr: optype");
  }
  return 1;
}

//--------------------------------------------------------------------------
static void make_pseudo(void)
{
  switch ( cmd.itype )
  {
    case TMS6_add:
      if ( cmd.Op1.type == o_imm && cmd.Op1.value == 0 )
      {
        cmd.itype = TMS6_mv;
SHIFT_OPS:
        cmd.Op1 = cmd.Op2;
        cmd.Op2 = cmd.Op3;
        cmd.Op1.n = 0;
        cmd.Op2.n = 1;
        cmd.Op3.type = o_void;
        cmd.cflags |= aux_pseudo;
      }
      break;
    case TMS6_sub:
      if ( cmd.Op1.type == o_imm && cmd.Op1.value == 0 )
      {
        cmd.itype = TMS6_neg;
        goto SHIFT_OPS;
      }
      if ( cmd.Op1.type == o_reg
        && cmd.Op2.type == o_reg
        && cmd.Op3.type == o_reg
        && cmd.Op1.reg  == cmd.Op2.reg
        && cmd.Op1.reg  == cmd.Op3.reg )
      {
          cmd.itype = TMS6_zero;
          cmd.Op2.type = o_void;
          cmd.Op3.type = o_void;
          cmd.cflags |= aux_pseudo;
      }
      break;
    case TMS6_xor:
      if ( cmd.Op1.type == o_imm && cmd.Op1.value == uval_t(-1) )
      {
        cmd.itype = TMS6_not;
        goto SHIFT_OPS;
      }
      break;
  }
}

//--------------------------------------------------------------------------
static int table_insns(ulong code, tmsinsn_t *insn, bool isother)
{
// +------------------------------------------...
// |31    29|28|27    23|22   18|17        13|...
// |  creg  |z |  dst   |  src2 |  src1/cst  |...
// +------------------------------------------...

  if ( insn->itype == TMS6_null ) return 0;
  cmd.itype = insn->itype;
  if ( isother ) cmd.cflags |= aux_xp;  // xpath is used
  op_t *xptr = &cmd.Op1;
  if ( !make_op(*xptr,insn->src1, (code >> 13) & 0x1F, isother) ) return 0;
  if ( xptr->type != o_void ) xptr++;
  if ( !make_op(*xptr,insn->src2, (code >> 18) & 0x1F, isother) ) return 0;
  if ( xptr->type != o_void ) xptr++;
  if ( !make_op(*xptr,insn->dst,  (code >> 23) & 0x1F, isother) ) return 0;
  make_pseudo();
  return cmd.size;
}

//--------------------------------------------------------------------------
//      L UNIT OPERATIONS
//--------------------------------------------------------------------------
static tmsinsn_t lops[128] =
{
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0000000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0000001
  { TMS6_add,   t_scst5,        t_xsint,        t_sint          }, // 0000010
  { TMS6_add,   t_sint,         t_xsint,        t_sint          }, // 0000011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0000100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0000101
  { TMS6_sub,   t_scst5,        t_xsint,        t_sint          }, // 0000110
  { TMS6_sub,   t_sint,         t_xsint,        t_sint          }, // 0000111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0001101
  { TMS6_ssub,  t_scst5,        t_xsint,        t_sint          }, // 0001110
  { TMS6_ssub,  t_sint,         t_xsint,        t_sint          }, // 0001111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0010000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0010001
  { TMS6_sadd,  t_scst5,        t_xsint,        t_sint          }, // 0010010
  { TMS6_sadd,  t_sint,         t_xsint,        t_sint          }, // 0010011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0010100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0010101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0010110
  { TMS6_sub,   t_xsint,        t_sint,         t_sint          }, // 0010111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011001
  { TMS6_abs,   t_none,         t_xsint,        t_sint          }, // 0011010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0011110
  { TMS6_ssub,  t_xsint,        t_sint,         t_sint          }, // 0011111
  { TMS6_add,   t_scst5,        t_slong,        t_slong         }, // 0100000
  { TMS6_add,   t_xsint,        t_slong,        t_slong         }, // 0100001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0100010
  { TMS6_add,   t_sint,         t_xsint,        t_slong         }, // 0100011
  { TMS6_sub,   t_scst5,        t_slong,        t_slong         }, // 0100100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0100101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0100110
  { TMS6_sub,   t_sint,         t_xsint,        t_slong         }, // 0100111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0101000
  { TMS6_addu,  t_xuint,        t_ulong,        t_ulong         }, // 0101001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0101010
  { TMS6_addu,  t_uint,         t_xuint,        t_ulong         }, // 0101011
  { TMS6_ssub,  t_scst5,        t_slong,        t_slong         }, // 0101100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0101101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0101110
  { TMS6_subu,  t_uint,         t_xuint,        t_ulong         }, // 0101111
  { TMS6_sadd,  t_scst5,        t_slong,        t_slong         }, // 0110000
  { TMS6_sadd,  t_xsint,        t_slong,        t_slong         }, // 0110001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0110010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0110011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0110100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0110101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0110110
  { TMS6_sub,   t_xsint,        t_sint,         t_slong         }, // 0110111
  { TMS6_abs,   t_none,         t_slong,        t_slong         }, // 0111000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 0111110
  { TMS6_subu,  t_xuint,        t_uint,         t_ulong         }, // 0111111
  { TMS6_sat,   t_none,         t_slong,        t_sint          }, // 1000000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1000001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1000010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1000011
  { TMS6_cmpgt, t_scst5,        t_slong,        t_uint          }, // 1000100
  { TMS6_cmpgt, t_xsint,        t_slong,        t_uint          }, // 1000101
  { TMS6_cmpgt, t_scst5,        t_xsint,        t_uint          }, // 1000110
  { TMS6_cmpgt, t_sint,         t_xsint,        t_uint          }, // 1000111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1001000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1001001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1001010
  { TMS6_subc,  t_uint,         t_xuint,        t_uint          }, // 1001011
  { TMS6_cmpgtu,t_scst5,        t_ulong,        t_uint          }, // 1001100
  { TMS6_cmpgtu,t_xuint,        t_ulong,        t_uint          }, // 1001101
  { TMS6_cmpgtu,t_scst5,        t_xuint,        t_uint          }, // 1001110
  { TMS6_cmpgtu,t_uint,         t_xuint,        t_uint          }, // 1001111
  { TMS6_cmpeq, t_scst5,        t_slong,        t_uint          }, // 1010000
  { TMS6_cmpeq, t_xsint,        t_slong,        t_uint          }, // 1010001
  { TMS6_cmpeq, t_scst5,        t_xsint,        t_uint          }, // 1010010
  { TMS6_cmpeq, t_sint,         t_xsint,        t_uint          }, // 1010011
  { TMS6_cmplt, t_scst5,        t_slong,        t_uint          }, // 1010100
  { TMS6_cmplt, t_xsint,        t_slong,        t_uint          }, // 1010101
  { TMS6_cmplt, t_scst5,        t_xsint,        t_uint          }, // 1010110
  { TMS6_cmplt, t_sint,         t_xsint,        t_uint          }, // 1010111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1011000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1011001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1011010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1011011
  { TMS6_cmpltu,t_scst5,        t_ulong,        t_uint          }, // 1011100
  { TMS6_cmpltu,t_xuint,        t_ulong,        t_uint          }, // 1011101
  { TMS6_cmpltu,t_scst5,        t_xuint,        t_uint          }, // 1011110
  { TMS6_cmpltu,t_uint,         t_xuint,        t_uint          }, // 1011111
  { TMS6_norm,  t_none,         t_slong,        t_uint          }, // 1100000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100010
  { TMS6_norm,  t_none,         t_xsint,        t_uint          }, // 1100011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1100111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1101000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1101001
  { TMS6_lmbd,  t_ucst1,        t_xuint,        t_uint          }, // 1101010
  { TMS6_lmbd,  t_uint,         t_xuint,        t_uint          }, // 1101011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1101100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1101101
  { TMS6_xor,   t_scst5,        t_xuint,        t_uint          }, // 1101110
  { TMS6_xor,   t_uint,         t_xuint,        t_uint          }, // 1101111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1110111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1111000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1111001
  { TMS6_and,   t_scst5,        t_xuint,        t_uint          }, // 1111010
  { TMS6_and,   t_uint,         t_xuint,        t_uint          }, // 1111011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1111100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 1111101
  { TMS6_or,    t_scst5,        t_xuint,        t_uint          }, // 1111110
  { TMS6_or,    t_uint,         t_xuint,        t_uint          }, // 1111111
};

static int l_ops(ulong code)
{
// +--------------------------------------------------------------+
// |31    29|28|27    23|22   18|17        13|12|11    5|4|3|2|1|0|
// |  creg  |z |  dst   |  src2 |  src1/cst  |x |   op  |1|1|0|s|p|
// +--------------------------------------------------------------+

  return table_insns(code, lops + (int(code >> 5) & 0x7F), (code & BIT12) != 0);
}

//--------------------------------------------------------------------------
//      M UNIT OPERATIONS
//--------------------------------------------------------------------------
static tmsinsn_t mops[32] =
{
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00000
  { TMS6_mpyh,  t_smsb16,       t_xsmsb16,      t_sint          }, // 00001
  { TMS6_smpyh, t_smsb16,       t_xsmsb16,      t_sint          }, // 00010
  { TMS6_mpyhsu,t_smsb16,       t_xumsb16,      t_sint          }, // 00011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00100
  { TMS6_mpyhus,t_umsb16,       t_xsmsb16,      t_sint          }, // 00101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00110
  { TMS6_mpyhu, t_umsb16,       t_xumsb16,      t_uint          }, // 00111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01000
  { TMS6_mpyhl, t_smsb16,       t_xslsb16,      t_sint          }, // 01001
  { TMS6_smpyhl,t_smsb16,       t_xslsb16,      t_sint          }, // 01010
  { TMS6_mpyhslu,t_smsb16,      t_xulsb16,      t_sint          }, // 01011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01100
  { TMS6_mpyhuls,t_umsb16,      t_xslsb16,      t_sint          }, // 01101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01110
  { TMS6_mpyhlu,t_umsb16,       t_xulsb16,      t_uint          }, // 01111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10000
  { TMS6_mpylh, t_slsb16,       t_xsmsb16,      t_sint          }, // 10001
  { TMS6_smpylh,t_slsb16,       t_xsmsb16,      t_sint          }, // 10010
  { TMS6_mpylshu,t_slsb16,      t_xumsb16,      t_sint          }, // 10011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10100
  { TMS6_mpyluhs,t_ulsb16,      t_xsmsb16,      t_sint          }, // 10101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10110
  { TMS6_mpylhu,t_ulsb16,       t_xumsb16,      t_uint          }, // 10111
  { TMS6_mpy,   t_scst5,        t_xslsb16,      t_sint          }, // 11000
  { TMS6_mpy,   t_slsb16,       t_xslsb16,      t_sint          }, // 11001
  { TMS6_smpy,  t_slsb16,       t_xslsb16,      t_sint          }, // 11010
  { TMS6_mpysu, t_slsb16,       t_xulsb16,      t_sint          }, // 11011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 11100
  { TMS6_mpyus, t_ulsb16,       t_xslsb16,      t_sint          }, // 11101
  { TMS6_mpysu, t_scst5,        t_xulsb16,      t_sint          }, // 11110
  { TMS6_mpyu,  t_ulsb16,       t_xulsb16,      t_uint          }, // 11111
};

static int m_ops(ulong code)
{
// +------------------------------------------------------------------+
// |31    29|28|27    23|22   18|17        13|12|11    7|6|5|4|3|2|1|0|
// |  creg  |z |  dst   |  src2 |  src1/cst  |x |   op  |0|0|0|0|0|s|p|
// +------------------------------------------------------------------+

  return table_insns(code, mops + (int(code >> 7) & 0x1F), (code & BIT12) != 0);
}

//--------------------------------------------------------------------------
//      D UNIT OPERATIONS
//--------------------------------------------------------------------------
static tmsinsn_t dops1[] =
{
  { TMS6_add,   t_sint,         t_sint,         t_sint          }, // 010000
  { TMS6_sub,   t_sint,         t_sint,         t_sint          }, // 010001
  { TMS6_add,   t_ucst5,        t_sint,         t_sint          }, // 010010
  { TMS6_sub,   t_ucst5,        t_sint,         t_sint          }, // 010011
};

static tmsinsn_t dops2[] =
{
  { TMS6_addab, t_sint,         t_sint,         t_sint          }, // 110000
  { TMS6_subab, t_sint,         t_sint,         t_sint          }, // 110001
  { TMS6_addab, t_ucst5,        t_sint,         t_sint          }, // 110010
  { TMS6_subab, t_ucst5,        t_sint,         t_sint          }, // 110011
  { TMS6_addah, t_sint,         t_sint,         t_sint          }, // 110100
  { TMS6_subah, t_sint,         t_sint,         t_sint          }, // 110101
  { TMS6_addah, t_ucst5,        t_sint,         t_sint          }, // 110110
  { TMS6_subah, t_ucst5,        t_sint,         t_sint          }, // 110111
  { TMS6_addaw, t_sint,         t_sint,         t_sint          }, // 111000
  { TMS6_subaw, t_sint,         t_sint,         t_sint          }, // 111001
  { TMS6_addaw, t_ucst5,        t_sint,         t_sint          }, // 111010
  { TMS6_subaw, t_ucst5,        t_sint,         t_sint          }, // 111011
};

static int d_ops(ulong code)
{
// +--------------------------------------------------------------+
// |31    29|28|27    23|22   18|17        13|12   7|6|5|4|3|2|1|0|
// |  creg  |z |  dst   |  src2 |  src1/cst  |  op  |1|0|0|0|0|s|p|
// +--------------------------------------------------------------+

  int opcode = int(code >> 7) & 0x3F;
  int res = 0;
  if ( opcode >= 0x10 && opcode <= 0x13 )
    res = table_insns(code, dops1 + (opcode - 0x10), 0);
  if ( opcode >= 0x30 && opcode <= 0x3B )
    res = table_insns(code, dops2 + (opcode - 0x30), 0);
  if ( res != 0 ) swap_op1_and_op2();
  return res;
}

//--------------------------------------------------------------------------
//      LOAD/STORE WITH 15-BIT OFFSET (ON D2 UNIT)
//--------------------------------------------------------------------------
static uchar ld_itypes[] =
{
  TMS6_ldhu,                    // 000
  TMS6_ldbu,                    // 001
  TMS6_ldb,                     // 010
  TMS6_stb,                     // 011
  TMS6_ldh,                     // 100
  TMS6_sth,                     // 101
  TMS6_ldw,                     // 110
  TMS6_stw,                     // 111
};

static uchar ld_dttypes[] =
{
  dt_word,                      // 000
  dt_byte,                      // 001
  dt_byte,                      // 010
  dt_byte,                      // 011
  dt_word,                      // 100
  dt_word,                      // 101
  dt_dword,                     // 110
  dt_dword,                     // 111
};

static uchar ld_shifts[] = { 1, 0, 0, 0, 1, 1, 2, 2 };

static int ld_common(ulong code)
{
  int idx = int(code >> 4) & 7;
  cmd.itype = ld_itypes[idx];
  if ( cmd.itype == TMS6_null ) return -1;
  cmd.Op2.type = o_reg;
  cmd.Op2.dtyp = dt_dword;
  cmd.Op2.reg  = (code >> 23) & 0x1F;
  if ( cmd.Op2.reg & 0x10 ) return -1;          // 30.11.01 register file is specified with BIT1
  if ( code & BIT1 ) cmd.Op2.reg += 0x10;
  cmd.Op1.dtyp = ld_dttypes[idx];
  return ld_shifts[idx];
}

static int ld15(ulong code)
{
  int shift = ld_common(code);
  if ( shift == -1 ) return 0;
  cmd.Op1.type = o_displ;
  cmd.Op1.mode = 5;             // *+R[cst]
  cmd.Op1.reg  = (code & BIT7) ? rB15 : rB14;
  cmd.Op1.addr = (code >> 8) & 0x7FFF;
  int is_store = ( cmd.itype == TMS6_stb
                || cmd.itype == TMS6_sth
                || cmd.itype == TMS6_stw );
  if ( isOff(get_flags_novalue(cmd.ea), is_store) )
    cmd.Op1.addr <<= shift;
  if ( is_store ) swap_op1_and_op2();
  return cmd.size;
}

//--------------------------------------------------------------------------
//      LOAD/STORE BASER+OFFSETR/CONST (ON D UNITS)
//--------------------------------------------------------------------------
static int ldbase(ulong code)
{
// +------------------------------------------------------------------------+
// |31    29|28|27   23|22     18|17           13|12   9|8|7|6     4|3|2|1|0|
// |  creg  |z |  dst  |  baseR  | offsetR/ucst5 | mode |r|y| ld/st |0|1|s|p|
// +------------------------------------------------------------------------+

  int shift = ld_common(code);
  if ( shift == -1 ) return 0;
  cmd.Op1.mode = (code >> 9) & 0xF;
  int is_store = ( cmd.itype == TMS6_stb
                || cmd.itype == TMS6_sth
                || cmd.itype == TMS6_stw );
  switch ( cmd.Op1.mode )
  {
    case 0x02:  // 0010
    case 0x03:  // 0011
    case 0x06:  // 0110
    case 0x07:  // 0111
      return 0;
    case 0x00:  // 0000 *-R[cst]
    case 0x01:  // 0001 *+R[cst]
    case 0x08:  // 1000 *--R[cst]
    case 0x09:  // 1001 *++R[cst]
    case 0x0A:  // 1010 *R--[cst]
    case 0x0B:  // 1011 *R++[cst]
      cmd.Op1.type = o_displ;
      cmd.Op1.addr = (code >> 13) & 0x1F;
      if ( isOff(uFlag,is_store) ) cmd.Op1.addr <<= shift;
      break;
    case 0x04:  // 0100 *-Rb[Ro]
    case 0x05:  // 0101 *+Rb[Ro]
    case 0x0C:  // 1100 *--Rb[Ro]
    case 0x0D:  // 1101 *++Rb[Ro]
    case 0x0E:  // 1110 *Rb--[Ro]
    case 0x0F:  // 1111 *Rb++[Ro]
      cmd.Op1.type   = o_phrase;
      cmd.Op1.secreg = make_reg((code >> 13) & 0x1F,0);
      break;
  }
  cmd.Op1.reg = make_reg((code >> 18) & 0x1F,0);
  if ( is_store ) swap_op1_and_op2();
  return cmd.size;
}

//--------------------------------------------------------------------------
//      S UNIT OPERATIONS
//--------------------------------------------------------------------------
static tmsinsn_t sops[64] =
{
  { TMS6_null,  t_none,         t_none,         t_none          }, // 000000
  { TMS6_add2,  t_sint,         t_xsint,        t_sint          }, // 000001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 000010
  { TMS6_b,     t_irp,          t_none,         t_none          }, // 000011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 000100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 000101
  { TMS6_add,   t_scst5,        t_xsint,        t_sint          }, // 000110
  { TMS6_add,   t_sint,         t_xsint,        t_sint          }, // 000111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 001000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 001001
  { TMS6_xor,   t_scst5,        t_xuint,        t_uint          }, // 001010
  { TMS6_xor,   t_uint,         t_xuint,        t_uint          }, // 001011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 001100
  { TMS6_b,     t_none,         t_xuint,        t_none          }, // 001101
  { TMS6_mvc,   t_none,         t_xuint,        t_cregw         }, // 001110
  { TMS6_mvc,   t_none,         t_cregr,        t_uint          }, // 001111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 010000
  { TMS6_sub2,  t_sint,         t_xsint,        t_sint          }, // 010001
  { TMS6_shl,   t_ucst5,        t_xsint,        t_slong         }, // 010010
  { TMS6_shl,   t_uint,         t_xsint,        t_slong         }, // 010011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 010100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 010101
  { TMS6_sub,   t_scst5,        t_xsint,        t_sint          }, // 010110
  { TMS6_sub,   t_sint,         t_xsint,        t_sint          }, // 010111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 011000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 011001
  { TMS6_or,    t_scst5,        t_xuint,        t_uint          }, // 011010
  { TMS6_or,    t_uint,         t_xuint,        t_uint          }, // 011011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 011100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 011101
  { TMS6_and,   t_scst5,        t_xuint,        t_uint          }, // 011110
  { TMS6_and,   t_uint,         t_xuint,        t_uint          }, // 011111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 100000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 100001
  { TMS6_sshl,  t_ucst5,        t_xsint,        t_sint          }, // 100010
  { TMS6_sshl,  t_uint,         t_xsint,        t_sint          }, // 100011
  { TMS6_shru,  t_ucst5,        t_ulong,        t_ulong         }, // 100100
  { TMS6_shru,  t_uint,         t_ulong,        t_ulong         }, // 100101
  { TMS6_shru,  t_ucst5,        t_xuint,        t_uint          }, // 100110
  { TMS6_shru,  t_uint,         t_xuint,        t_uint          }, // 100111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101010
  { TMS6_extu,  t_uint,         t_xuint,        t_uint          }, // 101011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 101110
  { TMS6_ext,   t_uint,         t_xsint,        t_sint          }, // 101111
  { TMS6_shl,   t_ucst5,        t_slong,        t_slong         }, // 110000
  { TMS6_shl,   t_uint,         t_slong,        t_slong         }, // 110001
  { TMS6_shl,   t_ucst5,        t_xsint,        t_sint          }, // 110010
  { TMS6_shl,   t_uint,         t_xsint,        t_sint          }, // 110011
  { TMS6_shr,   t_ucst5,        t_slong,        t_slong         }, // 110100
  { TMS6_shr,   t_uint,         t_slong,        t_slong         }, // 110101
  { TMS6_shr,   t_ucst5,        t_xsint,        t_sint          }, // 110110
  { TMS6_shr,   t_uint,         t_xsint,        t_sint          }, // 110111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111010
  { TMS6_set,   t_uint,         t_xuint,        t_uint          }, // 111011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 111110
  { TMS6_clr,   t_uint,         t_xuint,        t_uint          }, // 111111
};

static int s_ops(ulong code)
{
// +----------------------------------------------------------------+
// |31    29|28|27    23|22   18|17        13|12|11    6|5|4|3|2|1|0|
// |  creg  |z |  dst   |  src2 |  src1/cst  |x |   op  |1|0|0|0|s|p|
// +----------------------------------------------------------------+

  if ( !table_insns(code, sops + (int(code >> 6) & 0x3F), (code & BIT12) != 0) )
    return 0;
  switch ( cmd.itype )
  {
    case TMS6_mvc:
      cmd.cflags &= ~aux_xp;            // XPATH should not be displayed
                                        // (assembler does not like it)
      /* fall thru */
    case TMS6_b:
      if ( cmd.funit != FU_S2 ) return 0;
      break;
    case TMS6_extu:
    case TMS6_ext:
    case TMS6_set:
    case TMS6_clr:
      cmd.cflags &= ~aux_xp;            // XPATH should not be displayed
                                        // (assembler does not like it)
      /* fall thru */
    case TMS6_shl:
    case TMS6_sshl:
    case TMS6_shr:
    case TMS6_shru:
      swap_op1_and_op2();
      break;
  }
  return cmd.size;
}

//--------------------------------------------------------------------------
//      ADDK ON S UNITS
//--------------------------------------------------------------------------
static int addk(ulong code)
{
// +-----------------------------------------------------+
// |31    29|28|27    23|22               7|6|5|4|3|2|1|0|
// |  creg  |z |  dst   |        cst       |1|0|1|0|0|s|p|
// +-----------------------------------------------------+

  cmd.itype = TMS6_addk;
  cmd.Op1.type = o_imm;
  cmd.Op1.dtyp = dt_word;
  cmd.Op1.value = short(code >> 7);
  cmd.Op2.type = o_reg;
  cmd.Op2.dtyp = dt_dword;
  cmd.Op2.reg  = make_reg((code >> 23) & 0x1F,0);
  return cmd.size;
}

//--------------------------------------------------------------------------
//      FIELD OPERATIONS (IMMEDIATE FORMS) ON S UNITS
//--------------------------------------------------------------------------
static int field_ops(ulong code)
{
// +---------------------------------------------------------------+
// |31    29|28|27    23|22   18|17    13|12     8|7  6|5|4|3|2|1|0|
// |  creg  |z |  dst   |  src2 |  csta  |  cstb  | op |0|0|1|0|s|p|
// +---------------------------------------------------------------+
  static uchar itypes[] =
  {
    TMS6_extu,  // 00
    TMS6_ext,   // 01
    TMS6_set,   // 10
    TMS6_clr,   // 11
  };
  cmd.itype = itypes[int(code >> 6) & 3];
  cmd.Op1.type  = o_imm;
  cmd.Op1.value = (code >> 13) & 0x1F;
  cmd.Op2.type  = o_imm;
  cmd.Op2.value = (code >>  8) & 0x1F;
  cmd.Op3.type  = o_reg;
  cmd.Op3.reg   = make_reg((code >> 23) & 0x1F,0);
  cmd.Op1.src2  = make_reg((code >> 18) & 0x1F,0);
  cmd.cflags   |= aux_src2;
  return cmd.size;
}

//--------------------------------------------------------------------------
//      MVK AND MVKH ON S UNITS
//--------------------------------------------------------------------------
static int mvk(ulong code)
{
// +-----------------------------------------------------+
// |31    29|28|27    23|22               7|6|5|4|3|2|1|0|
// |  creg  |z |  dst   |        cst       |x|1|0|1|0|s|p|
// +-----------------------------------------------------+

  cmd.itype     = (code & BIT6) ? TMS6_mvkh : TMS6_mvk;
  cmd.Op1.type  = o_imm;
  cmd.Op1.dtyp  = dt_word;
  cmd.Op1.value = (code >> 7) & 0xFFFF;
  cmd.Op2.type  = o_reg;
  cmd.Op2.dtyp  = dt_word;
  cmd.Op2.reg   = make_reg((code >> 23) & 0x1F,0);
  return cmd.size;
}

//--------------------------------------------------------------------------
//      BCOND DISP ON S UNITS
//--------------------------------------------------------------------------
static int bcond(ulong code)
{
// +--------------------------------------------+
// |31    29|28|27               7|6|5|4|3|2|1|0|
// |  creg  |z |        cst       |0|0|1|0|0|s|p|
// +--------------------------------------------+

  sval_t cst = (code >> 7) & 0x1FFFFFL;
  if ( cst & 0x100000L ) cst |= ~0x1FFFFFL;     // extend sign
  cst <<= 2;
  cmd.itype = TMS6_b;
  cmd.Op1.type = o_near;
  cmd.Op1.addr = (cmd.ip & ~0x1F) + cst;
  return cmd.size;
}

//--------------------------------------------------------------------------
int ana(void)
{
  if ( cmd.ip & 3 ) return 0;           // alignment error

  ulong code = ua_next_long();

  if ( code & BIT0 )
  {
    cmd.cflags |= aux_para;     // parallel execution with the next insn
    // unfortunately, we can not check the packet boundaries
    // for object files - the offset might be shifted!
    // if ( ((cmd.ip + cmd.size) & 0x1F) == 0 ) return 0;  // can't be - packet boundary!
  }

  cmd.cond = (code >> 28);
  switch ( cmd.cond )
  {
    case 0x0: // 0000 unconditional
    case 0x2: // 0010 B0
    case 0x3: // 0011 !B0
    case 0x4: // 0100 B1
    case 0x5: // 0101 !B1
    case 0x6: // 0110 B2
    case 0x7: // 0111 !B2
    case 0x8: // 1000 A1
    case 0x9: // 1001 !A1
    case 0xA: // 1010 A2
    case 0xB: // 1011 !A2
      break;
    case 0x1: // 0001 reserved
    case 0xC: // 1100 reserved
    case 0xD: // 1101 reserved
    case 0xE: // 1110 reserved
    case 0xF: // 1111 reserved
      return 0;
  }

  switch ( (code >> 2) & 0x1F )
  {
//
//      Operations on L units
//
    case 0x06: // 00110
    case 0x0E: // 01110
    case 0x16: // 10110
    case 0x1E: // 11110
      cmd.funit = (code & BIT1) ? FU_L2 : FU_L1;
      return l_ops(code);
//
//      Operations on M units
//
    case 0x00: // 00000
      if ( (code & 0x3FFFCL) == 0x1E000L )
      {
        cmd.itype = TMS6_idle;
        return cmd.size;
      }
      if ( (code & 0x21FFEL) == 0 )
      {
        cmd.Op1.type = o_imm;
        cmd.Op1.dtyp = dt_dword;
        cmd.Op1.value = ((code >> 13) & 0xF) + 1;
        if ( cmd.Op1.value > 9 ) return 0;
        if ( cmd.Op1.value == 1 ) cmd.Op1.clr_showed();
        cmd.itype = TMS6_nop;
        return cmd.size;
      }
      cmd.funit = (code & BIT1) ? FU_M2 : FU_M1;
      return m_ops(code);
//
//      Operations on D units
//
    case 0x10: // 10000
      cmd.funit = (code & BIT1) ? FU_D2 : FU_D1;
      return d_ops(code);
//
//      Load/store with 15-bit offset (on D2 unit)
//
    case 0x03: // 00011
    case 0x07: // 00111
    case 0x0B: // 01011
    case 0x0F: // 01111
    case 0x13: // 10011
    case 0x17: // 10111
    case 0x1B: // 11011
    case 0x1F: // 11111
      cmd.funit = FU_D2;
      return ld15(code);
//
//      Load/store baseR+offsetR/const (on D units)
//
    case 0x01: // 00001
    case 0x05: // 00101
    case 0x09: // 01001
    case 0x0D: // 01101
    case 0x11: // 10001
    case 0x15: // 10101
    case 0x19: // 11001
    case 0x1D: // 11101
      cmd.funit = (code & BIT7) ? FU_D2 : FU_D1;
      return ldbase(code);
//
//      Operations on S units
//
    case 0x08: // 01000
    case 0x18: // 11000
      cmd.funit = (code & BIT1) ? FU_S2 : FU_S1;
      return s_ops(code);
//
//      ADDK on S units
//
    case 0x14: // 10100
      cmd.funit = (code & BIT1) ? FU_S2 : FU_S1;
      return addk(code);
//
//      Field operations (immediate forms) on S units
//
    case 0x02: // 00010
    case 0x12: // 10010
      cmd.funit = (code & BIT1) ? FU_S2 : FU_S1;
      return field_ops(code);
//
//      MVK and MVKH on S units
//
    case 0x0A: // 01010
    case 0x1A: // 11010
      cmd.funit = (code & BIT1) ? FU_S2 : FU_S1;
      return mvk(code);
//
//      Bcond disp on S units
//
    case 0x04: // 00100
      cmd.funit = (code & BIT1) ? FU_S2 : FU_S1;
      return bcond(code);
//
//      Undefined opcodes
//
    case 0x0C: // 01100
    case 0x1C: // 11100
      break;
  }
  return 0;
}

