/*
 * 	Panasonic MN102 (PanaXSeries) processor module for IDA Pro.
 *	Copyright (c) 2000-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#include "pan.hpp"

static void OutVarName(op_t &x)
{
ea_t addr = x.addr;
ea_t toea = toEA(codeSeg(addr,x.n), addr);
#if IDP_INTERFACE_VERSION > 37
//	msg("AT:%a target=%lx, segm=%lx, Result=%lx\n",
//			cmd.ea,addr, codeSeg(addr,x.n),toea);
	if(out_name_expr(x,toea,addr))return;
#else
	const char *ptr;
	if((ptr=get_name_expr(cmd.ea+x.offb, toea, addr)) != NULL){
		//�뢮� ���� ��६����� � ��⮪ ���室�
	    OutLine(ptr);
	}
#endif
	else{
		OutValue(x, OOF_ADDR | OOF_NUMBER |
					OOFS_NOSIGN | OOFW_32);
		// ����⨬ �஡���� - ��� �����
		QueueMark(Q_noName,cmd.ea);
	}
}

//----------------------------------------------------------------------
// �뢮� ������ ���࠭��
bool idaapi mn102_outop(op_t &x)
{
  switch ( x.type ){
  // ��뫪� �� ������ � �ᯮ�짮������ ॣ���� (ॣ���஢)
  // (disp,Ri)
  case o_displ: // ���뢠��� ᪮��� ���� �ᥣ��
                // ॣ���� ���������?
                out_symbol('(');
				OutValue(x);
                out_symbol(',');
                out_register(ph.regNames[x.reg]);
                out_symbol(')');
                break;

  // ॣ����
  case o_reg:           if(x.reg&0x80)out_symbol('(');
                        if(x.reg&0x10){
                                out_register(ph.regNames[((x.reg>>5)&3)+rD0]);
                                out_symbol(',');
                        }
                        out_register(ph.regNames[x.reg&0x0F]);
                        if(x.reg&0x80)out_symbol(')');
                        break;

  // �����।�⢥��� �����
  case o_imm:           
#if IDP_INTERFACE_VERSION > 37
				refinfo_t ri;
				// micro bug-fix
				if(get_refinfo(cmd.ea, x.n, &ri)){
					if(ri.flags==REF_OFF16)
						set_refinfo(cmd.ea, x.n, 
							REF_OFF32, ri.target, ri.base, ri.tdelta);
					msg("Exec OFF16_Op Fix AT:%a Flags=%lx, Target=%lx, Base=%lx, Delta=%lx\n",
						cmd.ea,
						ri.flags,ri.target,ri.base,ri.tdelta);
				}
#endif
                        OutValue(x, /*OOFS_NOSIGN | */ OOF_SIGNED | OOFW_IMM);
                        break;

  // ��뫪� �� �ணࠬ��
  case o_near:  OutVarName(x);
                break;

  // ��ﬠ� ��뫪� �� ������
  case o_mem:   out_symbol('(');
				OutVarName(x);
                out_symbol(')');
                break;

  // ����몠 �� �뢮�����
  case o_void:  return 0;

  // ��������� ���࠭�
  default:      warning("out: %lx: bad optype",cmd.ea,x.type);
                break;
  }
  return 1;
}

//----------------------------------------------------------------------
// �᭮���� �뢮����� ������
void idaapi mn102_out(void)
{
  char buf[MAXSTR];
#if IDP_INTERFACE_VERSION > 37
   init_output_buffer(buf, sizeof(buf)); // setup the output pointer
#else
   u_line = buf;
#endif
  // �뢥��� ���������
  OutMnem();

  // �뢥��� ���� ���࠭�
  if(cmd.Op1.type!=o_void)out_one_operand(0);

  // �뢥��� ��ன ���࠭�
  if(cmd.Op2.type != o_void){
        out_symbol(',');
        OutChar(' ');
        out_one_operand(1);
        // �뢥��� ��⨩ ���࠭�
        if(cmd.Op3.type != o_void){
                out_symbol(',');
                OutChar(' ');
                out_one_operand(2);
        }
  }

  // �뢥��� �����।�⢥��� �����, �᫨ ��� ����
  if ( isVoid(cmd.ea,uFlag,0) ) OutImmChar(cmd.Op1);
  if ( isVoid(cmd.ea,uFlag,1) ) OutImmChar(cmd.Op2);
  if ( isVoid(cmd.ea,uFlag,2) ) OutImmChar(cmd.Op3);

  // �����訬 ��ப�
#if IDP_INTERFACE_VERSION > 37
   term_output_buffer();
#else
  *u_line = '\0';
#endif
  gl_comm = 1;
  MakeLine(buf);
}

