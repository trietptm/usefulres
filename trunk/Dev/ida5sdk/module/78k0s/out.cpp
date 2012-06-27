/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2001 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *
 */

#include "78k_0s.hpp"
#include <ints.hpp>
#include <diskio.hpp>

//----------------------------------------------------------------------
inline void OutReg(int rgnum)
{
out_register(ph.regNames[rgnum]);
}
//----------------------------------------------------------------------
static int OutVarName(op_t &x, int iscode, int relative)
{
char *ptr;
ushort addr = x.addr;
if(relative)
  {
  addr += (ushort)cmd.ip;
  addr += cmd.size;           // ig: this is tested only for 6809
  }
//������� ������� �����
ulong toea = toEA((iscode || relative) ? codeSeg(addr,x.n) : dataSeg_op(x.n), addr);
//������� ��ப� ��� ������� ���. �����
return out_name_expr(x, toea, addr);
}
//----------------------------------------------------------------------
bool outop(op_t &x)
{
switch(x.type)
  {
  case o_void:  return 0;

  case o_reg:
    if(x.prepost) out_symbol('[');
    //�뢮� ॣ���� �� ������ � ॣ����
    OutReg(x.reg);
    if(x.xmode)
      {
      out_symbol('+');
      OutValue(x, OOF_ADDR | OOF_NUMBER | OOFW_8);
      }
    if(x.prepost) out_symbol(']');
    break;

  case o_phrase:
    OutLine(ph.regNames[x.reg]);
    break;

  case o_bit:
    {
    switch(x.reg)
      {
      case rPSW:
        {
        OutLine("PSW.");
        switch(x.value)
          {
          case 0: OutLine("CY");break;
          case 4: OutLine("AC");break;
          case 6: OutLine("Z");break;
          case 7: OutLine("IE");break;
          default:OutValue(x, OOFW_IMM);
          } break;
        }

      case rA:
        {
        OutLine( "A." );
        OutChar('0'+x.value);
        } break;

      default:
        {
        if(!OutVarName(x, 1, 0) ) OutValue(x, OOF_ADDR | OOFW_16);
        out_symbol('.');
        //�祬 �������� ��� �� 㪠������� ������
        if(!nec_find_ioport_bit(x.addr, x.value))
          {
          //�뢮� ������(⨯ o_imm)
          OutChar('0'+x.value);
          }
        }//       switch(x.regmode)
      }  // end switch(x.reg)
    } break;

  case o_imm:
    {
    if(!x.regmode)
      {
      out_symbol('#');
      //�뢮� ������(⨯ o_imm)
      OutValue(x, OOFW_IMM );
      }
    else
      {
      out_symbol('1');
      }
    } break;

  case o_mem:
    {
    if(x.addr16) out_symbol('!');
    //�뢮��� ��� ��६����� �� �����(���ਬ�� byte_98)
    //�뢮� ����� ��६�����
    if(!OutVarName(x, 1, 0) )
    //�뢮� ������
    OutValue(x, OOF_ADDR | OOFW_16);
    } break;

  case o_near:
    {
    ulong v;
    if(x.addr16) out_symbol('!');
    if(x.form) out_symbol('[');
    //������� ������� �����
    v = toEA(cmd.cs,x.addr);
    if(!out_name_expr(x, v, x.addr))
      {
      //�뢥�� ���祭��
      OutValue(x, OOF_ADDR | OOF_NUMBER | OOFW_16);
      QueueMark(Q_noName, cmd.ea);
      }
    if(x.form) out_symbol(']');
    } break;

  default:
    warning("out: %a: bad optype %d", cmd.ip, x.type);
    break;
  }

return(1);
}
//----------------------------------------------------------------------
void out(void)
{
char buf[MAXSTR];

init_output_buffer(buf, sizeof(buf)); // setup the output pointer
OutMnem();                            // output instruction mnemonics

out_one_operand(0);                   // output the first operand

//�뢮� ���࠭��
if(cmd.Op2.type != o_void)
  {
  out_symbol(',');//�뢮� ࠧ����⥫� ����� ���࠭����
  //�᫨ ��㪠��� 䫠� UAS_NOSPA �⠢�� �஡��
  if(!(ash.uflag & UAS_NOSPA)) OutChar(' ');
  out_one_operand(1);
  }

if(cmd.Op3.type != o_void)
  {
  out_symbol(',');
  if(!(ash.uflag & UAS_NOSPA)) OutChar(' ');
  out_one_operand(2);
  }

if(isVoid(cmd.ea, uFlag, 0)) OutImmChar(cmd.Op1);
if(isVoid(cmd.ea, uFlag, 1)) OutImmChar(cmd.Op2);
if(isVoid(cmd.ea, uFlag, 2)) OutImmChar(cmd.Op3);

term_output_buffer();

gl_comm = 1;
MakeLine(buf);
}
//--------------------------------------------------------------------------
void header(void)
{
gen_cmt_line("Processor:       %s [%s]", device[0] ? device : inf.procName, deviceparams);
gen_cmt_line("Target assebler: %s", ash.name);
if (ash.header != NULL )
for (const char **ptr=ash.header; *ptr != NULL; ptr++ ) MakeLine(*ptr, 0);
}
//--------------------------------------------------------------------------
void segstart(ea_t /*ea*/)
{
}
//--------------------------------------------------------------------------
void footer(void)
{
char buf[MAXSTR];
char *const end = buf + sizeof(buf);
if (ash.end != NULL)
  {
  MakeNull();
  char *ptr = tag_addstr(buf, end, COLOR_ASMDIR, ash.end);
  char name[MAXSTR];
  if (get_colored_name(BADADDR, inf.beginEA, name, sizeof(name)) != NULL)
    {
    register size_t i = strlen(ash.end);
    do APPCHAR(ptr, end, ' '); while(++i < 8);
    APPEND(ptr, end, name);
    }
  MakeLine(buf, inf.indent);
  }
else gen_cmt_line("end of file");
}
//--------------------------------------------------------------------------


