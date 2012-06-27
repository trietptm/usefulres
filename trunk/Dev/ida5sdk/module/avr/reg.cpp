/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *      Atmel AVR - 8-bit RISC processor
 *
 */

#include "avr.hpp"
#include <srarea.hpp>
#include <diskio.hpp>
#include <loader.hpp>
#include <entry.hpp>
#include <fpro.h>
#include <ctype.h>

//--------------------------------------------------------------------------

static char *register_names[] =
{
   "r0",   "r1",   "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
   "r8",   "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
  "r16",  "r17",  "r18", "r19", "r20", "r21", "r22", "r23",
  "r24",  "r25",  "r26", "r27", "r28", "r29", "r30", "r31",
  "cs","ds",       // virtual registers for code and data segments
};

//-----------------------------------------------------------------------
//           AVR assembler
//-----------------------------------------------------------------------
static asm_t avrasm =
{
  AS_COLON|AS_N2CHR|ASH_HEXF3|ASD_DECF0|ASB_BINF3|ASO_OCTF0|AS_ONEDUP,
  0,
  "AVR Assembler",
  0,
  NULL,         // header lines
  NULL,         // no bad instructions
  ".org",       // org
  ".exit",      // end

  ";",          // comment string
  '"',          // string delimiter
  '\'',         // char delimiter
  "\"'",        // special symbols in char and string constants

  ".db",        // ascii string directive
  ".db",        // byte directive
  ".dw",        // word directive
  NULL,         // double words
  NULL,         // no qwords
  NULL,         // oword  (16 bytes)
  NULL,         // float  (4 bytes)
  NULL,         // double (8 bytes)
  NULL,         // tbyte  (10/12 bytes)
  NULL,         // packed decimal real
  NULL,         // arrays (#h,#d,#v,#s(...)
  ".byte %s",   // uninited arrays
  ".equ",       // equ
  NULL,         // 'seg' prefix (example: push seg seg001)
  NULL,         // Pointer to checkarg_preline() function.
  NULL,         // char *(*checkarg_atomprefix)(char *operand,void *res); // if !NULL, is called before each atom
  NULL,         // const char **checkarg_operations;
  NULL,         // translation to use in character and string constants.
  NULL,         // current IP (instruction pointer)
  NULL,         // func_header
  NULL,         // func_footer
  NULL,         // "public" name keyword
  NULL,         // "weak"   name keyword
  NULL,         // "extrn"  name keyword
  NULL,         // "comm" (communal variable)
  NULL,         // get_type_name
  NULL,         // "align" keyword
  '(', ')',	// lbrace, rbrace
  NULL,         // mod
  "&",          // and
  "|",          // or
  "^",          // xor
  "~",          // not
  "<<",         // shl
  ">>",         // shr
  NULL,         // sizeof
};

static asm_t *asms[] = { &avrasm, NULL };

//--------------------------------------------------------------------------
static ioport_t *ports = NULL;
static size_t numports = 0;
char device[MAXSTR] = "";
static const char cfgname[] = "avr.cfg";
netnode helper;

ulong ramsize = 0;
ulong romsize = 0;
ulong eepromsize = 0;
ea_t ram = BADADDR;

static bool entry_processing(ea_t &ea1)
{
  helper.altset(ea1, 1);
  ua_code(ea1);
  ea_t ea = get_first_fcref_from(ea1);
  if ( ea != BADADDR ) ea1 = ea;
  return false; // continue processing
}
static const char *avr_callback(const ioport_t *ports, size_t numports, const char *line);
#define callback avr_callback
#define ENTRY_PROCESSING(ea, name, cmt)  entry_processing(ea)
#include "../iocommon.cpp"

//--------------------------------------------------------------------------
static const char *avr_callback(const ioport_t *ports, size_t numports, const char *line)
{
  char word[MAXSTR];
  int addr;
  if ( sscanf(line, "%[^=] = %d", word, &addr) == 2 )
  {
    if ( strcmp(word, "RAM") == 0 )
    {
      ramsize = addr;
      return NULL;
    }
    if ( strcmp(word, "ROM") == 0 )
    {
      romsize = addr;
      return NULL;
    }
    if ( strcmp(word, "EEPROM") == 0 )
    {
      eepromsize = addr;
      return NULL;
    }
  }
  return standard_callback(ports, numports, line);
}

//--------------------------------------------------------------------------
const char *find_port(int address)
{
  const ioport_t *port = find_ioport(ports, numports, address);
  return port ? port->name : NULL;
}

