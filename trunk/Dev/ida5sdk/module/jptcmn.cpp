
// common file to handle jump tables

#include <limits.h>

//#define JUMP_DEBUG
//----------------------------------------------------------------------
enum jump_table_type_t
{
  JT_NONE,        // No jump table
  JT_FLAT32,      // Flat 32-bit jump table
  JT_ARM_LDRB,    // pc + byte(table[i]) * 2
  JT_ARM_LDRH,    // pc + word(table[i]) * 2
};

// Class to check for a jump table sequence.
// This class should be used in preference to the hard encoding of jump table sequences
// because it allows for:
//      - instruction rescheduling
//      - intermingling the jump sequence with other instructions
//      - sequence variants
//
// For this class:
//   all instructions of the sequence are numbered starting from the last instruction.
//   The last instruction has the number 0.
//   The instruction before the last instruciton has the number 1, etc.
//   There is a virtual function jpiN() for each instruction of the sequence
//   These functions return true if 'cmd' is filled with the required instruction
//
// The comparison is made in the match() function:
//
//   ea points to the last instruction of the sequence (instruction #0)
//
//   the 'depends' array contains dependencies between the instructions of the sequence.
//   For example:
//      ARM thumb LDRH switch
//      7 SUB     Ra, #minv (optional)
//      6 CMP     Ra, #size
//      5 BCS     defea
//      4 ADR     Rb, jt
//      3 ADD     Rb, Rb, Ra
//      2 LDRH    Rb, [Rb,Ra]
//      1 LSL     Rb, Rb, #1
//      0 ADD     PC, Rb
//   In this sequence, instruction #0 depends on the value of Rb which is produced
//   by the instruction #1. So, the instruction #0 depends on #1. Therefore, depends[0]
//   will contain '1' as its element.
//   The instruction #2 depends on 3 registers: Ra and Rb, or in other words,
//   it depends on the instructions #4 and #6. Therefore, depends[2] will contain { 4, 6 }
//   Maximum 2 dependencies per instruction are allowed.
//
//   The 'roots' array contains the first instruction of the dependency chains.
//   In our case we can say that there are 2 dependency chains:
//      0 -> 1 -> 2 -> 3 -> 4
//                       -> 6 -> 7
//      5 -> 6 -> 7
//   Therefore the roots array will consist of {1, 5}.
//   0 denotes the end of the chain and can not be the root of a dependency chain
//   Usually 1 is a root of any jump sequence.
//
//   The dependencies array allows to check for optimized sequences of instrucitons.
//   If 2 instructions are not dependent on each other, they may appear in any order.
//   (for example, the instruction #4 and the instruction sequence #5-6-7 may appear
//   in any order because they do not depend on each other)
//   Also any other instructions not modifying the register values may appear between
//   the instructions of the sequence (due to the instruction rescheduling performed
//   by the compiler).
//
//   Provision for optional instructions:
//   The presence of an optional instruction in the sequence (like #7) is signalled
//   by a negative number of the dependency in the 'depends' array.
//
//   Provision for variable instructions:
//   In some cases several variants of the same instructions may be supported.
//   For example, the instruction #5 might be BCS as well as BGE. It is the job of
//   the jpi5() function to check for all variants.
//
//   Provision to skip some instructions of the sequence:
//   Sometimes one variant of the instruction might mean that a previous instruction
//   must be missing. For example, the instructions #5, #6 might look like
//
//       Variant 1   Variant 2   Variant 3
//    6  BCC label
//    5  B defea     BGE defea   BCS defea
//   label:
//
//   Then jpi5() must behave like this:
//      if the instruction in 'cmd' is 'BSC' or 'BGE'
//        then skip instruction #6. For this:
//              skip[6] = true;
//      if the instruction in 'cmd' is 'B'
//              remember defea; return true;
//   And jpi6() must behave like this:
//      check if the instruction in 'cmd' is 'BCC' and jump to the end of instruction #5
//
// In order to use the 'jump_pattern_t' class you should derive another class from it
// and define the jpiN() virtual functions.
// Then you have to define the 'depends' and 'roots' arrays and call the match()
// function.
// If you processor contains instructions who modify registers in peculiar ways
// you might want to override the check_spoiled() function.

class jump_pattern_t
{
public:
  typedef bool (jump_pattern_t::*check_insn_t)(void);
  jump_pattern_t(void);

protected:
  ea_t minea;

#define NINS 16         // the maximum length of the sequence
  ea_t eas[NINS];
  bool skip[NINS];
  check_insn_t check[NINS];
  int r[16];
  bool spoiled[16];

  const char (*depends)[2];     // positive numbers - instruction on which we depend
                                // negative means the dependence is optional,
                                //   the other instruction might be missing

