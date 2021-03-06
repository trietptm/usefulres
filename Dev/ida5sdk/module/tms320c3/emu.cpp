/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *
 */

#include "tms320c3x.hpp"
#include <srarea.hpp>
#include <frame.hpp>

static int flow;
//----------------------------------------------------------------------
ea_t calc_code_mem(op_t &x)
{
  return toEA(cmd.cs, x.addr);
}

//------------------------------------------------------------------------
ea_t calc_data_mem(op_t &x)
{
    sel_t dpage = getSR(cmd.ea, dp);
    if ( dpage == BADSEL ) return BADSEL;
    return ((dpage & 0xFF) << 16) | (x.addr);
}

//----------------------------------------------------------------------

static void destroy_if_unnamed_array(ea_t ea)
{
  flags_t F = get_flags_novalue(ea);
  if ( isTail(F) && segtype(ea) == SEG_IMEM )
  {
    ea_t head = prev_not_tail(ea);
    if ( !has_user_name(head) )
    {
      do_unknown(head, 0);
      doByte(head, ea-head);
      ea_t end = nextthat(ea, inf.maxEA, f_isHead, NULL);
      if ( end == BADADDR ) end = getseg(ea)->endEA;
      doByte(ea+1, end-ea-1);
    }
  }
}

//----------------------------------------------------------------------
static void process_imm(op_t &x)
{
  doImmd(cmd.ea);
  if ( isDefArg(uFlag,x.n) ) return; // if already defined by user
  op_num(cmd.ea, x.n);
}

//----------------------------------------------------------------------
static void process_operand(op_t &x, int use)
{
  switch ( x.type )
  {
    case o_reg:
      return;

    case o_near:
      {
        if (cmd.itype != TMS320C3X_RPTB )
        {
          cref_t ftype = fl_JN;
          ea_t ea = calc_code_mem(x);
          if ( InstrIsSet(cmd.itype, CF_CALL) )
          {
            if ( !func_does_return(ea) )
              flow = false;
            ftype = fl_CN;
          }
          ua_add_cref(x.offb, ea, ftype);
        }
        else // evaluate RPTB loops as dref
          ua_add_dref(x.offb, calc_code_mem(x), dr_I);
      }
      break;

    case o_imm:
      if ( !use ) error("interr: emu");
      process_imm(x);
      if ( isOff(uFlag, x.n) ) ua_add_off_drefs(x, dr_O);
      break;

    case o_mem:
      {
        ea_t ea = calc_data_mem(x);
        if (ea != BADADDR)
        {
          destroy_if_unnamed_array(ea);
          ua_add_dref(x.offb, ea, use ? dr_R : dr_W);
          ua_dodata(ea, x.dtyp);
          if ( !use ) doVar(ea);
        }
      }
      break;

    case o_phrase:
      break;

    case o_displ:
      doImmd(cmd.ea);
      break;

    default:
      warning("interr: emu2 address:%a operand:%d type:%d", cmd.ea, x.n, x.type);
  }
}

//----------------------------------------------------------------------
// is the previous instruction unconditional delayed jump ?
//
// The following array shows all delayed instructions (xxx[D])
// who are required to always stop.
//
// BRANCH INSTRUCTIONS

// TMS320C3X_BRD,		// Branch unconditionally (delayed)	0110 0001 xxxx xxxx xxxx xxxx xxxx xxxx
// TMS320C3X_Bcond,		// Branch conditionally			0110 10x0 001x xxxx xxxx xxxx xxxx xxxx
// TMS320C3X_DBcond,		// Decrement and branch conditionally	0110 11xx xx1x xxxx xxxx xxxx xxxx xxxx


bool delayed_stop(void)
{
  if (! isFlow(uFlag)) return false;	// Does the previous instruction exist and pass execution flow to the current byte?

  if (cmd.size <= 0) return false;

  int sub = 3; // backward offset to skip 3 previous 1-word instruction

  ea_t ea = cmd.ea - sub;

  if (isCode(get_flags_novalue(ea)))		// Does flag denote start of an instruction?
  {
    int code = get_full_byte(ea); // get the instruction word

	if ((code & 0xff000000) == 0x61000000)	return true;	// Branch unconditionally delayed				0110 0001 xxxx xxxx xxxx xxxx xxxx xxxx
	if ((code & 0xfdff0000) == 0x68200000)	return true;	// Branch conditionally delayed (with U cond)			0110 10x0 001x xxxx xxxx xxxx xxxx xxxx
	//if ((code & 0xfc3f0000) == 0x6c200000)	return true;	// Decrement and branch conditionally (with U cond)	0110 11xx xx1x xxxx xxxx xxxx xxxx xxxx
	// ���������, �.�. ������������ � �������� ��� ����������� ������
	// � ����� �� �������� ������� ���������
  }


  return false;
}

int GetDelayedBranchAdr(void)   // if previous instruction is delayed jump return jump adr, else -1
{
  int16 disp;

  if (! isFlow(uFlag)) return -1; // Does the previous instruction exist and pass execution flow to the current byte?

  if (cmd.size <= 0) return -1;

  int sub = 3; // backward offset to skip 3 previous 1-word instruction

  ea_t ea = cmd.ea - sub;

  if (isCode(get_flags_novalue(ea)))	  // Does flag denote start of an instruction?
  {
    int code = get_full_byte(ea); // get the instruction word

    if ((code & 0xff000000) == 0x61000000) // Branch unconditionally (delayed)
        return code & 0xffffff;

    if ((code & 0xffe00000) == 0x6a200000) // BranchD conditionally
    {
        disp = code & 0xffff;
        return  cmd.ea + disp;
    }

    if ((code & 0xfe200000) == 0x6e200000) // DecrementD and branch conditionally
    {
        disp = code & 0xffff;
        return  cmd.ea + disp;
    }
  }


  return -1;
}


