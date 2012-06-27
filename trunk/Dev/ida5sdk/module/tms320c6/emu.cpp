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

static bool flow;
//------------------------------------------------------------------------
static void doImmdValue(int n)
{
  doImmd(cmd.ea);
  if ( isDefArg(uFlag,n) ) return;
  switch ( cmd.itype )
  {
    case TMS6_and:              // Rd = Op1 & Op2
    case TMS6_xor:              // Rd = Op1 ^ Op2
    case TMS6_or:               // Rd = Op2 | Op1
    case TMS6_set:              // Rd = Op1 & ~Op2
    case TMS6_clr:              // Rd = Op1 & ~Op2
    case TMS6_ext:              // Rd = Op1 & ~Op2
    case TMS6_extu:             // Rd = Op1 & ~Op2
      op_num(cmd.ea,n);
  }
}

//----------------------------------------------------------------------
static void TouchArg(op_t &x,int isload)
{
  switch ( x.type )
  {
    case o_regpair:
    case o_reg:
    case o_phrase:
      break;
    case o_imm:
      if ( !isload ) goto badTouch;
      /* no break */
    case o_displ:
      doImmdValue(x.n);
      if ( isOff(uFlag, x.n) ) ua_add_off_drefs(x, dr_O);
      break;
    case o_near:
      ua_add_cref(x.offb,toEA(cmd.cs,x.addr),fl_JN);
      break;
    default:
badTouch:
      warning("%a: %s,%d: bad optype %d", cmd.ea, cmd.get_canon_mnem(), x.n, x.type);
      break;
  }
}

//----------------------------------------------------------------------
ea_t find_first_insn_in_packet(ea_t ea)
{
  while ( (ea & 0x1F) != 0 )
  {
    ea_t ea2 = prev_not_tail(ea);
    if ( ea2 == BADADDR
      || !isCode(get_flags_novalue(ea2))
      || (get_long(ea2) & BIT0) == 0 ) break;
    ea = ea2;
  }
  return ea;
}

//----------------------------------------------------------------------
inline bool is_nop(ulong code) { return (code & 0x21FFEL) == 0; }

static int get_delay(ulong code)
{
  if ( is_nop(code) )                           // NOP
    return int((code >> 13) & 0xF) + 1;
  return 1;
}

//----------------------------------------------------------------------
struct call_info_t
{
  ulong next;
  ea_t mvk;
  ea_t mvkh;
  int reg;
  int call_is_present(void) { return mvk != BADADDR && mvkh != BADADDR; }
  void test(ea_t ea, ulong code);
};

//----------------------------------------------------------------------
inline ushort get_mvk_op(ulong code) { return ushort(code >> 7); }

void call_info_t::test(ea_t ea, ulong code)
{
  // unconditional MVK
  if ( (code & 0xF000007CL) == 0x28 && mvk == BADADDR )
  {
    int mvk_reg = int(code >> 23) & 0x1F;
    if ( code & BIT1 ) mvk_reg += 0x10;
    if ( (reg == -1 || reg == mvk_reg) && ushort(next) == get_mvk_op(code) )
    {
      reg  = mvk_reg;
      mvk  = ea;
    }
  }
  else if ( (code & 0xF000007CL) == 0x68 && mvkh == BADADDR )
  { // unconditional MVKH
    int mvk_reg = int(code >> 23) & 0x1F;
    if ( code & BIT1 ) mvk_reg += 0x10;
    if ( (reg == -1 || reg == mvk_reg) && ushort(next>>16) == get_mvk_op(code) )
    {
      reg  = mvk_reg;
      mvkh = ea;
    }
  }
}

//----------------------------------------------------------------------
static int calc_packet_delay(ea_t ea, call_info_t *ci)
{
  int delay = 1;
  while ( 1 )
  {
    ulong code = get_long(ea);
    int d2 = get_delay(code);
    if ( d2 > delay ) delay = d2;
    ci->test(ea, code);
    if ( (code & BIT0) == 0 ) break;
    ea += 4;
    if ( !isCode(get_flags_novalue(ea)) ) break;
  }
  return delay;
}