  virtual void check_spoiled(void);
  void spoil(int reg);
  bool follow_tree(ea_t ea, int n);

  virtual bool jpi0(void) = 0;
  virtual bool jpi1(void) { return false; }
  virtual bool jpi2(void) { return false; }
  virtual bool jpi3(void) { return false; }
  virtual bool jpi4(void) { return false; }
  virtual bool jpi5(void) { return false; }
  virtual bool jpi6(void) { return false; }
  virtual bool jpi7(void) { return false; }
  virtual bool jpi8(void) { return false; }
  virtual bool jpi9(void) { return false; }
  virtual bool jpia(void) { return false; }
  virtual bool jpib(void) { return false; }
  virtual bool jpic(void) { return false; }
  virtual bool jpid(void) { return false; }
  virtual bool jpie(void) { return false; }
  virtual bool jpif(void) { return false; }

public:
  ea_t *base;
  ea_t *table;
  ea_t *defea;
  int *minv;
  int *size;

  bool match(ea_t ea,
             const char *roots,
             const char _depends[][2]);
};


//----------------------------------------------------------------------
#ifdef JUMP_DEBUG
inline void jmsg(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vmsg(format, va);
  va_end(va);
}
#else
inline void jmsg(const char *, ...) {}
#endif

//----------------------------------------------------------------------
inline jump_pattern_t::jump_pattern_t(void) :
  minea(0),
  base(NULL),
  table(NULL),
  defea(NULL),
  minv(NULL),
  size(NULL)
{
}

//----------------------------------------------------------------------
void jump_pattern_t::spoil(int reg)
{
  for ( int i=0; i < qnumber(spoiled); i++ )
    if ( r[i] == reg )
      spoiled[i] = true;
}

//----------------------------------------------------------------------
void jump_pattern_t::check_spoiled(void)
{
  ulong F = cmd.get_canon_feature();
  if ( F != 0 )
  {
    for ( int i=0; i < UA_MAXOP; i++ )
    {
      if ( (F & (CF_CHG1<<i)) == 0 )
        continue;
      op_t &x = cmd.Operands[i];
      switch ( x.type )
      {
        case o_reg:
          spoil(x.reg);
          break;
      }
    }
  }
}

//----------------------------------------------------------------------
bool jump_pattern_t::follow_tree(ea_t ea, int n)
{
  if ( n == 0 ) return true;
  bool success = false;
  if ( n < 0 )
  {
    success = true;
    n = -n;
  }
  jmsg("follow_tree(%a, %d)\n", ea, n);
  if ( !skip[n] )
  {
    if ( eas[n] == BADADDR )
    {
      cmd.ea = ea;
      memset(spoiled, 0, sizeof(spoiled));
      while ( true )
      {
        if ( cmd.ea < minea )
          return success;
        if ( decode_prev_insn(cmd.ea) == BADADDR )
          return success;
        if ( (this->*check[n])() ) break;
        jmsg("%a: can't be %d.", cmd.ea, n);
        jmsg(" rA=%d%s rB=%d%s rC=%d%s rD=%d%s rE=%d%s\n",
                        r[1], spoiled[1] ? "*" : "",
                        r[2], spoiled[2] ? "*" : "",
                        r[3], spoiled[3] ? "*" : "",
                        r[4], spoiled[4] ? "*" : "",
                        r[5], spoiled[5] ? "*" : "");
        check_spoiled();
      }
      eas[n] = cmd.ea;
    }
    if ( eas[n] >= ea )
    {
      jmsg("%a: depends on %a\n", ea, eas[n]);
      return success;
    }
    ea = eas[n];
    jmsg("%a: found %d\n", cmd.ea, n);
  }
  if ( depends[n][0] && !follow_tree(ea, depends[n][0]) ) return success;
  if ( depends[n][1] && !follow_tree(ea, depends[n][1]) ) return success;
  jmsg("follow_tree(%d) - ok\n", n);
  return true;
}

