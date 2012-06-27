/*
 * 	NEC 78K0 processor module for IDA Pro.
 *	Copyright (c) 2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#include "78k0.hpp"
#include <diskio.hpp>
#include <entry.hpp>
#include <srarea.hpp>

//----------------------------------------------------------------------
static char *RegNames[] = {
  "X", "A", "C", "B", "E", "D", "L", "H", "AX", "BC", "DE","HL",
  "PSW", "SP", "CY", "RB0", "RB1", "RB2", "RB3",
  "cs", "ds"
};

//----------------------------------------------------------------------
static asm_t nec78k0 = {
  AS_COLON | ASB_BINF4 | AS_N2CHR ,
  // ���짮��⥫�᪨� 䫠���
  0,
  "NEC 78K0 Assembler",			// �������� ��ᥬ����
  0,							// ����� � help'e
  NULL,							// ��⮧��������
  NULL,							// ���ᨢ �� �ᯮ������� ������権
  ".org",
  ".end",

  ";",        // comment string
  '"',        // string delimiter
  '\'',       // char delimiter
  "'\"",      // special symbols in char and string constants

  ".db",    // ascii string directive
  ".db",    // byte directive
  ".dw",    // word directive
  ".dd",     // no double words
  NULL,     // no qwords
#if IDP_INTERFACE_VERSION > 37
  NULL,     // oword  (16 bytes)
#endif
  NULL,     // no float
  NULL,     // no double
  NULL,     // no tbytes
  NULL,     // no packreal
  "#d dup(#v)",     //".db.#s(b,w) #d,#v",   // #h - header(.byte,.word)
                    // #d - size of array
                    // #v - value of array elements
                    // #s - size specifier
  ".rs %s",    // uninited data (reserve space)
  ".equ",
  NULL,         // seg prefix
  NULL,         // preline for checkarg
  NULL,      // checkarg_atomprefix
  NULL,   // checkarg operations
  NULL,   // XlatAsciiOutput
  "$",    // a_curip

  NULL,         // returns function header line
  NULL,         // returns function footer line
  NULL,         // public
  NULL,         // weak
  NULL,         // extrn
  NULL,         // comm
  NULL,         // get_type_name
  NULL         // align

#if IDP_INTERFACE_VERSION > 37
  ,'(', ')',     // lbrace, rbrace
  NULL,    // mod
  NULL,    // and
  NULL,    // or
  NULL,    // xor
  NULL,    // not
  NULL,    // shl
  NULL,    // shr
  NULL,    // sizeof
#endif
};


//----------------------------------------------------------------------
//----------------------------------------------------------------------
static char *shnames[] = { "78k0",
                     NULL };
static char *lnames[] =  {
                     "NEC 78K0",
                      NULL };


static asm_t *asms[] =
{
  &nec78k0,
  NULL
};

//--------------------------------------------------------------------------
static uchar retcNEC78K0_0[] = { 0xAF };    //ret
static uchar retcNEC78K0_1[] = { 0x9F };    //retb
static uchar retcNEC78K0_2[] = { 0x8F };    //reti
static uchar retcNEC78K0_3[] = { 0xBF };    //brk
static const bytes_t retcodes[] = {
 { sizeof(retcNEC78K0_0), retcNEC78K0_0 },
 { sizeof(retcNEC78K0_1), retcNEC78K0_1 },
 { sizeof(retcNEC78K0_2), retcNEC78K0_2 },
 { sizeof(retcNEC78K0_3), retcNEC78K0_3 },
 { 0, NULL }
};


//----------------------------------------------------------------------

#if IDP_INTERFACE_VERSION > 37
static netnode helper;
char device[MAXSTR] = "";
static size_t numports;
static ioport_t *ports;

#include "../iocommon.cpp"


//------------------------------------------------------------------
const bool nec_find_ioport_bit(int port, int bit)
{

  //���� ��� �� ॣ���� � ᯨ᪥ ���⮢
  const ioport_bit_t *b = find_ioport_bit(ports, numports, port, bit);
  if ( b != NULL && b->name != NULL ){
    //�뢮��� ��� ��� �� ॣ����
    out_line(b->name, COLOR_IMPNAME);
    return true;
  }
  return false;
}

//------------------------------------------------------------------
const char *set_idp_options(const char *keyword,int /*value_type*/,const void * /*value*/)
{
  if ( keyword != NULL ) return IDPOPT_BADKEY;
  //�㭪�� �롨ࠥ� �� ������ �� 㪠������� 䠩�� cfg
  //3 ��ࠬ��� �� callbak �㭪��, ����� ��뢠���� �� �����㦥��� ������
  char cfgfile[QMAXFILE];
  get_cfg_filename(cfgfile, sizeof(cfgfile));
  if ( choose_ioport_device(cfgfile, device, sizeof(device), parse_area_line0) )
    set_device_name(device, IORESP_PORT|IORESP_INT);
  return IDPOPT_OK;
}
//----------------------------------------------------------------------

