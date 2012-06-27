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

//----------------------------------------------------------------------
inline void outreg(int rn)
{
  out_register(ph.regNames[rn]);
}

//----------------------------------------------------------------------
static void out_pre_mode(int mode)
{
  out_symbol('*');
  switch ( mode )
  {
    case 0x08:  // 1000 *--R[cst]
    case 0x0C:  // 1100 *--Rb[Ro]
      out_symbol('-');
    case 0x00:  // 0000 *-R[cst]
    case 0x04:  // 0100 *-Rb[Ro]
      out_symbol('-');
      break;
    case 0x09:  // 1001 *++R[cst]
    case 0x0D:  // 1101 *++Rb[Ro]
      out_symbol('+');
      out_symbol('+');
      break;
    case 0x01:  // 0001 *+R[cst]
    case 0x05:  // 0101 *+Rb[Ro]
//      out_symbol('+');
      break;
    case 0x0A:  // 1010 *R--[cst]
    case 0x0B:  // 1011 *R++[cst]
    case 0x0E:  // 1110 *Rb--[Ro]
    case 0x0F:  // 1111 *Rb++[Ro]
      break;
  }
}

//----------------------------------------------------------------------
static void out_post_mode(int mode)
{
  switch ( mode )
  {
    case 0x08:  // 1000 *--R[cst]
    case 0x0C:  // 1100 *--Rb[Ro]
    case 0x00:  // 0000 *-R[cst]
    case 0x04:  // 0100 *-Rb[Ro]
    case 0x09:  // 1001 *++R[cst]
    case 0x0D:  // 1101 *++Rb[Ro]
    case 0x01:  // 0001 *+R[cst]
    case 0x05:  // 0101 *+Rb[Ro]
      break;
    case 0x0A:  // 1010 *R--[cst]
    case 0x0E:  // 1110 *Rb--[Ro]
      out_symbol('-');
      out_symbol('-');
      break;
    case 0x0B:  // 1011 *R++[cst]
    case 0x0F:  // 1111 *Rb++[Ro]
      out_symbol('+');
      out_symbol('+');
      break;
  }
}

//----------------------------------------------------------------------
inline int def_delta(int mode)
{
  if ( mode == 1 ) return 0;
  return 1;
}

//----------------------------------------------------------------------
static void outval(op_t &x,uchar flags)
{
  OutValue(x, flags);
}

//----------------------------------------------------------------------
bool outop(op_t &x)
{
  switch ( x.type )
  {
    case o_void:
      return 0;

    case o_reg:
      outreg(x.reg);
      break;

    case o_regpair:
      outreg(x.reg+1);
      out_symbol(':');
      outreg(x.reg);
      break;

    case o_imm:
      outval(x,OOFS_IFSIGN|OOFW_IMM|(x.dtyp == dt_word ? 0 : OOF_SIGNED));
      break;

    case o_near:
    case o_mem:
      {
        ea_t ea = toEA(cmd.cs,x.addr);
        if ( !out_name_expr(x, ea, x.addr) )
        {
          out_tagon(COLOR_ERROR);
          OutLong(x.addr,16);
          out_tagoff(COLOR_ERROR);
          QueueMark(Q_noName,cmd.ea);
        }
      }
      break;

    case o_phrase:
      out_pre_mode(x.mode);
      outreg(x.reg);
      out_post_mode(x.mode);
      out_symbol('[');
      outreg(x.secreg);
      out_symbol(']');
      break;

    case o_displ:
      out_pre_mode(x.mode);
      outreg(x.reg);
      out_post_mode(x.mode);
      if ( x.addr != def_delta(x.mode) || isOff(uFlag,x.n) )
      {
        if ( isOff(uFlag,x.n) )
        {
          out_symbol('(');
          outval(x,OOF_ADDR|OOFS_IFSIGN|OOFW_IMM|OOF_SIGNED|OOFW_32);
          out_symbol(')');
        }
        else
        {
          out_symbol('[');
          outval(x,OOF_ADDR|OOFS_IFSIGN|OOFW_IMM|OOF_SIGNED|OOFW_32);
          out_symbol(']');
        }
      }
      break;

    default:
      warning("out: %a: bad optype", cmd.ea, x.type);
      break;
  }
  return 1;
}

