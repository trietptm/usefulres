/*
 * 	NEC 78K0 processor module for IDA Pro.
 *	Copyright (c) 2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#include "78k0.hpp"
#include <ints.hpp>
#include <diskio.hpp>

static inline void OutReg(int rgnum)
{
  out_register(ph.regNames[rgnum]);
}

static void OutVarName(op_t &x)
{
ushort addr = x.addr;
ulong toea = toEA(codeSeg(addr,x.n), addr);
#if IDP_INTERFACE_VERSION > 37
	if(out_name_expr(x,toea,addr))return;
#else
	const char *ptr;
	if((ptr=get_name_expr(cmd.ea+x.offb, toea, addr)) != NULL){
		//�뢮� ���� ��६����� � ��⮪ ���室�
	    OutLine(ptr);
	}
#endif
else OutValue(x, OOF_ADDR | OOFW_16);
}

static void OutVarNameVal(op_t &x)
{
ushort addr = x.value;
ulong toea = toEA(codeSeg(addr,x.n), addr);
#if IDP_INTERFACE_VERSION > 37
	if(out_name_expr(x,toea,addr))return;
#else
	const char *ptr;
	if((ptr=get_name_expr(cmd.ea+x.offb, toea, addr)) != NULL){
		//�뢮� ���� ��६����� � ��⮪ ���室�
	    OutLine(ptr);
	}
#endif
else OutValue(x, OOFW_16);
}


//----------------------------------------------------------------------
// �뢮� ������ ���࠭��
bool N78K_outop(op_t &x)
{
#if IDP_INTERFACE_VERSION <= 37
  uFlag = getFlags(cmd.ea);
#endif
  switch(x.type){
  case o_void: return 0;

  case o_reg:
		if(x.FormOut & FORM_OUT_SKOBA) out_symbol('[');
		OutReg(x.reg);
		if(x.FormOut & FORM_OUT_PLUS) out_symbol('+');
		if(x.FormOut & FORM_OUT_DISP){
			if(isOff(uFlag, x.n)){
				OutVarNameVal(x);
			}
			else OutValue(x, OOFW_IMM );
		}
		if(x.FormOut & FORM_OUT_REG){
			out_keyword( ph.regNames[x.SecondReg] );
		}
		if(x.FormOut & FORM_OUT_SKOBA) out_symbol(']');
		break;

  case o_bit:
       switch(x.FormOut){
        case FORM_OUT_S_ADDR:
        case FORM_OUT_SFR:
				OutVarName(x);
				out_symbol('.');
#if IDP_INTERFACE_VERSION > 37
				if( !nec_find_ioport_bit(x.addr, x.value) )
#endif
				{
					OutValue(x, OOFW_IMM);
				}
				break;

        case FORM_OUT_A:
				OutLine("A.");
				OutValue(x, OOFW_IMM);
				break;

        case FORM_OUT_PSW:
				OutLine("PSW.");
				switch(x.value){
				case 0:	OutLine("CY");break;
				case 1:	OutLine("ISP");break;
				case 3:	OutLine("RBS0");break;
				case 4:	OutLine("AC");break;
				case 5:	OutLine("RBS1");break;
				case 6:	OutLine("Z");break;
				case 7:	OutLine("IE");break;
				default:OutValue(x, OOFW_IMM);
				}
				break;

		case FORM_OUT_HL:
            out_symbol('[');
            OutReg(rHL);
            out_symbol(']');
            out_symbol('.');
			if(isOff(uFlag, x.n)){
				OutVarNameVal(x);
			}
			else OutValue(x, OOFW_IMM );
            break;

		}
		break;

  case o_imm:
		out_symbol('#');
		if(isOff(uFlag, x.n)){
			OutVarNameVal(x);
		}
		else OutValue(x, OOFW_IMM );
		break;

  case o_mem:
			//�뢮��� ��� ��६����� �� �����(���ਬ�� byte_98)
			if(x.FormOut & FORM_OUT_VSK)  out_symbol('!');
            if(x.FormOut & FORM_OUT_SKOBA) out_symbol('[');
			//�뢮� ����� ��६�����
			OutVarName(x);
            if(x.FormOut & FORM_OUT_SKOBA) out_symbol(']');
		    break;

  case o_near:
            if(x.FormOut & FORM_OUT_VSK) out_symbol('!');
            if(x.FormOut & FORM_OUT_SKOBA) out_symbol('[');
	       {
		    ulong adr;
			adr = toEA(codeSeg(x.addr,x.n),x.addr);
#if IDP_INTERFACE_VERSION > 37
            if( !out_name_expr(x, adr, x.addr)){
              OutValue(x, OOF_ADDR | OOF_NUMBER | OOFW_16);
              QueueMark(Q_noName, cmd.ea);
            }
#else
		{const char *ptr;
			ptr=get_name_expr(cmd.ea+x.offb, adr, x.addr);
            if( ptr == NULL ){
				OutValue(x, OOF_ADDR | OOF_NUMBER | OOFW_16);
				QueueMark(Q_noName, cmd.ea);
			}
			else OutLine(ptr);
		}
#endif
			}
			if(x.FormOut & FORM_OUT_SKOBA) out_symbol(']');
			break;

  // ��������� ���࠭�
  default:      warning("out: %lx: bad optype",cmd.ea,x.type);
                break;
  }
  return(1);
}

//----------------------------------------------------------------------
// �᭮���� �뢮����� ������
void N78K_out(void)
{
  char buf[MAXSTR];
#if IDP_INTERFACE_VERSION > 37
   init_output_buffer(buf, sizeof(buf)); // setup the output pointer
#else
  uFlag = getFlags(cmd.ea);
   u_line = buf;
#endif
   // �뢥��� ���������
   OutMnem();

   // �뢥��� ���� ���࠭�
   if(cmd.Op1.type!=o_void)out_one_operand(0);
   // �뢥��� ��ன ���࠭�
   if(cmd.Op2.type != o_void) {
     out_symbol(',');
     OutChar(' ');
     out_one_operand(1);
   }
   // �뢥��� �����।�⢥��� �����, �᫨ ��� ����
   if(isVoid(cmd.ea, uFlag, 0)) OutImmChar(cmd.Op1);
   if(isVoid(cmd.ea, uFlag, 1)) OutImmChar(cmd.Op2);
#if IDP_INTERFACE_VERSION > 37
   term_output_buffer();
#else
  *u_line = '\0';
#endif
   gl_comm = 1;
   MakeLine(buf);
}

//--------------------------------------------------------------------------
void N78K_header(void)
{
#if IDP_INTERFACE_VERSION > 37
  gen_cmt_line("Processor:       %s [%s]", device[0] ? device : inf.procName, deviceparams);
#else
  gen_cmt_line("Processor:       %s", inf.procName);
#endif
  gen_cmt_line("Target assebler: %s", ash.name);
  // ��������� ��� �����⭮�� ��ᥬ����
  if (ash.header != NULL )
    for (const char **ptr=ash.header; *ptr != NULL; ptr++ ) MakeLine(*ptr, 0);
}

//--------------------------------------------------------------------------
// ��砫� ᥣ����
void N78K_segstart(ea_t ea)
{
  segment_t *Sarea = getseg(ea);
  const char *SegType=	(Sarea->type==SEG_CODE)?"CSEG":
						((Sarea->type==SEG_DATA)?"DSEG":
						"RSEG"
						);
	// �뢥��� ��ப� ���� RSEG <NAME>
#if IDP_INTERFACE_VERSION > 37
	char sn[MAXNAMELEN];
	get_segm_name(Sarea,sn,sizeof(sn));
	printf_line(-1,"%s %s ",SegType, sn);
#else
	printf_line(-1,"%s %s ",SegType, get_segm_name(Sarea));
#endif
	// �᫨ ᬥ饭�� �� ���� - �뢥��� � ��� (ORG XXXX)
	if ( inf.s_org ) {
		ulong org = ea - get_segm_base(Sarea);
		if( org != 0 ){
#if IDP_INTERFACE_VERSION > 37
			char bufn[MAX_NUMBUF];
			btoa(bufn, sizeof(bufn), org);
			printf_line(-1, "%s %s", ash.origin, bufn);
#else
			printf_line(-1, "%s %s", ash.origin, btoa(org));
#endif
		}
	}
}

//--------------------------------------------------------------------------
// ����� ⥪��
void N78K_footer(void)
{
  char buf[MAXSTR];
  char *const end = buf + sizeof(buf);
  if (ash.end != NULL){
    MakeNull();
#if IDP_INTERFACE_VERSION > 37
    char *ptr = tag_addstr(buf, end, COLOR_ASMDIR, ash.end);
    char name[MAXSTR];
    if (get_colored_name(BADADDR, inf.beginEA, name, sizeof(name)) != NULL){
		register int i = strlen(ash.end);
		do APPCHAR(ptr, end, ' '); while(++i < 8);
		APPEND(ptr, end, name);
    }
    MakeLine(buf,inf.indent);
#else
    register char *p = tag_addstr(buf, COLOR_ASMDIR, ash.end);
    const char *start = get_colored_name(inf.beginEA);
    if(start != NULL) {
      *p++ = ' ';
      strcpy(p, start);
    }
    MakeLine(buf);
#endif
  } else gen_cmt_line("end of file");
}


//--------------------------------------------------------------------------
void N78K_data(ea_t ea)
{
	gl_name =1;
	intel_data(ea);
}