void set_dopolnit_info(void)
{
   for(int NumBanck = 0; NumBanck < 4; NumBanck++)
     for(int Regs = 0; Regs < 8; Regs++){
      char temp[100];
      qsnprintf(temp, sizeof(temp), "Bank%d_%s", NumBanck, RegNames[Regs]);
      //����塞 �����
      ushort Addr = 0xFEE0+((NumBanck*8)+Regs);
      //��⠭�������� ��� ����
      set_name(Addr, temp);
      //��⠭�������� ������਩
      qsnprintf(temp, sizeof(temp), "Internal high-speed RAM (Bank %d registr %s)", NumBanck, RegNames[Regs]);
      set_cmt(Addr, temp, true);
     }
}

//----------------------------------------------------------------------
static int notify(processor_t::idp_notify msgid, ...)
{
  va_list va;
  va_start(va, msgid);
// A well behaving processor module should call invoke_callbacks()
// in his notify() function. If this function returns 0, then
// the processor module should process the notification itself
// Otherwise the code should be returned to the caller:

  int code = invoke_callbacks(HT_IDP, msgid, va);
  if ( code ) return code;

  switch(msgid){
	case processor_t::init:
      inf.mf = 0;
      inf.s_genflags |= INFFL_LZERO;
      helper.create("$ 78k0");
      break;

    case processor_t::newfile:
      //�뢮��� ���. ���� �����஢, � �������� ����� �㦭�, ���뢠�� ��� ��࠭���
      //������ ���ଠ�� �� cfg. �� ��⠭�� ���ଠ樨 ������뢠�� ����� � ॣ����
      {
        char cfgfile[QMAXFILE];
        get_cfg_filename(cfgfile, sizeof(cfgfile));
		if ( choose_ioport_device(cfgfile, device, sizeof(device), parse_area_line0) )
          set_device_name(device, IORESP_ALL);
        set_dopolnit_info();
      }
      break;

    case processor_t::newprc:{
          char buf[MAXSTR];
          if ( helper.supval(-1, buf, sizeof(buf)) > 0 )
            set_device_name(buf, IORESP_PORT);
        }
        break;

    case processor_t::newseg:{
		segment_t *s = va_arg(va, segment_t *);
		// Set default value of DS register for all segments
		set_default_dataseg(s->sel);
		}
		break;
  }
  va_end(va);
  return(1);
}
#else
static int notify(int msgnum,void *arg,...)
{ // Various messages:
  qnotused(arg);
  switch ( msgnum ) {
    // ����� ����� IDP
    case IDP_INIT:
      inf.mf = 0;
//      inf.s_genflags |= INFFL_LZERO;
                       break;

  // ���� 䠩�
  case IDP_NEWFILE: {
      segment_t *sptr = getnseg(0);
      if ( sptr != NULL ) {
	if ( sptr->startEA-get_segm_base(sptr) == 0 ) {
	  inf.beginEA = sptr->startEA;
	  inf.startIP = 0;
	}
      }
      // �᭮���� ᥣ���� - ������
      set_segm_class(getnseg(0),"CODE");
    }
      break;
    // ᮧ����� ������ ᥣ����
    case IDP_NEWSEG:    {
                        segment_t *seg;
                        seg=((segment_t *)arg);
						// Set default value of DS register for all segments
						set_default_dataseg(seg->sel);
                        break;
                        }

    // �롮� ������ ������
    case IDP_NEWPRC:    msg("NewProc: %s, index=%d\n",
                                lnames[(*(int*)arg)],
                                (*(int*)arg));
                        // ��⠭���� 䫠��� ����� ������
                        inf.lflags&=0x0FL;
                        inf.lflags|=(*(int*)arg)<<4;
                        break;
  }
  return(1);
}
#endif