//----------------------------------------------------------------------
static ea_t find_prev_packet(ea_t ea)
{
  ea_t res = BADADDR;
  while ( 1 )
  {
    ea_t ea2 = prev_not_tail(res!=BADADDR ? res : ea);
    if ( ea2 == BADADDR
      || !isCode(get_flags_novalue(ea2)) ) break;
    res = ea2;
    if ( (get_long(ea2) & BIT0) == 0 ) break;
  }
  return res;
}

//----------------------------------------------------------------------
static ea_t get_branch_ea(ea_t ea)
{
  while ( 1 )
  {
    ulong code = get_long(ea);
    if ( (code >> 28) == cAL )
    {
      switch ( (code >> 2) & 0x1F )
      {
        case 0x04:                      // bcond()
          return ea;
        case 0x08:                      // S unit
        case 0x18:
          {
            int opcode = int(code >> 6) & 0x3F;
            if ( opcode == 3 || opcode == 0xD ) return ea;
          }
          break;
      }
    }
    if ( (code & BIT0) == 0 ) break;
    ea += 4;
    if ( !isCode(get_flags_novalue(ea)) ) break;
  }
  return BADADDR;
}

//----------------------------------------------------------------------
int emu(void)
{
  ulong Feature = cmd.get_canon_feature();
  flow = ((Feature & CF_STOP) == 0);

  if ( segtype(cmd.ea) == SEG_XTRN )
  {
    flow = 0;
  }
  else if ( (cmd.cflags & aux_para) == 0 )           // the last instruction of packet
  {
    insn_t saved = cmd;
    ea_t ea = find_first_insn_in_packet(cmd.ea);
    int delay = 0;
    call_info_t ci;
    ci.next = cmd.ea + cmd.size;
    ci.mvk = ci.mvkh = BADADDR;
    ci.reg = rB3;
    while ( 1 )
    {
      if ( hasRef(get_flags_novalue(ea)) ) break;
      delay += calc_packet_delay(ea, &ci);
      if ( delay > 5 ) break;
      ea = find_prev_packet(ea);
      if ( ea == BADADDR ) break;
      ea = find_first_insn_in_packet(ea);
      ea_t brea;
      if ( delay == 5 && (brea=get_branch_ea(ea)) != BADADDR )
      {
        calc_packet_delay(ea, &ci);      // just to test for MVK/MVKH
        int iscall = ci.call_is_present();
        ua_ana0(brea);
        if ( cmd.Op1.type == o_near )
        {
          ea_t target = toEA(cmd.cs,cmd.Op1.addr);
          if ( iscall )
          {
            ua_add_cref(cmd.Op1.offb, target, fl_CN);
            if ( !func_does_return(target) )
              flow = false;
          }
          else
          {
            flow = false;
            target++;
          }
          tnode.altset(saved.ea, target);
        }
        else
        {
          tnode.altset(saved.ea, iscall ? 2 : 1);
          if ( !iscall ) flow = 0;
        }
        if ( iscall )
        {
          if ( !isOff0(get_flags_novalue(ci.mvk))   )
            op_offset(ci.mvk, 0, REF_LOW16, ci.next, cmd.cs, 0);
          if ( !isOff0(get_flags_novalue(ci.mvkh)) )
            op_offset(ci.mvkh, 0, REF_HIGH16, ci.next, cmd.cs, 0);
        }
        break;
      }
    }
    cmd = saved;
  }

  if ( Feature & CF_USE1 ) TouchArg(cmd.Op1, 1);
  if ( Feature & CF_USE2 ) TouchArg(cmd.Op2, 1);
  if ( Feature & CF_USE3 ) TouchArg(cmd.Op3, 1);

  if ( Feature & CF_CHG1 ) TouchArg(cmd.Op1, 0);
  if ( Feature & CF_CHG2 ) TouchArg(cmd.Op2, 0);
  if ( Feature & CF_CHG3 ) TouchArg(cmd.Op3, 0);

  if ( flow ) ua_add_cref(0,cmd.ea+cmd.size,fl_F);
  return 1;
}

//----------------------------------------------------------------------
int is_align_insn(ea_t ea)
{
  ua_ana0(ea);
  switch ( cmd.itype )
  {
    case TMS6_mv:
      if ( cmd.Op1.reg == cmd.Op2.reg ) break;
    default:
      return 0;
    case TMS6_nop:
      break;
  }
  return cmd.size;
}