//----------------------------------------------------------------------
static bool is_first_insn_in_exec_packet(ea_t ea)
{
  if ( (ea & 0x1F) == 0 ) return 1;
  ea = prev_not_tail(ea);
  return ea == BADADDR
      || !isCode(get_flags_novalue(ea))
      || (get_long(ea) & BIT0) == 0;
}

//----------------------------------------------------------------------
static bool prev_complex(void)
{
  ea_t ea = prev_not_tail(cmd.ea);
  if ( ea == BADADDR || !isCode(get_flags_novalue(ea)) ) return 0;
  return !is_first_insn_in_exec_packet(ea);
}

//----------------------------------------------------------------------
void out(void)
{
  char buf[MAXSTR];
  ea_t ea = cmd.ea;

  init_output_buffer(buf, sizeof(buf));

//
//      Parallel instructions
//
  if ( !is_first_insn_in_exec_packet(cmd.ea) )
  {
    out_symbol('|');
    out_symbol('|');
  }
  else
  {
    if ( !has_any_name(uFlag)
      && (prev_complex() || cmd.cflags & aux_para) ) MakeNull();
    OutChar(' ');
    OutChar(' ');
  }

//
//      Condition code
//
  static char *conds[] =
  {
    "     ", "     ", "[B0] ", "[!B0]",
    "[B1] ", "[!B1]", "[B2] ", "[!B2]",
    "[A1] ", "[!A1]", "[A2] ", "[!A2]",
    "     ", "     ", "     ", "     "
  };
  out_keyword(conds[cmd.cond]);
  OutChar(' ');

//
//      Instruction name
//
  OutMnem();
//
//      Functional unit
//
  static char *units[] =
  {
    NULL,
    ".L1", ".L2",
    ".S1", ".S2",
    ".M1", ".M2",
    ".D1", ".D2",
  };
  if ( cmd.funit != FU_NONE )
    out_keyword(units[cmd.funit]);
  else
    OutLine("   ");
  if ( cmd.cflags & aux_xp )
    out_keyword("X");
  else
    OutChar(' ');
  OutLine("   ");

//
//      Operands
//
  if ( (cmd.cflags & aux_src2) != 0 )
  {
    outreg(cmd.Op1.src2);
    out_symbol(',');
    OutChar(' ');
  }

  if ( cmd.Op1.showed() ) out_one_operand(0);

  if ( cmd.Op2.type != o_void && cmd.Op2.showed() )
  {
    out_symbol(',');
    OutChar(' ');
    out_one_operand(1);
  }


  if ( cmd.Op3.type != o_void && cmd.Op3.showed() )
  {
    out_symbol(',');
    OutChar(' ');
    out_one_operand(2);
  }

  if ( isVoid(ea,uFlag,0) ) OutImmChar(cmd.Op1);
  if ( isVoid(ea,uFlag,1) ) OutImmChar(cmd.Op2);

  term_output_buffer();
  gl_comm = 1;
  int indent = inf.indent - 7;
  if ( indent <= 0 ) indent = 1;
  MakeLine(buf,indent);

  if ( (cmd.cflags & aux_para) == 0 )
  {
    ea_t target = tnode.altval(cmd.ea);
    switch ( target )
    {
      case 1:
        printf_line(inf.indent+1, COLSTR("; BRANCH OCCURS",SCOLOR_AUTOCMT));
        break;
      case 2:
        printf_line(inf.indent+1, COLSTR("; CALL OCCURS",SCOLOR_AUTOCMT));
        break;
      default:
        if ( target != 0 )
          printf_line(inf.indent+1,
                      COLSTR("; %s %s OCCURS",SCOLOR_AUTOCMT),
                      (target & 1) ? "BRANCH" : "CALL",
                      get_colored_name(cmd.ea, target&~1, buf, sizeof(buf)));
        break;
    }
  }
}

