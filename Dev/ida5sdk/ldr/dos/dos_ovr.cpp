/*
 *  This Loader Module is written by Ilfak Guilfanov and
 *                        rewriten by Yury Haron
 *
 */
/*
  L O A D E R  pard of MS-DOS file format's (overlayed EXE)
*/

#include "../idaldr.h"
#include  <struct.hpp>
#include <exehdr.h>
#include "dos_ovr.h"

//------------------------------------------------------------------------
typedef struct {
  ushort fb;
#define FB_MAGIC 0x4246
  ushort ov;
#define OV_MAGIC 0x564F
  ulong ovrsize;
  ulong exeinfo;
  long segnum;
}fbov_t;

typedef struct {
  ushort seg;
  ushort maxoff;                // FFFF - unknown
  ushort flags;
#define SI_COD  0x0001
#define SI_OVR  0x0002
#define SI_DAT  0x0004
  ushort minoff;
}seginfo_t;

typedef struct {
  uchar CDh;        // 0
  uchar intnum;     // 1
  ushort memswap;   // 2
  long fileoff;     // 4
  ushort codesize;      // 8
  ushort relsize;   // 10
  ushort nentries;      // 12
  ushort prevstub;      // 14
#define STUBUNK_SIZE            (0x20-0x10)
  uchar unknown[STUBUNK_SIZE];
}stub_t;

typedef struct {
  ushort int3f;
  ushort off;
  char segc;
}ovrentry_t;

typedef struct {
  uchar   CDh;
  uchar   intnum;   //normally 3Fh
  ushort  ovr_index;
  ushort  entry_off;
}ms_entry;


static char stub_class[]    = "STUBSEG",
            stub_name_fmt[] = "stub%03d",
            ovr_class[]     = "OVERLAY",
            ovr_name_fmt[]  = "ovr%03d";

//------------------------------------------------------------------------
static ulong ovr_off = 0;

//------------------------------------------------------------------------
o_type PrepareOverlayType(linput_t *li, exehdr *E)
{
  ulong   flen    = qlsize(li),
          base    = E->HdrSize * 16,
          loadend = base + E->CalcEXE_Length(),
          fbovoff;
  fbov_t  fbov;
  exehdr  e1;

  ovr_off = 0;

  for(fbovoff = (loadend + 0xF) & ~0xF; ; fbovoff += 0x10) {
    if(pos_read(li, fbovoff, &fbov, sizeof(fbov)) ||
       fbov.fb != FB_MAGIC) break;
    if(fbov.ov == OV_MAGIC) {
      ovr_off = fbovoff;
      return((fbov.exeinfo > loadend ||
              fbov.ovrsize > (flen-fbovoff) ||
              fbov.segnum <= 0) ? ovr_pascal : ovr_cpp);
    }
  }
  if(!pos_read(li, fbovoff = (loadend + 511) & ~511, &e1, sizeof(e1)) &&
     e1.exe_ident == EXE_ID &&  //only MZ !
     (flen -= fbovoff) >= (base = e1.HdrSize*16) &&
     e1.TablOff + (e1.ReloCnt*4) <= (flen -= base) &&
     e1.CalcEXE_Length() <= flen) {
    ovr_off = fbovoff;
    return(ovr_ms);
  }
  return(ovr_noexe);
}

//------------------------------------------------------------------------
static int isStubPascal(ea_t ea)
{
  return(get_byte(ea)          == 0xCD    &&
         get_byte(ea+1)        == 0x3F    &&
         (long)get_long(ea+4)   > 0       &&    // fileoff
         get_word(ea+8)        != 0       &&    // codesize
         (short)get_word(ea+10) >= 0      &&    // relsize (assume max 32k)
         (short)get_word(ea+12) > 0       &&    // nentries
         (short)get_word(ea+12) <
            (0x7FFF / sizeof(ovrentry_t)) &&    // nentries
         isEnabled(toEA(inf.baseaddr + get_word(ea+14), 0))); // prevstub
}