//--------------------------------------------------------------------------
// ��������� ⥪�� ���⨭��
void idaapi mn102_header(void)
{
#if IDP_INTERFACE_VERSION > 37
  gen_cmt_line("Processor:       %s [%s]", device[0] ? device : inf.procName, deviceparams);
#else
  gen_cmt_line("Processor:       %s", inf.procName);
#endif
  gen_cmt_line("Target assembler: %s", ash.name);
  // ��������� ��� �����⭮�� ��ᥬ����
  if ( ash.header != NULL )
    for ( const char **ptr=ash.header; *ptr != NULL; ptr++ ) MakeLine(*ptr,0);
}

//--------------------------------------------------------------------------
// ��砫� ᥣ����
void idaapi mn102_segstart(ea_t ea)
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
                ea_t org = ea - get_segm_base(Sarea);
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
void idaapi mn102_footer(void)
{
  char buf[MAXSTR];
  char *const end = buf + sizeof(buf);
  if(ash.end != NULL) {
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

/*
//--------------------------------------------------------------------------
static void near out_equ_name(char *name,const char *equ,ulong off)
{
  char buf[MAXSTR];
  register char *p;
   gl_name = 0;
    p = tag_addstr(buf, COLOR_DNAME, name);
    *p++ = ':';
    *p++ = ' ';
    p = tag_addstr(p, COLOR_KEYWORD, equ);
    *p++ = ' ';
  tag_addstr(p, COLOR_NUMBER, btoa(off));
  MakeLine(buf,0);
}

//--------------------------------------------------------------------------
static int near out_equ(ea_t ea)
{
  segment_t *s = getseg(ea);
  if(ea>0x880)return(0);
  if(s != NULL && s->type == SEG_IMEM && ash.a_equ != NULL) {
    char *name = get_name(ea);
    if(name != NULL && IsPredefined(name) ){
//                msg("Off=%lx, Name=%lx, Name=[%s]\n",
//                        ea, name, name);
                char buf[MAXSTR], *ptr = stpcpy(buf, name);
                uchar off = uchar(ea - get_segm_base(s));
                out_equ_name(buf, ash.a_equ, off);
                return(1);
    }
        gl_name = 0;
        MakeLine("");
        return 1;
  }
    if( !hasValue(getFlags(ea)) && s->type == SEG_CODE ) {
      char buf[MAXSTR];
      ulong org = ea - get_segm_base(s) + getSize(ea);
      sprintf(buf, COLSTR("%s %s", SCOLOR_ASMDIR), ash.origin, btoa(org));
      MakeLine(buf);
      return 1;
    }
  return 0;
}
*/

//--------------------------------------------------------------------------
void idaapi mn102_data(ea_t ea)
{
#if IDP_INTERFACE_VERSION > 37
refinfo_t ri;
	// micro bug-fix
	if(get_refinfo(ea, 0, &ri)){
		if(ri.flags==REF_OFF16){
			set_refinfo(ea, 0, 
				REF_OFF32, ri.target, ri.base, ri.tdelta);
		msg("Exec OFF16 Fix AT:%a Flags=%lx, Target=%lx, Base=%lx, Delta=%lx\n",ea,
				ri.flags,ri.target,ri.base,ri.tdelta);
		}
	}
#endif
  gl_name = 1;
  // ���஡㥬  �뢥��, ��� equ
//  if (out_equ(ea)) return;
  // �� ����稫��� - �뢮��� ����묨
        intel_data(ea);
}
