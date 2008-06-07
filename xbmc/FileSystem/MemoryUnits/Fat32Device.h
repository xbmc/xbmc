#pragma once

#include "IDevice.h"
#include "dosfs.h"
#include "memutil.h"
#include "utils/CriticalSection.h"
#include <map>

// structure for vfat name entries (unicode)
#pragma pack(push,1) 
struct VFAT_DIR_ENTRY
{
  unsigned char sequence;
  unsigned short unicode1[5];
  unsigned char attr_0f;
  unsigned char type_00;
  unsigned char checksum;
  unsigned short unicode2[6];
  unsigned short cluster_0000;
  unsigned short unicode3[2];
};
#pragma pack(pop)

#define FAT_PAGE_SIZE 4096
#define CACHE_SIZE      64  // 64 * 4k = 256k during reads an writes

// class for our sector cache
class CSector
{
public:
  CSector(unsigned char *buffer, unsigned long time) { fast_memcpy(m_buffer, buffer, FAT_PAGE_SIZE); m_usage = 1; m_dirty = false; m_lastUsed = time; };

  unsigned char *Get() { return m_buffer; };
  bool IsDirty() const { return m_dirty; };
  void SetDirty(bool dirty) { m_dirty = dirty; };
  void IncrementUsage(unsigned long time) { m_usage++; m_lastUsed = time; };
  unsigned long Usage(unsigned long time) const { if (m_usage) return (time - m_lastUsed) / m_usage; return (time - m_lastUsed); };

private:
  unsigned long m_usage;
  unsigned long m_lastUsed;
  unsigned char m_buffer[FAT_PAGE_SIZE];
  bool          m_dirty;
};

class CFat32Device : public IDevice
{
public:
  CFat32Device(unsigned long port, unsigned long slot, void *device);
  ~CFat32Device();

  virtual void LogInfo();
  virtual const char *GetFileSystem();
  virtual void UnMount() {};
  virtual bool Mount(const char *device);

  VOLINFO *GetVolume() { return &m_volume; };
#ifdef FAT32_ALLOW_WRITING
  void FlushWriteCache();
#endif

  void ReadVolumeName();

  unsigned long ReadSector(unsigned char *buffer, unsigned long sector, unsigned long count);
  unsigned long WriteSector(unsigned char *buffer, unsigned long sector, unsigned long count);

protected:
  void InitialReadForSlowCards(unsigned long sectors);
  int CheckSectorForLBR(unsigned long lbr);
  
  unsigned long  m_sectorsize;
  VOLINFO        m_volume;

  // FAT caching functions
  bool ReadFromCache(unsigned long sector, unsigned char *buffer);
  void CachePage(unsigned long sector, unsigned char *buffer);
  bool ReadPage(unsigned long page, unsigned char *buffer);
  void FlushCache();
#ifdef FAT32_ALLOW_WRITING
  bool WriteToCache(unsigned long sector, unsigned char *buffer);
  bool WritePage(unsigned long page, unsigned char *buffer);
#endif

  typedef std::map<unsigned long, CSector *> FATCACHE;
  
  FATCACHE m_cache;

  // critical section so that more than 1 thread doesn't screw up our caching
  CCriticalSection m_criticalSection;
};