//------------------------------------------------------------------------
linput_t *CheckExternOverlays(void)
{
  char buf[MAXSTR];
  const char *p;
  if ( get_input_file_path(buf, sizeof(buf)) <= 0
    || (p=strrchr(buf, '.')) == NULL
    || stricmp(++p, e_exe) != 0 )
  {
    return NULL;
  }

  for(int i = 0; i < segs.get_area_qty(); i++) {
    segment_t *seg = getnseg(i);

    ea_t ea = seg->startEA;
    if(isStubPascal(ea)) {
      switch(askyn_c(0,
                "This file contains reference to Pascal-stype overlays\n"
                "\3Do you want to load it?")) {

        case 0:   //No
          return(NULL);
        case -1:  //Cancel
          loader_failure(NULL);
        default:  //Yes
          break;
      }
      for( ; ; ) {
        p = askfile_c(0, set_file_ext(buf, sizeof(buf), buf, "ovr"),
                                    "Please enter pascal overlays file");
        CheckCtrlBrk();
        if(!p) return(NULL);

        linput_t *li = open_linput(p, false);
        if(li) return(li);
        warning("Pascal style overlays file '%s' is not found", p);
      }
    }
  }
  return(NULL);
}

//------------------------------------------------------------------------
static void removeBytes(void)
{
  ea_t ea = inf.ominEA;

  msg("Deleting bytes which do not belong to any segment...\n");
  for(int i=0; ea < inf.omaxEA; i++) {
    segment_t *sptr = getnseg(i);

    if(ea < sptr->startEA) {
      showAddr(ea);
      deb(IDA_DEBUG_LDR,
          "Deleting bytes at %a..%a (they do not belong to any segment)...\n",
          ea, sptr->startEA);
      if(FlagsDisable(ea,sptr->startEA)) {
        warning("Maximal number of segments is reached, some bytes are out of segments");
        return;
      }
      CheckCtrlBrk();
    }
    ea = sptr->endEA;
  }
}

//------------------------------------------------------------------------
static int add_struc_fld(struc_t *st, flags_t flag, size_t sz,
                                        const char *name, const char *cmt)
{
  int i = add_struc_member(st, name, BADADDR, flag, NULL, sz);
  if(!i && cmt) set_member_cmt(get_member_by_name(st, name), cmt, false);
  return(i);
}
//------------------------------------------------------
static void describeStub(ea_t stubEA)
{
  static const char stubSname[] = "_stub_descr";
  static tid_t id = 0;

  struc_t *st;

  if(!id &&
     (id = get_struc_id(stubSname)) == BADNODE) {
    if((st = get_struc(add_struc(BADADDR, stubSname))) == NULL) goto badst;
    st->props |= SF_NOLIST;
    if(add_struc_fld(st, byteflag()|hexflag(), 2,
                     "int_code", "Overlay manager interrupt")     ||
       add_struc_fld(st, wordflag()|hexflag(), sizeof(short),
                     "memswap", "Runtime memory swap address")    ||
       add_struc_fld(st, dwrdflag()|hexflag(), sizeof(long),
                     "fileoff", "Offset in the file to the code") ||
       add_struc_fld(st, wordflag()|hexflag(), sizeof(short),
                     "codesize", "Code size")                     ||
       add_struc_fld(st, wordflag()|hexflag(), sizeof(short),
                     "relsize", "Relocation area size")           ||
       add_struc_fld(st, wordflag()|decflag(), sizeof(short),
                     "nentries", "Number of overlay entries")     ||
       add_struc_fld(st, wordflag()|segflag(), sizeof(short),
                     "prevstub", "Previous stub")                 ||
       add_struc_fld(st, byteflag()|hexflag(), STUBUNK_SIZE,
                     "workarea", NULL)) {
badst:
      warning("Can't create stub structure descriptor");
      id = BADNODE;
    } else {
      array_parameters_t apt;
      apt.flags = AP_ALLOWDUPS;
      apt.lineitems = 8;
      apt.alignment = -1; // nonalign
      set_array_parameters(get_member(st, 0)->id, &apt);
      set_array_parameters(get_member(st,offsetof(stub_t,unknown))->id, &apt);
//      st->props |= SF_NOLIST;
//      save_struc(st);
      id = st->id;
    }
  }

  ushort tmp = get_word(stubEA + offsetof(stub_t, prevstub));
  if(tmp) put_word(stubEA + offsetof(stub_t, prevstub), tmp + inf.baseaddr);

  tmp = get_word(stubEA + offsetof(stub_t, nentries));

  if(id != BADNODE)
  {
    do_unknown_range(stubEA, sizeof(stub_t), true);
    doStruct(stubEA, sizeof(stub_t), id);
  }

  stubEA += sizeof(stub_t);

  if(tmp) do {
#if 0
    showAddr(stubEA);
    ua_code(stubEA);

    func_t fn;
    clear_func_struct(&fn);
    fn.flags |= FUNC_FAR;
    fn.startEA = stubEA;
#else
    auto_make_proc(stubEA);
#endif
    stubEA += 5;
#if 0
    fn.endEA   = stubEA;
    add_func(&fn);
#endif
    CheckCtrlBrk();
  } while(--tmp);
}