//----------------------------------------------------------------------
bool jump_pattern_t::match(ea_t ea,
                           const char *roots,
                           const char (*_depends)[2])
{
  check[0x00] = &jump_pattern_t::jpi0;
  check[0x01] = &jump_pattern_t::jpi1;
  check[0x02] = &jump_pattern_t::jpi2;
  check[0x03] = &jump_pattern_t::jpi3;
  check[0x04] = &jump_pattern_t::jpi4;
  check[0x05] = &jump_pattern_t::jpi5;
  check[0x06] = &jump_pattern_t::jpi6;
  check[0x07] = &jump_pattern_t::jpi7;
  check[0x08] = &jump_pattern_t::jpi8;
  check[0x09] = &jump_pattern_t::jpi9;
  check[0x0a] = &jump_pattern_t::jpia;
  check[0x0b] = &jump_pattern_t::jpib;
  check[0x0c] = &jump_pattern_t::jpic;
  check[0x0d] = &jump_pattern_t::jpid;
  check[0x0e] = &jump_pattern_t::jpie;
  check[0x0f] = &jump_pattern_t::jpif;
  depends= _depends;
  *table = BADADDR;
  *base  = 0;
  *defea = BADADDR;
  *size  = INT_MAX;
  *minv  = 0;
  memset(skip, 0, sizeof(skip));
  memset(eas, -1, sizeof(eas));
  memset(r, -1, sizeof(r));
  eas[0] = ea;
  func_t *pfn = get_fchunk(ea);
  if ( pfn == NULL ) pfn = get_prev_fchunk(ea);
  minea = pfn != NULL ? pfn->startEA : getseg(ea)->startEA;
  if ( !(this->*check[0])() ) return false;
  while ( *roots )
    if ( !follow_tree(eas[0], *roots++) ) return false;
  return true;
}

//----------------------------------------------------------------------
// check and create a flat 32 bit jump table -- the most common case
static void check_and_create_flat32(jump_table_type_t /*jtt*/,
                                    ea_t base,
                                    ea_t table,
                                    ea_t defea,
                                    int minv,
                                    int size)
{
  // check the table contents
  segment_t *s = getseg(table);
  if ( s == NULL ) return;
  int maxsize = s->endEA - table;
  if ( size > maxsize ) size = maxsize;

  int i;
  insn_t saved = cmd;
  for ( i=0; i < size; i++ )
  {
    ea_t ea = table + 4*i;
    flags_t F = getFlags(ea);
    if ( !hasValue(F)
      || (i && (has_any_name(F) || hasRef(F))) ) break;
    ea_t target = base + segm_adjust_diff(getseg(table), get_long(ea));
    if ( !isLoaded(target) ) break;
    flags_t F2 = get_flags_novalue(target);
    if ( isTail(F2)
      || isData(F2)
      || (!isCode(F2) && !ua_ana0(target)) ) break;
  }
  cmd = saved;
  size = i;
  // create the table
  for ( i=0; i < size; i++ )
  {
    ea_t ea = table + 4*i;
    doDwrd(ea, 4);
    op_offset(ea, 0, REF_OFF32, BADADDR, base);
    ea_t target = base + segm_adjust_diff(getseg(table), get_long(ea));
    ua_add_cref(0, target, fl_JN);
  }
  switch_info_t si;
  si.flags  = SWI_J32;
  if ( defea != BADADDR ) si.flags |= SWI_DEFAULT;
  si.ncases = size;
  si.jumps  = table;
  si.lowcase= minv;
  si.startea= cmd.ea;
  si.defjump= defea;
  set_switch_info(cmd.ea, &si);
}

//----------------------------------------------------------------------
typedef jump_table_type_t is_pattern_t(ea_t *base, ea_t *table, ea_t *defea, int *minv, int *size);

// This function finds and creates a 32-bit jump table
static void check_for_table_jump2(is_pattern_t * const patterns[],
                                  size_t qty,
                                  void (*create_table)(jump_table_type_t jtt,
                                                       ea_t base,
                                                       ea_t table,
                                                       ea_t defea,
                                                       int minv,
                                                       int size))
{
  ea_t base, table, defea;
  int minv, size;

  jump_table_type_t jtt = JT_NONE;
  insn_t saved = cmd;
  for ( int i=0; jtt == JT_NONE && i < qty; i++ )
  {
    jmsg("%a: check pattern %d ----\n", cmd.ea, i);
    jtt = patterns[i](&base, &table, &defea, &minv, &size);
    cmd = saved;
  }
  if ( jtt == JT_NONE ) return;

  jmsg("table=%a minv=%d. size=%d. base=%a defea=%a\n", table, minv, size, base, defea);
  if ( table != BADADDR ) table = toEA(cmd.cs, table);
  if ( base  != BADADDR ) base  = toEA(cmd.cs, base);
  if ( defea != BADADDR ) defea = toEA(cmd.cs, defea);

  if ( create_table == NULL )
    create_table = check_and_create_flat32;

  create_table(jtt, base, table, defea, minv, size);

  char buf[MAXSTR];
  qsnprintf(buf, sizeof(buf), "def_%a", cmd.ip);
  set_name(defea, buf, SN_NOWARN|SN_LOCAL);
  qsnprintf(buf, sizeof(buf), "jpt_%a", cmd.ip);
  set_name(table, buf, SN_NOWARN);
//  msg("final size=%d.\n", size);
}

