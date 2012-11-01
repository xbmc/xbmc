#ifndef UDF25_H
#define UDF25_H
/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *
 * Note: parts of this code comes from libdvdread.
 * Jorgen Lundman did the necessary modifications to support udf 2.5
 */

#include "File.h"

/**
 * The length of one Logical Block of a DVD.
 */
#define DVD_VIDEO_LB_LEN 2048

/**
 * Maximum length of filenames allowed in UDF.
 */
#define MAX_UDF_FILE_NAME_LEN 2048

struct Partition {
  int valid;
  char VolumeDesc[128];
  uint16_t Flags;
  uint16_t Number;
  char Contents[32];
  uint32_t AccessType;
  uint32_t Start;
  uint32_t Length;
};

struct AD {
  uint32_t Location;
  uint32_t Length;
  uint8_t  Flags;
  uint16_t Partition;
};

/* Previously dvdread would assume files only had one AD chain, and since they
 * are 1GB or less, this is most problably true. However, now we handle chains
 * for large files. ECMA_167 does not specify the maximum number of chains, is
 * it as many as can fit in a 2048 block (minus ID 266 size), or some other
 * limit. For now, I have assumed that;
 * a 4.4GB file uses 5 AD chains. A BluRay disk can store 50GB of data, so the
 * largest file should be 50 GB. So the maximum number of chains should be
 * around 62.
 */

#define UDF_MAX_AD_CHAINS 50

struct FileAD {
    uint64_t Length;
    uint32_t num_AD;
    uint32_t Partition_Start;
    struct AD AD_chain[UDF_MAX_AD_CHAINS];
};

struct extent_ad {
  uint32_t location;
  uint32_t length;
};

struct avdp_t {
  struct extent_ad mvds;
  struct extent_ad rvds;
};

struct pvd_t {
  uint8_t VolumeIdentifier[32];
  uint8_t VolumeSetIdentifier[128];
};

struct lbudf {
  uint32_t lb;
  uint8_t *data;
  /* needed for proper freeing */
  uint8_t *data_base;
};

struct icbmap {
  uint32_t lbn;
  struct FileAD  file;
  uint8_t filetype;
};

struct udf_cache {
  int avdp_valid;
  struct avdp_t avdp;
  int pvd_valid;
  struct pvd_t pvd;
  int partition_valid;
  struct Partition partition;
  int rooticb_valid;
  struct AD rooticb;
  int lb_num;
  struct lbudf *lbs;
  int map_num;
  struct icbmap *maps;
};

typedef enum {
  PartitionCache, RootICBCache, LBUDFCache, MapCache, AVDPCache, PVDCache
} UDFCacheType;

/*
 * DVDReaddir entry types.
 */
typedef enum {
  DVD_DT_UNKNOWN = 0,
  DVD_DT_FIFO,
  DVD_DT_CHR,
  DVD_DT_DIR,
  DVD_DT_BLK,
  DVD_DT_REG,
  DVD_DT_LNK,
  DVD_DT_SOCK,
  DVD_DT_WHT
} udf_dir_type_t;

/*
 * DVDReaddir structure.
 * Extended a little from POSIX to also return filesize.
 */
typedef struct {
  unsigned char  d_name[MAX_UDF_FILE_NAME_LEN];
  // "Shall not exceed 1023; Ecma-167 page 123"
  udf_dir_type_t d_type;       // DT_REG, DT_DIR
  unsigned int   d_namlen;
  uint64_t       d_filesize;
} udf_dirent_t;


/*
 * DVDOpendir DIR* structure
 */
typedef struct {
  uint32_t dir_location;
  uint32_t dir_length;
  uint32_t dir_current;   // Separate to _location should we one day want to
                          // implement dir_rewind()
  unsigned int current_p; // Internal implementation specific. UDFScanDirX
  udf_dirent_t entry;
} udf_dir_t;



typedef struct FileAD *UDF_FILE;

typedef struct _BD_FILE
{
  UDF_FILE file;
  uint64_t seek_pos;  // in bytes
  uint64_t filesize;  // in bytes

} *BD_FILE;


class udf25
{

public:
  udf25( );
  virtual ~udf25( );

  DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod );
  int64_t GetFileSize(HANDLE hFile);
  int64_t GetFilePosition(HANDLE hFile);
  int64_t Seek(HANDLE hFile, int64_t lOffset, int whence);
  HANDLE OpenFile( const char* filename );
  long ReadFile(HANDLE fd, unsigned char *pBuffer, long lSize);
  void CloseFile(HANDLE hFile);

  udf_dir_t *OpenDir( const char *subdir );
  udf_dirent_t *ReadDir( udf_dir_t *dirp );
  int CloseDir( udf_dir_t *dirp );

  void Reset();
  void Scan();
  bool IsScanned();

private:
  UDF_FILE UDFFindFile( const char* filename, uint64_t *filesize );
  int UDFScanDirX( udf_dir_t *dirp );
  void UDFFreeFile(UDF_FILE file);
  int DVDUDFCacheLevel(int level);
  void* GetUDFCacheHandle();
  void SetUDFCacheHandle(void *cache);
  int GetUDFCache(UDFCacheType type,uint32_t nr, void *data);
  int UDFFindPartition( int partnum, struct Partition *part );
  int UDFGetAVDP( struct avdp_t *avdp);
  int DVDReadLBUDF( uint32_t lb_number, size_t block_count, unsigned char *data, int encrypted );
  int UDFReadBlocksRaw( uint32_t lb_number, size_t block_count, unsigned char *data, int encrypted );
  int UDFMapICB( struct AD ICB, uint8_t *FileType, struct Partition *partition, struct FileAD *File );
  int UDFScanDir( struct FileAD Dir, char *FileName, struct Partition *partition, struct AD *FileICB, int cache_file_info);
  int SetUDFCache(UDFCacheType type, uint32_t nr, void *data);
protected:
    /* Filesystem cache */
  int m_udfcache_level; /* 0 - turned off, 1 - on */
  void *m_udfcache;
  XFILE::CFile* m_fp;
};

#endif