//------------------------------------------------------------------------
static void load_overlay(linput_t *li, ulong exeinfo, ea_t stubEA,
                                                  segment_t *s, long fboff)
{
  ea_t entEA = stubEA + sizeof(stub_t);
  stub_t stub;

  if(!get_many_bytes(stubEA, &stub, sizeof(stub))) errstruct();
  msg("Overlay stub at %a, code at %a...\n", stubEA, s->startEA);
  if(stub.CDh != 0xCD) errstruct();   //i bad stub

            //i now load overlay code:
  int waszero = 0;
  if(!stub.codesize) {   //i IDA  doesn't allow 0 length segments
    stub.codesize = 1;
    waszero = 1;
  }
  s->endEA = s->startEA + stub.codesize;
  file2base(li, fboff+stub.fileoff, s->startEA, s->endEA,
            fboff == 0 ? FILEREG_NOTPATCHABLE : FILEREG_PATCHABLE);
  if(waszero) {
    s->type = SEG_NULL;
    stub.codesize = 0;
  }

  int i;
  for(i = 0; i < stub.nentries; i++) {
    showAddr(entEA);
    put_byte(entEA, 0xEA);     // jmp far
    ushort offset = get_word(entEA+2);
    put_word(entEA+1, offset); // offset
    put_word(entEA+3, s->sel); // selector
    autoMark(toEA(ask_selector(s->sel), offset), AU_PROC);
    entEA += sizeof(ovrentry_t);
    CheckCtrlBrk();
  }

  qlseek(li, fboff + stub.fileoff + stub.codesize);

  fixup_data_t fd;
  fd.type = FIXUP_SEG16;
  fd.off  = 0;
  fd.displacement = 0;

  int relcnt = stub.relsize / 2;
  if(relcnt) {
    ushort *relb = qnewarray(ushort, relcnt);
    if(!relb) nomem("overlay relocation table");

    lread(li, relb, sizeof(ushort)*relcnt);
    long pos = qltell(li); //must??

    ushort *relc = relb;
    do {
      if(*relc > stub.codesize) errstruct();

      ea_t xEA = s->startEA + *relc++;
      showAddr(xEA);
      ushort relseg = get_word(xEA);
      if(exeinfo) {
        seginfo_t si;

        if(pos_read(li, exeinfo+relseg, &si, sizeof(si))) errstruct();
        relseg = si.seg;
      }

      fd.sel  = relseg + inf.baseaddr;
      set_fixup(xEA, &fd);
      put_word(xEA, fd.sel);
      CheckCtrlBrk();
    } while(--relcnt);
    qfree(relb);
    qlseek(li, pos);
  }
}