//--------------------------------------------------------------------------
const char *find_bit(int address, int bit)
{
  const ioport_bit_t *b = find_ioport_bit(ports, numports, address, bit);
  return b ? b->name : NULL;
}

//--------------------------------------------------------------------------
static void setup_avr_device(int respect_info)
{
  if ( !choose_ioport_device(cfgname, device, sizeof(device), NULL) )
    return;

  set_device_name(device, respect_info);
  noUsed(0, BADADDR); // reanalyze program

  // resize the ROM segment
  {
    segment_t *s = getseg(helper.altval(-1));
    if ( s == NULL ) s = getnseg(0);  // for the old databases
    if ( s != NULL )
    {
      if ( s->size() > romsize )
        warning("The input file is bigger than the ROM size of the current device");
      set_segm_end(s->startEA, s->startEA+romsize, true);
    }
  }
  // resize the RAM segment
  {
    segment_t *s = get_segm_by_name("RAM");
    if ( s == NULL && ramsize != 0 )
    {
      ea_t start = (inf.maxEA + 0xFFFFF) & ~0xFFFFF;
      add_segm(start>>4, start, start+ramsize, "RAM", "DATA");
      s = getseg(start);
    }
    ram = BADADDR;
    if ( s != NULL )
    {
      int i;
      ram = s->startEA;
      set_segm_end(ram, ram+ramsize, true);
      // set register names
      for ( i=0; i < 32; i++ )
        if ( !has_any_name(get_flags_novalue(ram+i)) )
          set_name(ram+i, register_names[i]);
      // set I/O port names
      for ( i=0; i < numports; i++ )
      {
        ioport_t *p = ports + i;
        set_name(ram+p->address+0x20, p->name);
        set_cmt(ram+p->address+0x20, p->cmt, true);
      }
    }
  }
}

//--------------------------------------------------------------------------
const char *set_idp_options(const char *keyword,int /*value_type*/,const void * /*value*/)
{
  if ( keyword != NULL ) return IDPOPT_BADKEY;
  setup_avr_device(IORESP_INT);
  return IDPOPT_OK;
}

//--------------------------------------------------------------------------
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

  switch(msgid)
  {
    case processor_t::init:
      helper.create("$ atmel");
      break;

    case processor_t::newfile:   // new file loaded
      // remember the ROM segment
      {
        segment_t *s = getnseg(0);
        set_segm_name(getnseg(0), "ROM");
        helper.altset(-1, s->startEA);
      }
      apply_config_file(IORESP_NONE);  // just in case if the user refuses to select
      setup_avr_device(/*IORESP_AREA|*/IORESP_INT); // allow the user to select the device
      // create additional segments
      {
        ea_t start = (inf.maxEA + 0xFFFFF) & ~0xFFFFF;
        if ( eepromsize != 0 )
        {
          char *file = askfile_c(0,"*.bin","Please enter the binary EEPROM image file");
          if ( file != NULL )
          {
            add_segm(start>>4, start, start+eepromsize, "EEPROM", "DATA");
            linput_t *li = open_linput(file, false);
            if ( li != NULL )
            {
              ulong size = qlsize(li);
              if ( size > eepromsize ) size = eepromsize;
              file2base(li, 0, start, start+size, FILEREG_NOTPATCHABLE);
              close_linput(li);
            }
          }
        }
      }
      break;

    case processor_t::oldfile:   // old file loaded
      {
        char buf[MAXSTR];
        if ( helper.supval(-1, buf, sizeof(buf)) > 0 )
          set_device_name(buf, IORESP_NONE);
        segment_t *s = get_segm_by_name("RAM");
        if ( s != NULL ) ram = s->startEA;
      }
      break;

    case processor_t::newprc:    // new processor type
      break;

    case processor_t::newasm:    // new assembler type
      break;

    case processor_t::newseg:    // new segment
      {
        segment_t *s = va_arg(va, segment_t *);
        set_default_dataseg(s->sel);
      }
      break;

    case processor_t::outlabel: // The kernel is going to generate an instruction
                                // label line or a function header
      {
        ea_t ea = va_arg(va, ea_t);
        if ( helper.altval(ea) ) // if entry point
        {
          char buf[MAX_NUMBUF];
          btoa(buf, sizeof(buf), ea);
          printf_line(inf.indent, COLSTR("%s %s", SCOLOR_ASMDIR), ash.origin, buf);
        }
      }
      break;

    case processor_t::move_segm:// A segment is moved
                                // Fix processor dependent address sensitive information
                                // args: ea_t from - old segment address
                                //       segment_t - moved segment
      {
        ea_t from    = va_arg(va, ea_t);
        segment_t *s = va_arg(va, segment_t *);
        helper.altshift(from, s->startEA, s->size()); // move address information
      }
      break;
  }
  va_end(va);
  return 1;
}