//----------------------------------------------------------------------
bool is_basic_block_end(void)
{
  if (delayed_stop()) return true;
  return ! isFlow(get_flags_novalue(cmd.ea+cmd.size));
}

//----------------------------------------------------------------------
static void trace_sp(void)
{
  switch (cmd.itype)
  {
    case TMS320C3X_RETIcond:
      add_auto_stkpnt(cmd.ea+cmd.size, -2);
      break;
    case TMS320C3X_RETScond:
      add_auto_stkpnt(cmd.ea+cmd.size, -1);
      break;
    case TMS320C3X_POP:
    case TMS320C3X_POPF:
      add_auto_stkpnt(cmd.ea+cmd.size, -1);
      break;
    case TMS320C3X_PUSH:
    case TMS320C3X_PUSHF:
      add_auto_stkpnt(cmd.ea+cmd.size, 1);
      break;
  }
}

//----------------------------------------------------------------------
int emu(void)
{
  ulong feature = cmd.get_canon_feature();
  flow = (feature & CF_STOP) == 0;

  if ((cmd.auxpref & DBrFlag) == 0)   // � ���������� ��������� �� ���� ������������ ��������, �.�.
                                      // �������� � ��� �� ��������������, � ���. �������� ����� ��������� ����� 3 �������
  {
        if ( feature & CF_USE1 ) process_operand(cmd.Op1, 1);
        if ( feature & CF_USE2 ) process_operand(cmd.Op2, 1);
        if ( feature & CF_USE3 ) process_operand(cmd.Op3, 1);

        if ( feature & CF_CHG1 ) process_operand(cmd.Op1, 0);
        if ( feature & CF_CHG2 ) process_operand(cmd.Op2, 0);
        if ( feature & CF_CHG3 ) process_operand(cmd.Op3, 0);
  }


  if  (GetDelayedBranchAdr() != -1)  // �������� ������ �� ����������� ��������
        ua_add_cref(0, toEA(cmd.cs, GetDelayedBranchAdr()), fl_JN);

  if  (cmd.itype == TMS320C3X_RETScond)  // �������� ������ �� ��������� ������
        ua_add_cref(0, cmd.ea, fl_JN);

  // check for DP changes
  if ( ((cmd.itype == TMS320C3X_LDIcond) || (cmd.itype == TMS320C3X_LDI)) && (cmd.Op1.type == o_imm)
    && (cmd.Op2.type == o_reg) && (cmd.Op2.reg == dp))
      splitSRarea1(get_item_end(cmd.ea), dp, cmd.Op1.value & 0xFF, SR_auto);

  // determine if the next instruction should be executed
  if ( segtype(cmd.ea) == SEG_XTRN ) flow = 0;
  if ( flow && delayed_stop() ) flow = 0;
  if ( flow ) ua_add_cref(0,cmd.ea+cmd.size,fl_F);

  if ( may_trace_sp() )
  {
    if ( !flow )
      recalc_spd(cmd.ea);     // recalculate SP register for the next insn
    else
      trace_sp();
  }

  return 1;
}

//----------------------------------------------------------------------
bool create_func_frame(func_t *pfn)	// create frame of newly created function
{
  if ( pfn != NULL )
  {
    if ( pfn->frame == BADNODE )
    {
      ea_t ea = pfn->startEA;
      int regsize = 0;
      while (ea < pfn->endEA) // check for register pushs
      {
        ua_ana0(ea);
        ea += cmd.size;		// ������� ���-�� push
        if ( ((cmd.itype == TMS320C3X_PUSH) || (cmd.itype == TMS320C3X_PUSHF)) && (cmd.Op1.type == o_reg) )
            regsize++;
        else	// �������� ����������� � sp ����:	LDI	SP,AR3	ADDI	#0001,SP ����������
	    if 	( ((cmd.Op1.type == o_reg) && (cmd.Op1.reg == sp)) || ((cmd.Op2.type == o_reg) && (cmd.Op2.reg == sp)) )
		continue;
	    else
		break;
      }

      ea = pfn->startEA;
      int localsize = 0;
      while (ea < pfn->endEA) // check for frame creation
      {
        ua_ana0(ea);
        ea += cmd.size;	// ������� ���������� ������� ����	ADDI	#0001,SP
        if ( (cmd.itype == TMS320C3X_ADDI) && (cmd.Op1.type == o_imm) && (cmd.Op2.type == o_reg) && (cmd.Op2.reg == sp) )
        {
          localsize = cmd.Op1.value;
          break;
        }
      }
      add_frame(pfn, localsize, regsize, 0);
    }
  }
  return 0;
}

//----------------------------------------------------------------------
//      Is the instruction created only for alignment purposes?
//      returns: number of bytes in the instruction
int is_align_insn(ea_t ea)
{

  if ( !ua_ana0(ea) ) return 0;

  switch ( cmd.itype )
  {
    case TMS320C3X_NOP:
      break;
    default:
      return 0;
  }

  return cmd.size;
}