//--------------------------------------------------------------------------
void segstart(ea_t ea)
{
  segment_t *Sarea = getseg(ea);
  if ( is_spec_segm(Sarea->type) ) return;

  char sname[MAXNAMELEN];
  get_true_segm_name(Sarea, sname, sizeof(sname));

  char *dir;
  if ( strcmp(sname,".bss") == 0 ) return;
  if ( strcmp(sname,".text") == 0
    || strcmp(sname,".data") == 0 )
      dir = COLSTR("%s",SCOLOR_ASMDIR);
    else
      dir = COLSTR(".sect \"%s\"",SCOLOR_ASMDIR);
  printf_line(inf.indent, dir, sname);
}

//--------------------------------------------------------------------------
void segend(ea_t)
{
}

//--------------------------------------------------------------------------
void header(void)
{
  gen_cmt_line("Processor       : %8.8s", inf.procName);
  gen_cmt_line("Target assembler: %s", ash.name);
  gen_cmt_line("Byte sex        : %s", inf.mf ? "Big endian" : "Little endian");
  if ( ash.header != NULL )
    for ( const char **ptr=ash.header; *ptr != NULL; ptr++ )
      printf_line(0,COLSTR("%s",SCOLOR_ASMDIR),*ptr);
}

//--------------------------------------------------------------------------
void footer(void)
{
  char name[MAXSTR];
  get_colored_name(BADADDR, inf.beginEA, name, sizeof(name));
  const char *end = ash.end;
  if ( end == NULL )
    printf_line(inf.indent,COLSTR("%s end %s",SCOLOR_AUTOCMT), ash.cmnt, name);
  else
    printf_line(inf.indent,COLSTR("%s",SCOLOR_ASMDIR)
                  " "
                  COLSTR("%s %s",SCOLOR_AUTOCMT), ash.end, ash.cmnt, name);
}

//--------------------------------------------------------------------------
void data(ea_t ea)
{
  segment_t *s = getseg(ea);
  if ( s != NULL )
  {
    char sname[MAXNAMELEN];
    if ( get_true_segm_name(s, sname, sizeof(sname)) > 0
      && strcmp(sname, ".bss") == 0 )
    {
      char nbuf[MAXSTR];
      flags_t flags = get_flags_novalue(ea);
      char *name = get_colored_name(BADADDR, ea, nbuf, sizeof(nbuf));
      char buf[MAXSTR];
      if ( name == NULL )
      {
        name = buf;
        qsnprintf(buf, sizeof(buf), COLSTR("bss_dummy_name_%lx",SCOLOR_UNKNAME), ea);
      }
      char num[MAX_NUMBUF];
      btoa(num, sizeof(num), get_item_size(ea), getRadix(flags, 0));
      printf_line(-1,
                  COLSTR(".bss",SCOLOR_KEYWORD)
                  " %s, "
                  COLSTR("%s",SCOLOR_DNUM),
                  name,
                  num);
      return;
    }
  }
  intel_data(ea);
}

//--------------------------------------------------------------------------
bool outspec(ea_t ea, uchar segtype)
{
  char name[MAXSTR];
  if ( get_colored_name(BADADDR, ea, name, sizeof(name)) == NULL )
    return false;
  gl_xref = 1;
  char buf[MAX_NUMBUF];
  switch ( segtype )
  {
    case SEG_XTRN:
      return printf_line(-1,COLSTR("%s %s",SCOLOR_ASMDIR),ash.a_extrn,name);
    case SEG_ABSSYM:
      // i don't know how to declare absolute symbols.
      // perhaps, like this?
      btoa(buf, sizeof(buf), get_long(ea));
      return printf_line(-1,COLSTR("%s = %s", SCOLOR_ASMDIR), name, buf);
    case SEG_COMM:
      gl_name = 1;
      btoa(buf, sizeof(buf), get_long(ea));
      printf_line(-1,COLSTR("%s \"%s\", %s",SCOLOR_ASMDIR),
                                        ash.a_comdef, name, buf);
  }
  return false;
}