//------------------------------------------------------------------------
static sel_t AdjustStub(ea_t ea) // returns prev stub
{
  segment_t *seg = getseg(ea);

  if(ea != seg->startEA) add_segm(ea>>4, ea, BADADDR, NULL, NULL);

  ushort nentries = get_word(ea+12);
  ulong segsize = sizeof(stub_t) + nentries * sizeof(ovrentry_t);
  seg = getseg(ea);

  asize_t realsize = seg->endEA - seg->startEA;
  if(segsize > realsize) return(BADSEL);      // this stub is bad

  if(segsize != realsize) {
    ea_t next = seg->startEA + segsize;

    set_segm_end(seg->startEA,next,0);
    next += 0xF;
    next &= ~0xF;
    if(isEnabled(next)) {
      segment_t *s = getseg(next);
      if(!s) add_segm(next>>4, next, BADADDR, NULL, NULL);
    }
  }
  return(get_word(ea+14));
}

//------------------------------------------------------------------------
void LoadPascalOverlays(linput_t *li)
{
//AdjustPascalOverlay
  for(ea_t ea = inf.minEA; ea < inf.maxEA; ) {
    ea &= ~0xF;
    if(isStubPascal(ea)) {
      AdjustStub(ea);
      ea = getseg(ea)->endEA;
      ea += 0xF;
      CheckCtrlBrk();
    } else ea += 0x10;
  }
//-
  for(int i=0; i < segs.get_area_qty(); i++) {
    segment_t *seg = getnseg(i);
    ea_t ea = seg->startEA;

    if(get_byte(ea) != 0xCD || get_byte(ea+1) != 0x3F ) continue;
    set_segm_class(seg, stub_class);
    set_segm_name(seg, stub_name_fmt, i);

    segment_t s;
    s.align   = saRelByte;
    s.comb    = scPub;
    s.align   = saRelPara;
    s.sel = setup_selector((s.startEA = (inf.maxEA + 0xF) & ~0xF) >> 4);
    // 04.06.99 ig: what is exeinfo and why it is passed as 0 here?
    load_overlay(li, 0/*???*/, ea, &s, ovr_off); //i
    add_segm_ex(&s, NULL, ovr_class, ADDSEG_NOSREG|ADDSEG_OR_DIE);
    set_segm_name(&s, ovr_name_fmt, i);
    describeStub(ea);
    CheckCtrlBrk();
  }
  removeBytes();
}

//------------------------------------------------------------------------
static ea_t CppInfoBase(fbov_t *fbov)
{
  seginfo_t si;
  ea_t      siEA = get_fileregion_ea(fbov->exeinfo);

  if(siEA == BADADDR ||
     !get_many_bytes(siEA, &si, sizeof(si))) errstruct();

  if((si.flags & SI_OVR) && si.seg) { //possible trucate
    ushort lseg = si.seg;

    msg("Probbly the input file was truncated by 'unp -h'. Searching the base...\n");
    do {
      if(si.seg > lseg) errstruct();
      lseg = si.seg;

      if(siEA < inf.ominEA+sizeof(si) ||
         !get_many_bytes(siEA -= sizeof(si), &si, sizeof(si))) errstruct();
      fbov->exeinfo -= sizeof(si);
      CheckCtrlBrk();
    } while(si.seg);
    add_pgm_cmt("Real (before unp -h) EXEinfo=%08lX", fbov->exeinfo);
  }
  return(siEA);
}

