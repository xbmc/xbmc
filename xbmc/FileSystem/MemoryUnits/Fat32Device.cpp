#include "stdafx.h"
#include "Fat32Device.h"
#include "xbox/Undocumented.h"

using namespace std;

static int cacheHit = 0;
static int cacheMiss = 0;

#define IRP_MJ_READ                     0x02
#define IRP_MJ_WRITE                    0x03

#define FAT_VOLUME_NAME_LENGTH          32
#define FAT_ONLINE_DATA_LENGTH          2048

CFat32Device::CFat32Device(unsigned long port, unsigned long slot, void *device)
: IDevice(port, slot, device)
{
  m_sectorsize = SECTOR_SIZE;
  memset(&m_volume, 0, sizeof(m_volume));
}

CFat32Device::~CFat32Device()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" Killing our cache");
  CSingleLock lock(m_criticalSection);
  // nothing we can do at this stage except kill our cache
  for (FATCACHE::iterator it = m_cache.begin(); it != m_cache.end(); it++)
    delete (*it).second;
  m_cache.clear();
}

void CFat32Device::LogInfo()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" Volume label '%-11.11s'", m_volume.label);
  CLog::Log(LOGDEBUG, __FUNCTION__" Separate label '%s'", m_volumeName.c_str());
  CLog::Log(LOGDEBUG, __FUNCTION__" %d sector/s per cluster, %d reserved sector/s, volume total %d sectors.", m_volume.secperclus, m_volume.reservedsecs, m_volume.numsecs);
  CLog::Log(LOGDEBUG, __FUNCTION__" %d sectors per FAT, first FAT at sector #%d, root dir at #%d.",m_volume.secperfat,m_volume.fat1,m_volume.rootdir);
  CLog::Log(LOGDEBUG, __FUNCTION__" (For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)");
  CLog::Log(LOGDEBUG, __FUNCTION__" %d root dir entries, data area commences at sector #%d.",m_volume.rootentries,m_volume.dataarea);
  CLog::Log(LOGDEBUG, __FUNCTION__" %d clusters (%d bytes) in data area, filesystem IDd as %s", m_volume.numclusters, m_volume.numclusters * m_volume.secperclus * m_sectorsize, GetFileSystem());
}

const char *CFat32Device::GetFileSystem()
{
  if (m_volume.filesystem == FAT12)
    return "FAT12";
  else if (m_volume.filesystem == FAT16)
    return "FAT16";
  else if (m_volume.filesystem == FAT32)
    return "FAT32";
  return "Unknown";
}