//--------------------------------------------------------------------------
// 1001 0101 0xx0 1000     ret
// 1001 0101 0xx1 1000     reti
static uchar retcode_1[] = { 0x08, 0x95 };  // ret
static uchar retcode_2[] = { 0x18, 0x95 };  // reti
static uchar retcode_3[] = { 0x28, 0x95 };  // ret
static uchar retcode_4[] = { 0x38, 0x95 };  // reti
static uchar retcode_5[] = { 0x48, 0x95 };  // ret
static uchar retcode_6[] = { 0x58, 0x95 };  // reti
static uchar retcode_7[] = { 0x68, 0x95 };  // ret
static uchar retcode_8[] = { 0x78, 0x95 };  // reti

static bytes_t retcodes[] =
{
 { sizeof(retcode_1), retcode_1 },
 { sizeof(retcode_2), retcode_2 },
 { sizeof(retcode_3), retcode_3 },
 { sizeof(retcode_4), retcode_4 },
 { sizeof(retcode_5), retcode_5 },
 { sizeof(retcode_6), retcode_6 },
 { sizeof(retcode_7), retcode_7 },
 { sizeof(retcode_8), retcode_8 },
 { 0, NULL }
};

//-----------------------------------------------------------------------
static char *shnames[] = { "AVR", NULL };
static char *lnames[] = {
  "Atmel AVR",
  NULL
};

//-----------------------------------------------------------------------
//      Processor Definition
//-----------------------------------------------------------------------
processor_t LPH =
{
  IDP_INTERFACE_VERSION,        // version
  PLFM_AVR,                     // id
  PRN_HEX|PR_RNAMESOK,
  16,                   // 16 bits in a byte for code segments
  8,                    // 8 bits in a byte for other segments

  shnames,
  lnames,

  asms,

  notify,

  header,
  footer,

  segstart,
  segend,

  NULL,                 // generate "assume" directives

  ana,                  // analyze instruction
  emu,                  // emulate instruction

  out,                  // generate text representation of instruction
  outop,                // generate ...                    operand
  intel_data,           // generate ...                    data directive
  NULL,                 // compare operands
  NULL,                 // can have type

  qnumber(register_names), // Number of registers
  register_names,       // Register names
  NULL,                 // get abstract register

  0,                    // Number of register files
  NULL,                 // Register file names
  NULL,                 // Register descriptions
  NULL,                 // Pointer to CPU registers

  rVcs,                 // first
  rVds,                 // last
  0,                    // size of a segment register
  rVcs, rVds,

  NULL,                 // No known code start sequences
  retcodes,

  AVR_null,
  AVR_last,
  Instructions,

  NULL,                 // int  (*is_far_jump)(int icode);
  NULL,                 // Translation function for offsets
  0,                    // int tbyte_size;  -- doesn't exist
  NULL,                 // int (*realcvt)(void *m, ushort *e, ushort swt);
  { 0, },               // char real_width[4];
                        // number of symbols after decimal point
                        // 2byte float (0-does not exist)
                        // normal float
                        // normal double
                        // long double
  NULL,                 // int (*is_switch)(switch_info_t *si);
  NULL,                 // long (*gen_map_file)(FILE *fp);
  NULL,                 // ulong (*extract_address)(ulong ea,const char *string,int x);
  NULL,                 // int (*is_sp_based)(op_t &x); -- always, so leave it NULL
  NULL,                 // int (*create_func_frame)(func_t *pfn);
  NULL,                 // int (*get_frame_retsize(func_t *pfn)
  NULL,                 // void (*gen_stkvar_def)(char *buf,const member_t *mptr,long v);
  gen_spcdef,           // Generate text representation of an item in a special segment
  AVR_ret,              // Icode of return instruction. It is ok to give any of possible return instructions
  set_idp_options,      // const char *(*set_idp_options)(const char *keyword,int value_type,const void *value);
  NULL,                 // int (*is_align_insn)(ulong ea);
  NULL,                 // mvm_t *mvm;
};