//------------------------------------------------------------------------
sel_t LoadCppOverlays(linput_t *li)
{
  fbov_t fbov;
  sel_t  dseg = BADSEL;

  if(pos_read(li, ovr_off, &fbov, sizeof(fbov))) errstruct();
  add_pgm_cmt("Overlays: base=%08lX, size=%08lX, EXEinfo=%08lX",
              ovr_off, fbov.ovrsize, fbov.exeinfo);
  ovr_off += sizeof(fbov_t);

  if(!fbov.segnum) errstruct();

  ea_t    siEA = CppInfoBase(&fbov);
  ushort  lseg = 0;
  for(long i = 0; i < fbov.segnum; i++) {
    seginfo_t si;

    if(!get_many_bytes(siEA, &si, sizeof(si))) errstruct();
    siEA += sizeof(si);

    if(si.maxoff == 0xFFFF) continue;  //i skip EXEINFO & OVRDATA
    if(si.maxoff <= si.minoff) continue;
    if(si.seg < lseg) errstruct();
    lseg = si.seg;

    si.seg += inf.baseaddr;

    char      *sclass  = NULL;
    segment_t s;      //i initialize segment_t with 0s
    s.align  = saRelByte;
    s.comb   = scPub;
    if(si.seg == inf.start_ss) {
      sclass = CLASS_STACK;
      s.type = SEG_DATA;
      s.comb = scStack;
    }
    if(si.flags & SI_COD) {
      sclass = CLASS_CODE;
      s.type = SEG_CODE;
    }
    if(si.flags & SI_DAT) {
      sclass = CLASS_BSS;
      s.type = SEG_DATA;
      dseg   = si.seg;
    }
    s.name = 0;
    if(si.flags & SI_OVR) {
      s.align = saRelPara;
      s.sel = setup_selector((s.startEA = (inf.maxEA + 0xF) & ~0xF) >> 4);
                                            //i endEA is set in load_overlay()
      load_overlay(li, fbov.exeinfo, toEA(si.seg, 0), &s, ovr_off);
      if (s.type != SEG_NULL) s.type  = SEG_CODE;
      add_segm_ex(&s, NULL, ovr_class, ADDSEG_NOSREG|ADDSEG_OR_DIE);
      set_segm_name(&s, ovr_name_fmt, i);
      s.name = 0;
      s.type = SEG_NORM;        // undefined segment type
      sclass = stub_class;
    }
    s.sel     = si.seg;
    s.startEA = toEA(s.sel, si.minoff);
    s.endEA   = toEA(s.sel, si.maxoff);
    add_segm_ex(&s, NULL, sclass, ADDSEG_NOSREG|ADDSEG_OR_DIE);
    if(si.flags & SI_OVR) {
      describeStub(s.startEA);
      set_segm_name(&s, stub_name_fmt, i);
    }
    CheckCtrlBrk();
  }
  removeBytes();
  return(dseg);
}

//------------------------------------------------------------------------
//+
//------------------------------------------------------------------------
static netnode msnode;

typedef struct {
  ulong   bpos, size;
  ushort  Toff, Hsiz, Rcnt, Mpara;
} modsc_t;

static ea_t ref_off_EA, ref_ind_EA;
static int  ref_oi_cnt;

//------------------------------------------------------------------------
static int CreateMsOverlaysTable(linput_t *li, bool *PossibleDynamic)
{
  modsc_t o;
  int     Count = 0;
  ulong   flen = qlsize(li);

  o.bpos = ovr_off;
  msnode.create();
  msg("Searching for the overlays in the file...\n");
  while(o.bpos + sizeof(exehdr) < flen) {
    exehdr  E;
    ulong   delta;

    if(pos_read(li, o.bpos, &E, sizeof(E))) errstruct();

    o.size = E.CalcEXE_Length();
    delta = (ulong)(o.Hsiz = E.HdrSize) * 16;
    o.Toff = E.TablOff;
    o.Rcnt = E.ReloCnt;
    o.Mpara = (ushort)((o.size + 0xF) >> 4);

    ulong ost = flen - o.bpos;
    if(E.exe_ident != EXE_ID                            ||  //only MZ !
       ost < delta                                      ||
       (ulong)o.Toff + (E.ReloCnt*4) > (ost -= delta)   ||
       o.size > ost) return(Count);

    CheckCtrlBrk();

    msnode.supset(++Count, &o, sizeof(o));
    o.bpos = ((ovr_off = o.bpos + delta + o.size) + 511) & ~511;
    if(E.Overlay != Count) *PossibleDynamic = false;
  }
  ovr_off = 0;
  return(Count);
}