void CFat32Device::CachePage(unsigned long page, unsigned char *buffer)
{
  DWORD time = timeGetTime();
  if (m_cache.size() >= CACHE_SIZE)
  { // remove the last used cache
    FATCACHE::iterator lastUsed = m_cache.begin();
    //CStdString usage;
    unsigned long smallest = 0;
    for (FATCACHE::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
    {
      if ((*it).second->Usage(time) > (*lastUsed).second->Usage(time))
        lastUsed = it;
      if ((*it).second->Usage(time) < smallest)
        smallest = (*it).second->Usage(time);
/*      CStdString log;
      if ((*it).second->Usage(time) > 0xFF)
        log = "FF ";
      else
        log.Format("%02x ", (*it).second->Usage());
      usage += log;*/
    }
    //CLog::Log(LOGDEBUG, "%s", usage.c_str());
    // lastUsed is the one we remove
    unsigned long oldpage = (*lastUsed).first;
    CSector *sector = (*lastUsed).second;
    m_cache.erase(lastUsed);
//    CLog::Log(LOGDEBUG, "Swapping page %i for page %i, oldest usage: %d (%d)", oldpage, page, sector->Usage(time), smallest);
#ifdef FAT32_ALLOW_WRITING
    // write it out to disk if dirty
    if (sector->IsDirty())
      WritePage(oldpage, sector->Get());
#endif
    delete sector;
  }
  // cache our page
  m_cache.insert(pair<unsigned int, CSector *>(page, new CSector(buffer, time)));
}

bool CFat32Device::ReadFromCache(unsigned long sector, unsigned char *buffer)
{
  // round sector size down to page size
  unsigned long page = sector & ~(FAT_PAGE_SIZE / m_sectorsize - 1);
  FATCACHE::iterator it = m_cache.find(page);
  if (it != m_cache.end())
  {
    cacheHit++;
    CSector *cacheSector = (*it).second;
    cacheSector->IncrementUsage(timeGetTime());
    fast_memcpy(buffer, cacheSector->Get() + (sector - page)*m_sectorsize, m_sectorsize);
    return true;
  }
  return false;
}

#ifdef FAT32_ALLOW_WRITING
bool CFat32Device::WriteToCache(unsigned long sector, unsigned char *buffer)
{
  // round sector size down to page size
  unsigned long page = sector & ~(FAT_PAGE_SIZE / m_sectorsize - 1);
  FATCACHE::iterator it = m_cache.find(page);
  if (it != m_cache.end())
  {
    cacheHit++;
    CSector *cacheSector = (*it).second;
    cacheSector->IncrementUsage();
    cacheSector->SetDirty(true);
    fast_memcpy(cacheSector->Get() + (sector - page)*m_sectorsize, buffer, m_sectorsize);
    return true;
  }
  return false;
}
#endif

void CFat32Device::FlushCache()
{
  CSingleLock lock(m_criticalSection);
  // destroy our FAT cache
  for (FATCACHE::iterator it = m_cache.begin(); it != m_cache.end(); it++)
  {
#ifdef FAT32_ALLOW_WRITING
    // if dirty, write to disk
    if ((*it)->IsDirty())
    {
      WritePage((*it).first, (*it).second->Get());
    }
#endif
    delete (*it).second;
  }
  m_cache.clear();
}

#ifdef FAT32_ALLOW_WRITING
void CFat32Device::FlushWriteCache()
{
  CSingleLock lock(m_criticalSection);
  // write our cache sectors to disk
  for (FATCACHE::iterator it = m_cache.begin(); it != m_cache.end(); it++)
  { // if dirty, write to disk
    if ((*it)->IsDirty() && WritePage((*it)->Page(), (*it)->Get()))
      (*it)->SetDirty(false);
  }
}
#endif

bool CFat32Device::ReadPage(unsigned long page, unsigned char *buffer)
{
	LARGE_INTEGER StartingOffset;
	StartingOffset.QuadPart = (__int64)page * m_sectorsize;
	NTSTATUS status = IoSynchronousFsdRequest(IRP_MJ_READ, (PDEVICE_OBJECT)m_device, buffer, FAT_PAGE_SIZE, &StartingOffset);
  if (NT_SUCCESS(status))
    return true;
  CLog::Log(LOGERROR, __FUNCTION__" Error reading page %d : %08x", page, status);
  return false;
}

#ifdef FAT32_ALLOW_WRITING
bool CFat32Device::WritePage(unsigned long page, unsigned char *buffer)
{
	LARGE_INTEGER StartingOffset;
	StartingOffset.QuadPart = (__int64)page * m_sectorsize;
	NTSTATUS status = IoSynchronousFsdRequest(IRP_MJ_WRITE, (PDEVICE_OBJECT)m_device, buffer, FAT_PAGE_SIZE, &StartingOffset);
  if (NT_SUCCESS(status))
    return true;
  CLog::Log(LOGERROR, __FUNCTION__" Error writing page %d : %08x", page, status);
  return false;
}
#endif

unsigned long CFat32Device::ReadSector(unsigned char *buffer, unsigned long sector, unsigned long count)
{
  CSingleLock lock(m_criticalSection);

  // check our cache(s) first
  if (ReadFromCache(sector, buffer))
    return 0;

  cacheMiss++;
  unsigned long page = sector & ~(FAT_PAGE_SIZE / m_sectorsize - 1);
  BYTE tempBuffer[FAT_PAGE_SIZE];
  if (ReadPage(page, tempBuffer))
  {
    CachePage(page, tempBuffer);
    fast_memcpy(buffer, tempBuffer + (sector - page) * m_sectorsize, m_sectorsize);
//    CLog::MemDump(buffer, SECTOR_SIZE);
    return 0;
  }

  CLog::Log(LOGERROR, __FUNCTION__" Error reading sector %u from fat (port %u, slot %u)", sector, m_port, m_slot);
  return 1;
}

unsigned long CFat32Device::WriteSector(unsigned char *buffer, unsigned long sector, unsigned long count)
{
#ifdef FAT32_ALLOW_WRITING
  CSingleLock lock(m_criticalSection);

  // check our cache(s) first
  if (WriteToCache(sector, buffer))
    return 0;

  cacheMiss++;
  // we first have to read the appropriate amount?
  unsigned long page = sector & ~(FAT_PAGE_SIZE / m_sectorsize - 1);
  unsigned char temp[FAT_PAGE_SIZE];
  if (ReadPage(page, temp))
    CachePage(page, temp);
  if (WriteToCache(sector, buffer))
    return 0;

  CLog::Log(LOGERROR, __FUNCTION__" Error writing sector %u from fat (port %u, slot %u)", sector, m_port, m_slot);
#endif
  return 1;
}

void CFat32Device::ReadVolumeName()
{
  DIRINFO di;
  DIRENT de;
  BYTE buffer[FAT_PAGE_SIZE];
  di.scratch = (uint8_t*)&buffer;
	if (DFS_OpenDir(&m_volume, (uint8_t*)"", &di))
		return;

	while (!DFS_GetNext(&m_volume, &di, &de))
  {
    if (de.attr == ATTR_VOLUME_ID)
    {
      char volume[12];
      strncpy(volume, (char *)de.name, 11);
      volume[11] = 0;
      m_volumeName = volume;
      return;
    }
  }
}

bool CFat32Device::Mount(const char *device)
{
  // first try and read the disk geometry
  CLog::Log(LOGDEBUG, __FUNCTION__" Reading MU disk geometry");
  DISK_GEOMETRY DiskGeometry;
  NTSTATUS Status = IoSynchronousDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
          (PDEVICE_OBJECT)m_device, NULL, 0, &DiskGeometry, sizeof(DISK_GEOMETRY),
          NULL, FALSE);

  if (!NT_SUCCESS(Status))
    return false;

  CLog::Log(LOGDEBUG, __FUNCTION__" Reading MU partition info");
  PARTITION_INFORMATION PartitionInformation;
  Status = IoSynchronousDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
          (PDEVICE_OBJECT)m_device, NULL, 0, &PartitionInformation,
          sizeof(PARTITION_INFORMATION), NULL, FALSE);

  if (!NT_SUCCESS(Status))
    return false;

  uint8_t sector[FAT_PAGE_SIZE];

  // guess sector size to start with
  m_sectorsize = 512;

  // Dump first 32k of image to make sure we're reading valid data (yuck!)
  InitialReadForSlowCards(64);

  // Clear our cache
  FlushCache();

  CLog::Log(LOGDEBUG, __FUNCTION__" Reading first sector to determine type");
  uint32_t lbrSector = 0;
  int ret = CheckSectorForLBR(lbrSector);
  if (ret)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" First sector failed LBR test (%i)", ret);
    // read the MBR
	  uint32_t psize;
	  uint8_t pactive, ptype;
    // 0 is the partition number
    lbrSector = DFS_GetPtnStart((unsigned long)this, sector, 0, &pactive, &ptype, &psize);
    if (lbrSector == 0xFFFFFFFF)
    { // fail
      CLog::Log(LOGDEBUG, __FUNCTION__" No MBR found - assuming none exists");
      lbrSector = 0;
    }

    // look for the first lbr, sadly we don't know the correct
    // sector size, so we have to guess
    for(;m_sectorsize<=4096;m_sectorsize*=2)
    {
      if (!CheckSectorForLBR(lbrSector))
        break;
      FlushCache();
    }


    if(m_sectorsize<=4096)
      CLog::Log(LOGDEBUG, __FUNCTION__" Partition 0 start sector 0x%-08.8lX active %-02.2hX type %-02.2hX size %-08.8lX\n", lbrSector, pactive, ptype, psize);
    else
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" No LBR found - this can't be FAT12/16/32");
      return false;
    }
  }

  if (DFS_GetVolInfo((unsigned long)this, sector, lbrSector, &m_volume))
    return false;

  m_sectorsize = m_volume.sectorsize;
  CLog::Log(LOGDEBUG, __FUNCTION__" Sector size of 0x%-08.8lX detected", m_sectorsize);

  ReadVolumeName();
  LogInfo();

  return true;
}

