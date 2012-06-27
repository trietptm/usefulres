/*
 *  Interactive disassembler (IDA).
 *  Zilog Z8 module
 *
 */

#include "z8.hpp"

// ig: emuFlg �����-� �� �㦥�, �.�. �� 㦥 � ��६����� uFlag
//     �������� 䫠�� ��। �맮��� emu()
//static long emuFlg;
static int  flow;

//----------------------------------------------------------------------

static void TouchArg( op_t &x, int isload )
{
  switch( x.type )
  {
    case o_imm:
    case o_displ:
      if ( isOff(uFlag, x.n) ) ua_add_off_drefs(x, dr_O);
      break;

    case o_mem:
    case o_ind_mem:
      {
        ulong dea = intmem + x.addr;
        ua_dodata( dea, x.dtyp );
        if( !isload )   doVar( dea );
        ua_add_dref( x.offb, dea, isload ? dr_R : dr_W );
      }
      break;

    case o_near:
      ulong ea = toEA( cmd.cs, x.addr );
      int iscall = InstrIsSet( cmd.itype, CF_CALL );
      ua_add_cref( x.offb, ea, iscall ? fl_CN : fl_JN );
      if( flow && iscall )
      {
        if ( !func_does_return(ea) )
          flow = false;
      }
  }
}

//----------------------------------------------------------------------

int emu( void )
{
  ulong Feature = cmd.get_canon_feature();

  flow = ((Feature & CF_STOP) == 0);

  if( Feature & CF_USE1 )   TouchArg( cmd.Op1, 1 );
  if( Feature & CF_USE2 )   TouchArg( cmd.Op2, 1 );
  if( Feature & CF_JUMP )   QueueMark( Q_jumps, cmd.ea );

  if( Feature & CF_CHG1 )   TouchArg( cmd.Op1, 0 );
  if( Feature & CF_CHG2 )   TouchArg( cmd.Op2, 0 );

  if( flow )                ua_add_cref( 0, cmd.ea+cmd.size, fl_F );

  return 1;
}