//------------------------------------------------------------------------
static void LoadMsOvrData(linput_t *li, int Count, bool Dynamic)
{
  fixup_data_t fd;

  fd.type         = FIXUP_SEG16;
  fd.off          = 0;
  fd.displacement = 0;

  for(int i = 1; i <= Count; i++) {
    modsc_t o;

    if ( msnode.supval(i, &o, sizeof(o)) <= 0 )
      continue;  // skip dropped overlays

    segment_t s;
    s.comb    = scPub;
    s.align   = saRelPara;
    s.sel = setup_selector((s.startEA = (inf.maxEA + 0xF) & ~0xF) >> 4);
    msnode.altset(i, s.sel);
    s.endEA = s.startEA + ((ulong)o.Mpara << 4);
    add_segm_ex(&s, NULL, ovr_class, ADDSEG_NOSREG|ADDSEG_OR_DIE);
    set_segm_name(&s, ovr_name_fmt, i);
    file2base(li, o.bpos + o.Hsiz*16, s.startEA, s.startEA + o.size,
                                                          FILEREG_PATCHABLE);

    qlseek(li, o.bpos + o.Toff);

    for(int j = o.Rcnt; j; j--) {
      unsigned short buf[2];

      lread(li, buf, sizeof(buf));

//ATTENTION!!! if Dynamic (ms-autopositioning) segment part of relocation
//             address == psedudodata segment  to load (from data in ovr!)
//             его надо проверять, но пока Ильфак решил обойтись :)
      ea_t xEA = s.startEA + (Dynamic ? buf[0] : toEA(buf[1], buf[0]));

      if(xEA >= s.endEA) errstruct();

      showAddr(xEA);

      ushort ubs = ushort(get_word(xEA) + inf.baseaddr);
      put_word(xEA, ubs);
      fd.sel  = ubs;
      set_fixup(xEA, &fd);
      add_segm_by_selector(ubs, CLASS_CODE);
      CheckCtrlBrk();
    }
  }
}