#define IsPowerOfTwo(x) !((x-1) & x)

int CFat32Device::CheckSectorForLBR(unsigned long offset)
{
  BYTE sector[FAT_PAGE_SIZE];

	PLBR lbr = (PLBR) sector;

	if (ReadSector(sector,offset,1))
    return -1;

  // ok, let's do some checks
  // check 1: number of fats nonzero
  if (lbr->bpb.numfats == 0)
    return 1;
  // check 2: cluster size a power of 2
  if (!IsPowerOfTwo(lbr->bpb.secperclus))
    return 2;
  // check 3: logical sector size is a power of 2, > 512
  unsigned short bytespersec = *(unsigned short *)&lbr->bpb.bytepersec_l;
  if (!IsPowerOfTwo(bytespersec) || bytespersec < 512)
    return 3;
  // check 4: number of rootdir entries is a multiple of number of dirs per logical sector
  //int rootEntries = (((int)lbr->bpb.rootentries_h) << 8) + lbr->bpb.rootentries_l;
  //lbr->bpb.
  // check 5: disk geometry has nonzero number of heads and sectors per track
  if (*(unsigned short *)&lbr->bpb.heads_l == 0)
    return 5;
  if (*(unsigned short *)&lbr->bpb.secpertrk_l == 0)
    return 5;
  // check 6: number of reserved sectors non-zero
  if (*(unsigned short *)&lbr->bpb.reserved_l == 0)
    return 6;
  // check 7: media descripter is f0 or f8->ff
  if ((lbr->bpb.mediatype & 0xf0) != 0xf0)
    return 7;
  if ((lbr->bpb.mediatype & 0x0f) != 0 && (lbr->bpb.mediatype & 0x08) != 0x08)
    return 7;

  // ok :)
  return 0;
}

void CFat32Device::InitialReadForSlowCards(unsigned long sectors)
{
  BYTE buffer[FAT_PAGE_SIZE];
  for (unsigned int i = 0; i < sectors; i++)
  {
    if (ReadSector(buffer, i, 1))
      break;  // error
  }
}
