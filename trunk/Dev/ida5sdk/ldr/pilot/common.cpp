
#include <malloc.h>

//-------------------------------------------------------------------------
static void swap_prc(DatabaseHdrType &h)
{
  h.attributes         = swap16(h.attributes);
  h.version            = swap16(h.version);
  h.creationDate       = swap32(h.creationDate);
  h.modificationDate   = swap32(h.modificationDate);
  h.lastBackupDate     = swap32(h.lastBackupDate);
  h.modificationNumber = swap32(h.modificationNumber);
  h.appInfoID          = swap32(h.appInfoID);
  h.sortInfoID         = swap32(h.sortInfoID);
//  h.type             = swap32(h.type);
//  h.id               = swap32(h.id);
  h.uniqueIDSeed       = swap32(h.uniqueIDSeed);
  h.nextRecordListID   = swap32(h.nextRecordListID);
  h.numRecords         = swap16(h.numRecords);
}

//-------------------------------------------------------------------------
static void swap_resource_map_entry(ResourceMapEntry &re)
{
  re.id       = swap16(re.id);
  re.ulOffset = swap32(re.ulOffset);
}

//-------------------------------------------------------------------------
void swap_bitmap(pilot_bitmap_t *b)
{
  b->cx = swap16(b->cx);
  b->cy = swap16(b->cy);
  b->cbRow = swap16(b->cbRow);
  b->ausUnk[0] = swap16(b->ausUnk[0]);
  b->ausUnk[1] = swap16(b->ausUnk[1]);
  b->ausUnk[2] = swap16(b->ausUnk[2]);
  b->ausUnk[3] = swap16(b->ausUnk[3]);
}

//-------------------------------------------------------------------------
void swap_code0000(code0000_t *cp)
{
  cp->data_size = swap32(cp->data_size);
  cp->bss_size  = swap32(cp->bss_size);
}

//-------------------------------------------------------------------------
void swap_pref0000(pref0000_t *pp)
{
  pp->flags      = swap16(pp->flags);
  pp->stack_size = swap32(pp->stack_size);
  pp->heap_size  = swap32(pp->heap_size);
}

//-------------------------------------------------------------------------
// Since the Palm Pilot programs are really poorly recognized by usual
// methods, we are forced to read the resource tablee to determine
// if everying is ok
bool is_prc_file(linput_t *li)
{
  DatabaseHdrType h;
  if ( qlread(li,&h,sizeof(h)) != sizeof(h) ) return false;
  swap_prc(h);
  if ( (h.attributes & dmHdrAttrResDB) == 0 ) return false;
  if ( short(h.numRecords) <= 0 ) return false;
  const ulong filesize = qlsize(li);
  const ulong lowestpos = h.numRecords*sizeof(ResourceMapEntry) + sizeof(h);
  if ( lowestpos > filesize ) return false;

  // the dates can be plain wrong, so don't check them:
  //ulong now = time(NULL);
  //&& ulong(h.lastBackupDate) <= now    // use unsigned comparition!
  //&& ulong(h.creationDate) <= now      // use unsigned comparition!
  //&& ulong(h.modificationDate) <= now  // use unsigned comparition!

  size_t size = sizeof(ResourceMapEntry) * h.numRecords;
  ResourceMapEntry *re = (ResourceMapEntry *)alloca(size);
  if ( re == NULL ) return false;
  if ( qlread(li,re,size) != size ) return false;
  for ( int i=0; i < h.numRecords; i++ )
  {
    swap_resource_map_entry(re[i]);
    if ( re[i].ulOffset >= filesize || re[i].ulOffset < lowestpos )
      return false;
  }

  return true;
}