//-----------------------------------------------------------------------
//      Processor Definition
//-----------------------------------------------------------------------
extern "C" processor_t LPH = {
  IDP_INTERFACE_VERSION,        // version
  0x8078,                       // id ������
#if IDP_INTERFACE_VERSION > 37
  PRN_HEX|PR_SEGTRANS|PR_SEGS,	// can use register names for byte names
  8,							// 8 bits in a byte for code segments
#else
  PRN_HEX|PR_SEGS,				// can use register names for byte names
#endif
  8,                            // 8 bits in a byte

  shnames,                      // ���⪨� ����� �����஢ (�� 9 ᨬ�����)
  lnames,                       // ������ ����� �����஢

  asms,                         // ᯨ᮪ ��������஢

  notify,                       // �㭪�� �����饭��

  N78K_header,                  // ᮧ����� ��������� ⥪��
  N78K_footer,                  // ᮧ����� ���� ⥪��

  N78K_segstart,                // ��砫� ᥣ����
  std_gen_segm_footer,          // ����� ᥣ���� - �⠭�����, ��� �����襭��

  NULL,                         // ��४⨢� ᬥ�� ᥣ���� - �� �ᯮ�������

  N78K_ana,                     // ����������
  N78K_emu,                     // ����� ������権

  N78K_out,                     // ⥪�⮣������
  N78K_outop,                   // ⥪⮣������ ���࠭���
  N78K_data,                    // ������� ���ᠭ�� ������
  NULL,                         // �ࠢ������� ���࠭���
  NULL,                         // can have type

  qnumber(RegNames),		// Number of registers
  RegNames,						// Regsiter names
  NULL,                         // ������� ���祭�� ॣ����

  0,                            // �᫮ ॣ���஢�� 䠩���
  NULL,                         // ����� ॣ���஢�� 䠩���
  NULL,                         // ���ᠭ�� ॣ���஢
  NULL,				// Pointer to CPU registers
  rVcs,rVds,
#if IDP_INTERFACE_VERSION > 37
  2,                            // size of a segment register
#endif
  rVcs,rVds,
  NULL,                         // ⨯��� ���� ��砫� �����
  retcodes,                     // ���� return'ov
#if IDP_INTERFACE_VERSION <= 37
  NULL,                         // �����頥� ����⭮��� ������� ��᫥����⥫쭮��
#endif
  0,NEC_78K_0_last,				// ��ࢠ� � ��᫥���� ������樨
  Instructions,                 // ���ᨢ �������� ������権
  NULL,                         // �஢�ઠ �� �������� ���쭥�� ���室�
#if IDP_INTERFACE_VERSION <= 37
  NULL,                         // ���஥��� �����稪
#endif
  NULL,                         // �࠭���� ᬥ饭��
  3,                            // ࠧ��� tbyte - 24 ���
  NULL,                         // �८�ࠧ���⥫� ������饩 �窨
  {0,0,0,0},                    // ����� ������ � ������饩 �窮�
  NULL,                         // ���� switch
  NULL,                         // ������� MAP-䠩��
  NULL,                         // ��ப� -> ����
  NULL,                         // �஢�ઠ �� ᬥ饭�� � �⥪�
  NULL,                         // ᮧ����� �३�� �㭪樨
#if IDP_INTERFACE_VERSION > 37
  NULL,							// Get size of function return address in bytes (2/4 by default)
#endif
  NULL,                         // ᮧ����� ��ப� ���ᠭ�� �⥪���� ��६�����
  NULL,                         // ������� ⥪�� ��� ....
  0,                            // Icode ��� ������� ������
  NULL,                         // ��।�� ��権 � IDP
  NULL,							// Is the instruction created only for alignment purposes?
  NULL                          // micro virtual mashine
#if IDP_INTERFACE_VERSION > 37
  ,0							// fixup bit's
#endif
};