//------------------------------------------------------------------------
static sel_t SearchMsOvrTable(int *Cnt)
{
  ea_t dstea, from = inf.minEA;
  modsc_t dsc;
  msnode.supval(1, &dsc, sizeof(dsc));
  ulong src = dsc.bpos;
  int  AddSkip, Count = *Cnt;
  int  i, j; // watcom мать его...

  msg("Searching the overlay reference data table...\n");
  while(from + sizeof(ulong) < inf.maxEA) {
    ea_t sea, ea = bin_search(from, inf.maxEA, (uchar *)&src, NULL, sizeof(src),
                               BIN_SEARCH_FORWARD, BIN_SEARCH_CASE);
    if(CheckCtrlBrk()) continue;
    if(ea == BADADDR) break;

    segment_t *s = getseg(ea);
    if(!s || ea - s->startEA < sizeof(ulong)*(Count+1) ||
       get_long(sea = ea - sizeof(ulong)) ||
       ea + (2 * Count * sizeof(ulong)) > s->endEA) {
      from = ea + sizeof(ulong);
      continue;
    }
    AddSkip = 0;
    for(i = 2; i <= Count + AddSkip; i++) {
      ulong pos = get_long(ea += sizeof(ulong));

      if(!pos) {
        ++AddSkip;
        if(ea + (2 * (Count+AddSkip-i) * sizeof(ulong)) >= s->endEA) break;
      }
      else
      {
        modsc_t tmp;
        msnode.supval(i - AddSkip, &tmp, sizeof(tmp));
        if(pos != tmp.bpos) break;
      }
      from = ea;
    }
    if(from != ea) {
      from = ea;
      continue;
    }

    if(AddSkip) {
      ea = sea + sizeof(ulong);
      for(i = 2; i <= Count; i++) {
        ea += sizeof(ulong);
        if(get_long(ea)) continue;
        if(!AddSkip) goto experr;
        --AddSkip;
        for(j = Count; j >= i; j--)
        {
          modsc_t modsc;
          if ( msnode.supval(j, &modsc, sizeof(modsc)) >= 0 )
            msnode.supset(j+1, &modsc, sizeof(modsc));
        }
        msnode.supdel(i);
        ++Count;
        CheckCtrlBrk();
      }
      if(AddSkip) {
experr:
        warning("Internal error in msOverlay expand");
        goto badtable;
      }
    }

//msg("Found disk blocks table\n");
    src = 0;
    from = bin_search(s->startEA, sea - ((Count-1) * sizeof(ushort)),
                      (uchar *)&src, NULL, sizeof(ushort), BIN_SEARCH_BACKWARD,
                      BIN_SEARCH_CASE | BIN_SEARCH_NOBREAK);
    if(from == BADADDR || (sea - from) % sizeof(ushort)) break;
//\  for(from = sea - ((Count-1) * sizeof(ushort)); from >= s->startEA;
//\      from -= sizeof(ushort)) if(!get_word(from)) break;
//\  if(from < s->startEA) break;

    ref_ind_EA = from;
    if((ref_oi_cnt = (int)((sea - from) / 2)) <= 1) goto badtable;

//msg("Check all tables...\n");
    for(ea = ref_ind_EA; ea < sea; ea += sizeof(ushort)) {
      unsigned val = get_word(ea);

      if(val > Count) {
        if(Count == *Cnt) goto badtable;
        AddSkip = val - Count;
        if((ref_oi_cnt -= (AddSkip*2)) <= 1) goto badtable;
        sea -= (AddSkip * sizeof(ulong));
        for(j = Count; j; j--) {
          modsc_t tmp;
          if ( msnode.supval(j, &tmp, sizeof(tmp)) >= 0 )
            msnode.supset(j+AddSkip, &tmp, sizeof(tmp));
          else
            msnode.supdel(j+AddSkip);
        }
        for(j = 1; j <= AddSkip; j++) msnode.supdel(j);
        Count += AddSkip;
        CheckCtrlBrk();
      }
    }

    ref_off_EA = ref_ind_EA - (ref_oi_cnt * sizeof(ushort));

    ea_t ea_r = ref_off_EA, ea_i = ref_ind_EA;
    for(i = 1; i < ref_oi_cnt; i++) {
      ea_r += sizeof(ushort);
      ea_i += sizeof(ushort);

      modsc_t o;
      if ( msnode.supval(get_word(ea_i), &o, sizeof(o)) < 0 )
      {
        if(get_word(ea_r)) goto badtable;
        msg("An overlay index %d in the table of indexes points to a missing overlay\n", i);
        continue;
      }

      if(get_word(ea_r) >= o.size)
      {
        static bool accept = false;
        if(!accept) {
          ask_for_feedback("Incompatible offsets table");
          accept = true;
        }
      }
    }

    dstea = sea;
    sea += (Count+1) * sizeof(ulong);
    for(i = 1; i <= Count; i++) {
      modsc_t o;
      bool ok = msnode.supval(i, &o, sizeof(o)) >= 0;
      ulong dt = get_long(sea += sizeof(ulong));

      if(!ok != !dt) goto badtable;
      if(!ok) continue;

      if(dt < o.Mpara || dt >= 0x1000) {
        static int accept = 0;
        if(!accept) {
          ask_for_feedback("Incompatible mem-size table");
          accept = 1;
        }
      }
      if(dt > o.Mpara) o.Mpara = dt;
    }

    msg("All tables OK\n");
    *Cnt = Count;
    Count = (Count + 1) * sizeof(ulong);
    doWord(ref_off_EA, ref_oi_cnt * sizeof(ushort));
    do_name_anyway(ref_off_EA, "ovr_off_tbl");
    doWord(ref_ind_EA, ref_oi_cnt * sizeof(ushort));
    do_name_anyway(ref_ind_EA, "ovr_index_tbl");
    doDwrd(dstea, Count);
    do_name_anyway(dstea, "ovr_start_tbl");
    dstea += Count;
    doDwrd(dstea, Count);
    do_name_anyway(dstea, "ovr_memsiz_tbl");

    return(s->sel);
  }
badtable:
  ref_oi_cnt = -1;
  return(BADSEL);
}

