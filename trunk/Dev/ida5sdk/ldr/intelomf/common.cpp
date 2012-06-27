
//-----------------------------------------------------------------------
bool is_intelomf_file(linput_t *li)
{
  uchar magic;
  lmh h;
  qlseek(li, 0);
  if ( qlread(li, &magic, sizeof(magic)) != sizeof(magic)
    || qlread(li, &h, sizeof(h)) != sizeof(h) ) return false;
  int fsize = qlsize(li);
  return magic == INTELOMF_MAGIC_BYTE
      && h.tot_length < fsize;
}

//-----------------------------------------------------------------------
static int read_pstring(linput_t *li, char *name, int size)
{
  char buf[256];
  uchar nlen;
  lread(li, &nlen, sizeof(nlen));
  lread(li, buf, nlen);
  buf[nlen] = '\0';
  qstrncpy(name, buf, size);
  return nlen;
}

//-----------------------------------------------------------------------
static ulong readdw(const uchar *&ptr, bool wide)
{
  ulong x;
  if ( wide )
  {
    x = *(ulong *)ptr;
    ptr += sizeof(ulong);
  }
  else
  {
    x = *(ushort *)ptr;
    ptr += sizeof(ushort);
  }
  return x;
}

