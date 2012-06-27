
static const uchar bpt_code[] = { 0xCC };  // breakpoint instruction opcode

// Hardware breakpoints

static ea_t hwbpt_ea[MAX_BPT];
static ulong dr6;
static ulong dr7;

//--------------------------------------------------------------------------
// returns -1 if something is wrong
static int find_hwbpt_slot(ea_t ea)
{
  for ( int i=0; i < MAX_BPT; i++ )
  {
    if ( hwbpt_ea[i] == ea ) // another breakpoint is here
      return -1;
    if ( hwbpt_ea[i] == BADADDR ) // empty slot found
      return i;
  }
  return -1;
}

#ifdef __NT__
//--------------------------------------------------------------------------
// Set hardware breakpoint for one thread
static bool set_hwbpts(HANDLE hThread)
{
//  sure_suspend_thread(ti);
  CONTEXT Context;
  Context.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_CONTROL;

  int ok = GetThreadContext(hThread, &Context);
  if ( !ok )
  {
    deberr("GetThreadContext");
    return false;
  }
  Context.Dr0 = hwbpt_ea[0];
  Context.Dr1 = hwbpt_ea[1];
  Context.Dr2 = hwbpt_ea[2];
  Context.Dr3 = hwbpt_ea[3];
  Context.Dr6 = 0;
  Context.Dr7 = dr7;

  ok = SetThreadContext(hThread, &Context);
  if ( !ok )
    deberr("SetThreadContext");
//  sure_resume_thread(ti);
  return ok;
}

//--------------------------------------------------------------------------
static bool add_hwbpt(bpttype_t type, ea_t ea, int len)
{
  int i = find_hwbpt_slot(ea);      // get slot number
  if ( i != -1 )
  {
    hwbpt_ea[i] = ea;

    int lenc = 0;                   // length code used by the processor
//    if ( len == 1 ) lenc = 0;
    if ( len == 2 ) lenc = 1;
    if ( len == 4 ) lenc = 3;

    dr7 |= (1 << (i*2));            // enable local breakpoint
    dr7 |= (type << (16+(i*4)));    // set breakpoint type
    dr7 |= (lenc << (18+(i*4)));    // set breakpoint length

    return refresh_hwbpts();
  }
  return false;
}

//--------------------------------------------------------------------------
static bool del_hwbpt(ea_t ea)
{
  for ( int i=0; i < MAX_BPT; i++ )
  {
    if ( hwbpt_ea[i] == ea )
    {
      hwbpt_ea[i] = BADADDR;            // clean the address
      dr7 &= ~(3 << (i*2));             // clean the enable bits
      dr7 &= ~(0xF << (i*4+16));        // clean the length and type
      return refresh_hwbpts();
    }
  }
  return false;
}

//--------------------------------------------------------------------------
static ea_t is_hwbpt_triggered(thread_id_t id)
{
  CONTEXT Context;
  Context.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_CONTROL;
  HANDLE h = get_thread_handle(id);
  if ( GetThreadContext(h, &Context) )
  {
    for ( int i=0; i < MAX_BPT; i++ )
    {
      if ( (Context.Dr7 & ulong(1 << (i*2)))
        && (Context.Dr6 & ulong(1 << i)) )  // Local hardware breakpoint 'i'
      {
        cpuregtype_t *dr = NULL;
        switch (i)
        {
          case 0: dr = &Context.Dr0; break;
          case 1: dr = &Context.Dr1; break;
          case 2: dr = &Context.Dr2; break;
          case 3: dr = &Context.Dr3; break;
        }
        if (! dr) break;
        if (hwbpt_ea[i] == *dr)
        {
          set_hwbpts(h);             // Clear the status bits
          return hwbpt_ea[i];
        }
//? TRACING                else
//                  debdeb("System hardware breakpoint at %08X ???\n", *dr); //?
          // what to do ?:
          // reset it, and continue as if no event were received ?
          // send it to IDA, and let the user setup a "stop on non-debugger hardware breakpoint" option ?
      }
    }
  }
  return BADADDR;
}
#endif // ifdef __NT__

//--------------------------------------------------------------------------
int idaapi remote_is_ok_bpt(bpttype_t type, ea_t ea, int /* len */)
{
  if ( type == BPT_SOFT )
    return true;
  return find_hwbpt_slot(ea) == -1 ? BPT_TOO_MANY : BPT_OK;
}

//--------------------------------------------------------------------------
static void cleanup_hwbpts(void)
{
  for ( int i=0; i < MAX_BPT; i++ )
    hwbpt_ea[i] = BADADDR;
  dr6 = 0;
  dr7 = 0x100;          // exact local breakpoints
}