//------------------------------------------------------------------------
static segment_t * MsOvrStubSeg(int *stub_cnt, ea_t r_top, sel_t dseg)
{
  int   cnt;
  uchar frs;

  msg("Searching for the stub segment...\n");
  for(int i = 0; i < segs.get_area_qty(); i++) {
    segment_t *seg = getnseg(i);
    if(seg->sel == dseg) continue;
    ea_t ea = seg->startEA;
    uchar buf[3*sizeof(ushort)];

    if(ea >= r_top) break;

    if(!get_many_bytes(ea, buf, sizeof(buf))) continue;
    if(*(ulong *)buf || *(ushort *)&buf[sizeof(ulong)]) continue;

    cnt = 0;
    while((ea += 3*sizeof(ushort)) < seg->endEA - 3*sizeof(ushort)) {
      if((frs = get_byte(ea)) == 0) break;
      ushort ind = get_word(ea + 2);
      if(get_byte(ea)          != 0xCD ||
         get_byte(ea+1)        != 0x3F ||
         !ind || ind > ref_oi_cnt) break;
      ++cnt;
      CheckCtrlBrk();
    }
    if(!frs && cnt >= ref_oi_cnt) {
      *stub_cnt = cnt;
      return(seg);
    }
  }
  return(NULL);
}

//------------------------------------------------------------------------
static void CreateMsStubProc(segment_t *s, int stub_cnt)
{
  ea_t ea = s->startEA;

  set_segm_name(s, "STUB");
  set_segm_class(s, CLASS_CODE);
  doByte(ea, 3*sizeof(ushort));
  ea += 3*sizeof(ushort);
  msg("Patching the overlay stub-segment...\n");
  for(int i = 0; i < stub_cnt; i++, ea += 3*sizeof(ushort)) {
    int ind = get_word(ea+2) * sizeof(ushort);

    if(!ind) continue;

    int ovr = get_word(ref_ind_EA + ind),
        off = get_word(ea+4) + get_word(ref_off_EA + ind);
    modsc_t o;
    bool ok = msnode.supval(ovr, &o, sizeof(o)) >= 0;
    ushort sel = msnode.altval(ovr);

    if(!ok) error("Internal error");

    if(off >= o.size) {
      static int accept = 0;
      if(!accept) {
        ask_for_feedback("Illegal offset in int 3F");
        accept = 1;
      }
    } else {
      showAddr(ea);
      put_byte(ea, 0xEA);   // jmp far
      put_word(ea+1, off);  // offset
      put_word(ea+3, sel);  // selector
      put_byte(ea+5, 0x90); //NOP -> for autoanalisis
      autoMark(ea, AU_PROC);
      autoMark(toEA(ask_selector(sel), off), AU_PROC);
      CheckCtrlBrk();
    }
  }
  doAlign(ea, s->endEA - ea, 0);
}

//------------------------------------------------------------------------
sel_t LoadMsOverlays(linput_t *li, bool PossibleDynamic)
{
  sel_t dseg = BADSEL;
  int   Cnt  = CreateMsOverlaysTable(li, &PossibleDynamic);

  if(ovr_off) warning("File has extra information\n"
                      "\3Loading 0x%X bytes, total file size 0x%X",
                      ovr_off, qlsize(li));

  if(Cnt) {
    dseg = SearchMsOvrTable(&Cnt);
    if(dseg != BADSEL) PossibleDynamic = false;
    else if(!PossibleDynamic)
      ask_for_feedback("Can't find the overlay call data table");

    ea_t r_top = inf.maxEA;
    LoadMsOvrData(li, Cnt, PossibleDynamic);

    if(ref_oi_cnt != -1) {
      int       stub_cnt;
      segment_t *s = MsOvrStubSeg(&stub_cnt, r_top, dseg);

      if(s) CreateMsStubProc(s, stub_cnt);
      else  ask_for_feedback("The overlay-manager segment not found");
    }
  }
  msnode.kill();
  return(dseg);
}

//------------------------------------------------------------------------


