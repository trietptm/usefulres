//--------------------------------------------------------------------------
int get_aout_file_format_index(linput_t *li)
{
  exec ex;
  register int i = 0;
  if(qlread(li, &ex, sizeof(ex)) != sizeof(ex)) return false;

  if(N_BADMAG(ex)) {
    ex.a_info = swap32(ex.a_info);
    switch(N_MACHTYPE(ex)) {
      case M_386_NETBSD:
      case M_68K_NETBSD:
      case M_68K4K_NETBSD:
      case M_532_NETBSD:
      case M_SPARC_NETBSD:
      case M_PMAX_NETBSD:
      case M_VAX_NETBSD:
      case M_ALPHA_NETBSD:
      case M_ARM6_NETBSD:
        break;

      default:
        return false;
    }
  }

  switch(N_MAGIC(ex)) {
    case NMAGIC:
      ++i;
    case CMAGIC:
      ++i;
    case ZMAGIC:
      ++i;
    case OMAGIC:
      ++i;
    case QMAGIC:
//      msg("text=%d data=%d symsize=%d txtoff=%d sum=%d\n", ex.a_text, ex.a_data,
//                N_SYMSIZE(ex), N_TXTOFF(ex), ex.a_text + ex.a_data + N_SYMSIZE(ex) + N_TXTOFF(ex));
      if ( qlsize(li) >= ex.a_text + ex.a_data + N_SYMSIZE(ex) + N_TXTOFF(ex) )
        break;
      if ( N_MAGIC(ex) == ZMAGIC
        && qlsize(li) >= ex.a_text + ex.a_data + N_SYMSIZE(ex) )
      {
        i = 5; // OpenBSD demand-paged
        break;
      }
    default:
      return false;
  }

  return i+1;
}

