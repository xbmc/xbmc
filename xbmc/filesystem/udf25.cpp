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
 * Jorgen Lundman and team boxee did the necessary modifications to support udf 2.5
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "system.h"
#include "utils/log.h"
#include "udf25.h"
#include "File.h"

/* For direct data access, LSB first */
#define GETN1(p) ((uint8_t)data[p])
#define GETN2(p) ((uint16_t)data[p] | ((uint16_t)data[(p) + 1] << 8))
#define GETN3(p) ((uint32_t)data[p] | ((uint32_t)data[(p) + 1] << 8)    \
                  | ((uint32_t)data[(p) + 2] << 16))
#define GETN4(p) ((uint32_t)data[p]                     \
                  | ((uint32_t)data[(p) + 1] << 8)      \
                  | ((uint32_t)data[(p) + 2] << 16)     \
                  | ((uint32_t)data[(p) + 3] << 24))
#define GETN8(p) ((uint64_t)data[p]                 \
                  | ((uint64_t)data[(p) + 1] << 8)  \
                  | ((uint64_t)data[(p) + 2] << 16) \
                  | ((uint64_t)data[(p) + 3] << 24) \
                  | ((uint64_t)data[(p) + 4] << 32) \
                  | ((uint64_t)data[(p) + 5] << 40) \
                  | ((uint64_t)data[(p) + 6] << 48) \
                  | ((uint64_t)data[(p) + 7] << 56))

/* This is wrong with regard to endianess */
#define GETN(p, n, target) memcpy(target, &data[p], n)

using namespace XFILE;

static int Unicodedecode( uint8_t *data, int len, char *target )
{
  int p = 1, i = 0;

  if( ( data[ 0 ] == 8 ) || ( data[ 0 ] == 16 ) ) do {
    if( data[ 0 ] == 16 ) p++;  /* Ignore MSB of unicode16 */
    if( p < len ) {
      target[ i++ ] = data[ p++ ];
    }
  } while( p < len );

  target[ i ] = '\0';
  return 0;
}

static int UDFExtentAD( uint8_t *data, uint32_t *Length, uint32_t *Location )
{
  *Length   = GETN4(0);
  *Location = GETN4(4);
  return 0;
}

static int UDFShortAD( uint8_t *data, struct AD *ad,
                       struct Partition *partition )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  ad->Partition = partition->Number; /* use number of current partition */
  return 0;
}

static int UDFLongAD( uint8_t *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  ad->Partition = GETN2(8);
  /* GETN(10, 6, Use); */
  return 0;
}

static int UDFExtAD( uint8_t *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(12);
  ad->Partition = GETN2(16);
  /* GETN(10, 6, Use); */
  return 0;
}


static int UDFICB( uint8_t *data, uint8_t *FileType, uint16_t *Flags )
{
  *FileType = GETN1(11);
  *Flags = GETN2(18);
  return 0;
}

static int UDFPartition( uint8_t *data, uint16_t *Flags, uint16_t *Number,
                         char *Contents, uint32_t *Start, uint32_t *Length )
{
  *Flags = GETN2(20);
  *Number = GETN2(22);
  GETN(24, 32, Contents);
  *Start = GETN4(188);
  *Length = GETN4(192);
  return 0;
}

static int UDFLogVolume( uint8_t *data, char *VolumeDescriptor )
{
  uint32_t lbsize;
  Unicodedecode(&data[84], 128, VolumeDescriptor);
  lbsize = GETN4(212);  /* should be 2048 */
  if (lbsize != DVD_VIDEO_LB_LEN) return 1;
  return 0;
}

static int UDFExtFileEntry( uint8_t *data, uint8_t *FileType,
                            struct Partition *partition, struct FileAD *fad )
{
  uint16_t flags;
  uint32_t L_EA, L_AD;
  unsigned int p;

  UDFICB( &data[ 16 ], FileType, &flags );

  /* Init ad for an empty file (i.e. there isn't a AD, L_AD == 0 ) */
  fad->Length = GETN8(56); // 64-bit.

  L_EA = GETN4( 208);
  L_AD = GETN4( 212);
  p = 216 + L_EA;
  fad->num_AD = 0;
  while( p < 216 + L_EA + L_AD ) {
    struct AD *ad;

    if (fad->num_AD  >= UDF_MAX_AD_CHAINS) return 0;

    ad =  &fad->AD_chain[fad->num_AD];
    ad->Partition = partition->Number;
    ad->Flags = 0;
    fad->num_AD++;

    switch( flags & 0x0007 ) {
    case 0:
        UDFShortAD( &data[ p ], ad, partition );
        p += 8;
        break;
    case 1:
        UDFLongAD( &data[ p ], ad );
        p += 16;
        break;
    case 2:
        UDFExtAD( &data[ p ], ad );
        p += 20;
        break;
    case 3:
        switch( L_AD ) {
        case 8:
            UDFShortAD( &data[ p ], ad, partition );
            break;
        case 16:
            UDFLongAD( &data[ p ],  ad );
            break;
        case 20:
            UDFExtAD( &data[ p ], ad );
            break;
        }
        p += L_AD;
        break;
    default:
        p += L_AD;
        break;
    }
  }
  return 0;

}

/* TagID 266
 * The differences in ExtFile are indicated below:
 * struct extfile_entry {
 *         struct desc_tag         tag;
 *         struct icb_tag          icbtag;
 *         uint32_t                uid;
 *         uint32_t                gid;
 *         uint32_t                perm;
 *         uint16_t                link_cnt;
 *         uint8_t                 rec_format;
 *         uint8_t                 rec_disp_attr;
 *         uint32_t                rec_len;
 *         uint64_t                inf_len;
 *         uint64_t                obj_size;        // NEW
 *         uint64_t                logblks_rec;
 *         struct timestamp        atime;
 *         struct timestamp        mtime;
 *         struct timestamp        ctime;           // NEW
 *         struct timestamp        attrtime;
 *         uint32_t                ckpoint;
 *         uint32_t                reserved1;       // NEW
 *         struct long_ad          ex_attr_icb;
 *         struct long_ad          streamdir_icb;   // NEW
 *         struct regid            imp_id;
 *         uint64_t                unique_id;
 *         uint32_t                l_ea;
 *         uint32_t                l_ad;
 *         uint8_t                 data[1];
 * } __packed;
 *
 * So old "l_ea" is now +216
 *
 * Lund
 */

static int UDFFileEntry( uint8_t *data, uint8_t *FileType,
                         struct Partition *partition, struct FileAD *fad )
{
  uint16_t flags;
  uint32_t L_EA, L_AD;
  unsigned int p;
  unsigned int curr_ad;

  UDFICB( &data[ 16 ], FileType, &flags );

  fad->Length = GETN8( 56 ); /* Was 4 bytes at 60, changed for 64bit. */

  L_EA = GETN4( 168 );
  L_AD = GETN4( 172 );

  if (176 + L_EA + L_AD > DVD_VIDEO_LB_LEN)
    return 0;

  p = 176 + L_EA;
  curr_ad = 0;

  /* Function changed to record all AD chains, not just the last one! */
  while( p < 176 + L_EA + L_AD ) {
    struct AD *ad;

    if (curr_ad >= UDF_MAX_AD_CHAINS) return 0;

    ad =  &fad->AD_chain[curr_ad];
    ad->Partition = partition->Number;
    ad->Flags = 0;
    // Increase AD chain ptr
    curr_ad++;

    switch( flags & 0x0007 ) {
    case 0:
      UDFShortAD( &data[ p ], ad, partition );
      p += 8;
      break;
    case 1:
      UDFLongAD( &data[ p ], ad );
      p += 16;
      break;
    case 2:
      UDFExtAD( &data[ p ], ad );
      p += 20;
      break;
    case 3:
      switch( L_AD ) {
      case 8:
        UDFShortAD( &data[ p ], ad, partition );
        break;
      case 16:
        UDFLongAD( &data[ p ], ad );
        break;
      case 20:
        UDFExtAD( &data[ p ], ad );
        break;
      }
      p += L_AD;
      break;
    default:
      p += L_AD;
      break;
    }
  }
  return 0;
}


// The API users will refer to block 0 as start of file and going up
// we need to convert that to actual disk block;
// partition_start + file_start + offset
// but keep in mind that file_start is chained, and not contiguous.
//
// We return "0" as error, since a File can not start at physical block 0
//
// Who made Location be blocks, but Length be bytes? Can bytes be uneven
// blocksize in the middle of a chain?
//
uint32_t UDFFileBlockPos(struct FileAD *File, uint32_t file_block)
{
  uint32_t result, i, offset;

  if (!File) return 0;

  // Look through the chain to see where this block would belong.
  for (i = 0, offset = 0; i < File->num_AD; i++) {

    // Is "file_block" inside this chain? Then use this chain.
    if (file_block < (offset + (File->AD_chain[i].Length /  DVD_VIDEO_LB_LEN)))
      break;

    offset += (File->AD_chain[i].Length /  DVD_VIDEO_LB_LEN);

  }

  if (i >= File->num_AD)
    i = 0; // This was the default behavior before I fixed things.

  //if(offset == 0)
  //  offset = 32;

  result = File->Partition_Start + File->AD_chain[i].Location + file_block - offset;
  return result;
}

static int UDFFileIdentifier( uint8_t *data, uint8_t *FileCharacteristics,
                              char *FileName, struct AD *FileICB )
{
  uint8_t L_FI;
  uint16_t L_IU;

  *FileCharacteristics = GETN1(18);
  L_FI = GETN1(19);
  UDFLongAD(&data[20], FileICB);
  L_IU = GETN2(36);
  if (L_FI) Unicodedecode(&data[38 + L_IU], L_FI, FileName);
  else FileName[0] = '\0';
  return 4 * ((38 + L_FI + L_IU + 3) / 4);
}

static int UDFDescriptor( uint8_t *data, uint16_t *TagID )
{
  *TagID = GETN2(0);
  /* TODO: check CRC 'n stuff */
  return 0;
}

/**
 * initialize and open a DVD device or file.
 */
static CFile* file_open(const char *target)
{
  CFile* fp = new CFile();

  if(!fp->Open(target))
  {
    CLog::Log(LOGERROR,"file_open - Could not open input");
    delete fp;
    return NULL;
  }

  return fp;
}


/**
 * seek into the device.
 */
static int file_seek(CFile* fp, int blocks)
{
  off64_t pos;

  pos = fp->Seek((off64_t)blocks * (off64_t)DVD_VIDEO_LB_LEN, SEEK_SET);

  if(pos < 0) {
    return (int) pos;
  }
  /* assert pos % DVD_VIDEO_LB_LEN == 0 */
  return (int) (pos / DVD_VIDEO_LB_LEN);
}

/**
 * read data from the device.
 */
static int file_read(CFile* fp, void *buffer, int blocks, int flags)
{
  size_t len;
  ssize_t ret;

  len = (size_t)blocks * DVD_VIDEO_LB_LEN;

  while(len > 0) {

    ret = fp->Read(buffer, len);

    if(ret < 0) {
      /* One of the reads failed, too bad.  We won't even bother
       * returning the reads that went OK, and as in the POSIX spec
       * the file position is left unspecified after a failure. */
      return ret;
    }

    if(ret == 0) {
      /* Nothing more to read.  Return all of the whole blocks, if any.
       * Adjust the file position back to the previous block boundary. */
      size_t bytes = (size_t)blocks * DVD_VIDEO_LB_LEN - len;
      off_t over_read = -(off_t)(bytes % DVD_VIDEO_LB_LEN);
      /*off_t pos =*/ fp->Seek(over_read, SEEK_CUR);
      /* should have pos % 2048 == 0 */
      return (int) (bytes / DVD_VIDEO_LB_LEN);
    }

    len -= ret;
  }

  return blocks;
}

/**
 * close the DVD device and clean up.
 */
static int file_close(CFile* fp)
{
  fp->Close();

  return 0;
}

// offset is in bytes, force_size is in sectors
uint64_t DVDFileSeekForce(BD_FILE bdfile, uint64_t offset, int64_t force_size)
{
  /* Check arguments. */
  if( bdfile == NULL || offset <= 0 )
      return -1;

  // if -1 then round up to the next block
  if( force_size < 0 )
    force_size = (offset + DVD_VIDEO_LB_LEN) - (offset % DVD_VIDEO_LB_LEN);

  if(bdfile->filesize < (uint64_t)force_size ) {
    bdfile->filesize = force_size;
    CLog::Log(LOGERROR, "DVDFileSeekForce - ignored size of file indicated in UDF");
  }

  if( offset > bdfile->filesize )
    return -1;

  bdfile->seek_pos = offset;
  return offset;
}

int UDFSplitIsoFile(const char *fullFilename, char* iso, char* file)
{
  const char* filename = strcasestr(fullFilename, ".iso");
  if(!filename)
    return -1;

  filename += strlen(".iso");
  if(*filename != '/')
    return -1;

  size_t size = strlen(filename);
  memcpy(file, filename, size);
  file[size] = 0;

  size = filename - fullFilename;
  memcpy(iso, fullFilename,  filename - fullFilename);
  iso[size] = 0;

  return 0;
}


int udf25::UDFScanDirX( udf_dir_t *dirp )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint8_t directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
  uint8_t *directory = (uint8_t *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  uint8_t filechar;
  unsigned int p;
  struct AD FileICB;
  struct FileAD File;
  struct Partition partition;
  uint8_t filetype;

  if(!(GetUDFCache(PartitionCache, 0, &partition)))
    return 0;

  /* Scan dir for ICB of file */
  lbnum = dirp->dir_current;

  // I have cut out the caching part of the original UDFScanDir() function
  // one day we should probably bring it back.
  memset(&File, 0, sizeof(File));

  if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
    return 0;
  }


  p = dirp->current_p;
  while( p < dirp->dir_length ) {
    if( p > DVD_VIDEO_LB_LEN ) {
      ++lbnum;
      p -= DVD_VIDEO_LB_LEN;

      //Dir.Length -= DVD_VIDEO_LB_LEN;
      if (dirp->dir_length >= DVD_VIDEO_LB_LEN)
        dirp->dir_length -= DVD_VIDEO_LB_LEN;
      else
        dirp->dir_length = 0;

      if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
      }
    }
    UDFDescriptor( &directory[ p ], &TagID );

    if( TagID == 257 ) {

      p += UDFFileIdentifier( &directory[ p ], &filechar,
                              filename, &FileICB );

      dirp->current_p = p;
      dirp->dir_current = lbnum;

      if (!*filename)  // No filename, simulate "." dirname
        strcpy((char *)dirp->entry.d_name, ".");
      else {
        // Bah, MSVC don't have strlcpy()
        strncpy((char *)dirp->entry.d_name, filename,
                sizeof(dirp->entry.d_name)-1);
        dirp->entry.d_name[ sizeof(dirp->entry.d_name) - 1 ] = 0;
      }


      // Look up the Filedata
      if( !UDFMapICB( FileICB, &filetype, &partition, &File))
        return 0;
      if (filetype == 4)
        dirp->entry.d_type = DVD_DT_DIR;
      else
        dirp->entry.d_type = DVD_DT_REG; // Add more types?

      dirp->entry.d_filesize = File.Length;

      return 1;

    } else {
      // Not TagID 257
      return 0;
    }
  }
  // End of DIR
  return 0;
}

int udf25::SetUDFCache(UDFCacheType type, uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;
  void *tmp;

  if(DVDUDFCacheLevel(-1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle();

  if(c == NULL) {
    c = (struct udf_cache *)calloc(1, sizeof(struct udf_cache));
    /* fprintf(stderr, "calloc: %d\n", sizeof(struct udf_cache)); */
    if(c == NULL)
      return 0;
    SetUDFCacheHandle(c);
  }


  switch(type) {
  case AVDPCache:
    c->avdp = *(struct avdp_t *)data;
    c->avdp_valid = 1;
    break;
  case PVDCache:
    c->pvd = *(struct pvd_t *)data;
    c->pvd_valid = 1;
    break;
  case PartitionCache:
    c->partition = *(struct Partition *)data;
    c->partition_valid = 1;
    break;
  case RootICBCache:
    c->rooticb = *(struct AD *)data;
    c->rooticb_valid = 1;
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        /* replace with new data */
        c->lbs[n].data_base = ((uint8_t **)data)[0];
        c->lbs[n].data = ((uint8_t **)data)[1];
        c->lbs[n].lb = nr;
        return 1;
      }
    }
    c->lb_num++;
    tmp = realloc(c->lbs, c->lb_num * sizeof(struct lbudf));
    /*
    fprintf(stderr, "realloc lb: %d * %d = %d\n",
    c->lb_num, sizeof(struct lbudf),
    c->lb_num * sizeof(struct lbudf));
    */
    if(tmp == NULL) {
      if(c->lbs) free(c->lbs);
      c->lb_num = 0;
      return 0;
    }
    c->lbs = (struct lbudf *)tmp;
    c->lbs[n].data_base = ((uint8_t **)data)[0];
    c->lbs[n].data = ((uint8_t **)data)[1];
    c->lbs[n].lb = nr;
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        /* replace with new data */
        c->maps[n] = *(struct icbmap *)data;
        c->maps[n].lbn = nr;
        return 1;
      }
    }
    c->map_num++;
    tmp = realloc(c->maps, c->map_num * sizeof(struct icbmap));
    /*
    fprintf(stderr, "realloc maps: %d * %d = %d\n",
      c->map_num, sizeof(struct icbmap),
      c->map_num * sizeof(struct icbmap));
    */
    if(tmp == NULL) {
      if(c->maps) free(c->maps);
      c->map_num = 0;
      return 0;
    }
    c->maps = (struct icbmap *)tmp;
    c->maps[n] = *(struct icbmap *)data;
    c->maps[n].lbn = nr;
    break;
  default:
    return 0;
  }

  return 1;
}

int udf25::DVDUDFCacheLevel(int level)
{
  if(level > 0) {
    level = 1;
  } else if(level < 0) {
    return m_udfcache_level;
  }

  m_udfcache_level = level;

  return level;
}

void *udf25::GetUDFCacheHandle()
{
  return m_udfcache;
}

void udf25::SetUDFCacheHandle(void *cache)
{
  m_udfcache = cache;
}

int udf25::GetUDFCache(UDFCacheType type, uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;

  if(DVDUDFCacheLevel(-1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle();

  if(c == NULL)
    return 0;

  switch(type) {
  case AVDPCache:
    if(c->avdp_valid) {
      *(struct avdp_t *)data = c->avdp;
      return 1;
    }
    break;
  case PVDCache:
    if(c->pvd_valid) {
      *(struct pvd_t *)data = c->pvd;
      return 1;
    }
    break;
  case PartitionCache:
    if(c->partition_valid) {
      *(struct Partition *)data = c->partition;
      return 1;
    }
    break;
  case RootICBCache:
    if(c->rooticb_valid) {
      *(struct AD *)data = c->rooticb;
      return 1;
    }
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        *(uint8_t **)data = c->lbs[n].data;
        return 1;
      }
    }
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        *(struct icbmap *)data = c->maps[n];
        return 1;
      }
    }
    break;
  default:
    break;
  }

  return 0;
}

int udf25::UDFReadBlocksRaw( uint32_t lb_number, size_t block_count, unsigned char *data, int encrypted )
{
  int ret;

  ret = file_seek( m_fp, (int) lb_number );
  if( ret != (int) lb_number ) {
    CLog::Log(LOGERROR, "udf25::UDFReadBlocksRaw -  Can't seek to block %u (got %u)", lb_number, ret );
    return 0;
  }

  ret = file_read( m_fp, (char *) data, (int) block_count, encrypted );
  return ret;
}

int udf25::DVDReadLBUDF( uint32_t lb_number, size_t block_count, unsigned char *data, int encrypted )
{
  int ret;
  size_t count = block_count;

  while(count > 0) {

    ret = UDFReadBlocksRaw(lb_number, count, data, encrypted);

    if(ret <= 0) {
      /* One of the reads failed or nothing more to read, too bad.
       * We won't even bother returning the reads that went ok. */
      return ret;
    }

    count -= (size_t)ret;
    lb_number += (uint32_t)ret;
  }

  return block_count;
}

int udf25::UDFGetAVDP( struct avdp_t *avdp)
{
  uint8_t Anchor_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *Anchor = (uint8_t *)(((uintptr_t)Anchor_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum, MVDS_location, MVDS_length;
  uint16_t TagID;
  uint32_t lastsector;
  int terminate;
  struct avdp_t;

  if(GetUDFCache(AVDPCache, 0, avdp))
    return 1;

  /* Find Anchor */
  lastsector = 0;
  lbnum = 256;   /* Try #1, prime anchor */
  terminate = 0;

  for(;;) {
    if( DVDReadLBUDF( lbnum, 1, Anchor, 0 ) > 0 ) {
      UDFDescriptor( Anchor, &TagID );
    } else {
      TagID = 0;
    }
    if (TagID != 2) {
      /* Not an anchor */
      if( terminate ) return 0; /* Final try failed */

      if( lastsector ) {
        /* We already found the last sector.  Try #3, alternative
         * backup anchor.  If that fails, don't try again.
         */
        lbnum = lastsector;
        terminate = 1;
      } else {
        /* TODO: Find last sector of the disc (this is optional). */
        if( lastsector )
          /* Try #2, backup anchor */
          lbnum = lastsector - 256;
        else
          /* Unable to find last sector */
          return 0;
      }
    } else
      /* It's an anchor! We can leave */
      break;
  }
  /* Main volume descriptor */
  UDFExtentAD( &Anchor[ 16 ], &MVDS_length, &MVDS_location );
  avdp->mvds.location = MVDS_location;
  avdp->mvds.length = MVDS_length;

  /* Backup volume descriptor */
  UDFExtentAD( &Anchor[ 24 ], &MVDS_length, &MVDS_location );
  avdp->rvds.location = MVDS_location;
  avdp->rvds.length = MVDS_length;

  SetUDFCache(AVDPCache, 0, avdp);

  return 1;
}

int udf25::UDFFindPartition( int partnum, struct Partition *part )
{
  uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum, MVDS_location, MVDS_length;
  uint16_t TagID;
  int i, volvalid;
  struct avdp_t avdp;

  if(!UDFGetAVDP(&avdp))
    return 0;

  /* Main volume descriptor */
  MVDS_location = avdp.mvds.location;
  MVDS_length = avdp.mvds.length;

  part->valid = 0;
  volvalid = 0;
  part->VolumeDesc[ 0 ] = '\0';
  i = 1;
  do {
    /* Find Volume Descriptor */
    lbnum = MVDS_location;
    do {

      if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
        TagID = 0;
      else
        UDFDescriptor( LogBlock, &TagID );

      if( ( TagID == 5 ) && ( !part->valid ) ) {
        /* Partition Descriptor */
        UDFPartition( LogBlock, &part->Flags, &part->Number,
                      part->Contents, &part->Start, &part->Length );
        part->valid = ( partnum == part->Number );
      } else if( ( TagID == 6 ) && ( !volvalid ) ) {
        /* Logical Volume Descriptor */
        if( UDFLogVolume( LogBlock, part->VolumeDesc ) ) {
          /* TODO: sector size wrong! */
        } else
          volvalid = 1;
      }

    } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
               / DVD_VIDEO_LB_LEN ) && ( TagID != 8 )
             && ( ( !part->valid ) || ( !volvalid ) ) );

    if( ( !part->valid) || ( !volvalid ) ) {
      /* Backup volume descriptor */
      MVDS_location = avdp.mvds.location;
      MVDS_length = avdp.mvds.length;
    }
  } while( i-- && ( ( !part->valid ) || ( !volvalid ) ) );

  /* We only care for the partition, not the volume */
  return part->valid;
}

int udf25::UDFMapICB( struct AD ICB, uint8_t *FileType, struct Partition *partition, struct FileAD *File )
{
  uint8_t LogBlock_base[DVD_VIDEO_LB_LEN + 2048];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  struct icbmap tmpmap;

  lbnum = partition->Start + ICB.Location;
  tmpmap.lbn = lbnum;
  if(GetUDFCache(MapCache, lbnum, &tmpmap)) {
    *FileType = tmpmap.filetype;
    memcpy(File, &tmpmap.file, sizeof(tmpmap.file));
    return 1;
  }

  do {
    if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
      TagID = 0;
    else
      UDFDescriptor( LogBlock, &TagID );

    if( TagID == 261 ) {
      UDFFileEntry( LogBlock, FileType, partition, File );
      memcpy(&tmpmap.file, File, sizeof(tmpmap.file));
      tmpmap.filetype = *FileType;
      SetUDFCache(MapCache, tmpmap.lbn, &tmpmap);
      return 1;
    };

    /* ExtendedFileInfo */
    if( TagID == 266 ) {
      UDFExtFileEntry( LogBlock, FileType, partition, File );
      memcpy(&tmpmap.file, File, sizeof(tmpmap.file));
      tmpmap.filetype = *FileType;
      SetUDFCache(MapCache, tmpmap.lbn, &tmpmap);
      return 1;
  }


  } while( ( lbnum <= partition->Start + ICB.Location + ( ICB.Length - 1 )
             / DVD_VIDEO_LB_LEN ) && ( TagID != 261 ) && (TagID != 266) );

  return 0;
}
void udf25::UDFFreeFile(UDF_FILE file)
{
  free(file);
}

int udf25::UDFScanDir( struct FileAD Dir, char *FileName, struct Partition *partition, struct AD *FileICB, int cache_file_info)
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint8_t directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
  uint8_t *directory = (uint8_t *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  uint8_t filechar;
  unsigned int p;
  uint8_t *cached_dir_base = NULL, *cached_dir;
  uint32_t dir_lba;
  struct AD tmpICB;
  int found = 0;
  int in_cache = 0;

  /* Scan dir for ICB of file */
  lbnum = partition->Start + Dir.AD_chain[0].Location;

  if(DVDUDFCacheLevel(-1) > 0) {
    /* caching */

    if(!GetUDFCache(LBUDFCache, lbnum, &cached_dir)) {
      dir_lba = (Dir.AD_chain[0].Length + DVD_VIDEO_LB_LEN) / DVD_VIDEO_LB_LEN;
      if((cached_dir_base = (uint8_t *)malloc(dir_lba * DVD_VIDEO_LB_LEN + 2048)) == NULL)
        return 0;
      cached_dir = (uint8_t *)(((uintptr_t)cached_dir_base & ~((uintptr_t)2047)) + 2048);
      if( DVDReadLBUDF( lbnum, dir_lba, cached_dir, 0) <= 0 ) {
        free(cached_dir_base);
        cached_dir_base = NULL;
        cached_dir = NULL;
      }
      /*
      if(cached_dir) {
        fprintf(stderr, "malloc dir: %d\n",  dir_lba * DVD_VIDEO_LB_LEN);
      }
      */
      {
        uint8_t *data[2];
        data[0] = cached_dir_base;
        data[1] = cached_dir;
        SetUDFCache(LBUDFCache, lbnum, data);
      }
    } else
      in_cache = 1;

    if(cached_dir == NULL)
      return 0;

    p = 0;

    while( p < Dir.AD_chain[0].Length ) {  /* Assuming dirs don't use chains? */
      UDFDescriptor( &cached_dir[ p ], &TagID );
      if( TagID == 257 ) {
        p += UDFFileIdentifier( &cached_dir[ p ], &filechar,
                                filename, &tmpICB );
        if(cache_file_info && !in_cache) {
          uint8_t tmpFiletype;
          struct FileAD tmpFile;

          memset(&tmpFile, 0, sizeof(tmpFile));

          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            found = 1;
          }
          UDFMapICB(tmpICB, &tmpFiletype, partition, &tmpFile);
        } else {
          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            return 1;
          }
        }
      } else {
        if(cache_file_info && (!in_cache) && found)
          return 1;
        return 0;
      }
    }
    if(cache_file_info && (!in_cache) && found)
      return 1;
    return 0;
  }

  if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 )
    return 0;

  p = 0;
  while( p < Dir.AD_chain[0].Length ) {
    if( p > DVD_VIDEO_LB_LEN ) {
      ++lbnum;
      p -= DVD_VIDEO_LB_LEN;
      Dir.AD_chain[0].Length -= DVD_VIDEO_LB_LEN;
      if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
      }
    }
    UDFDescriptor( &directory[ p ], &TagID );
    if( TagID == 257 ) {
      p += UDFFileIdentifier( &directory[ p ], &filechar,
                              filename, FileICB );
      if( !strcasecmp( FileName, filename ) ) {
        return 1;
      }
    } else
      return 0;
  }

  return 0;
}

udf25::udf25( )
{
  m_fp = NULL;
  m_udfcache_level = 1;
  m_udfcache = NULL;
}

udf25::~udf25( )
{
  if(m_fp)
    m_fp->Close();
}

UDF_FILE udf25::UDFFindFile( const char* filename, uint64_t *filesize )
{
  uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  struct Partition partition;
  struct AD RootICB, ICB;
  struct FileAD File;
  char tokenline[ MAX_UDF_FILE_NAME_LEN ];
  char *token;
  uint8_t filetype;
  struct FileAD *result;

  *filesize = 0;
  tokenline[0] = '\0';
  strncat(tokenline, filename, MAX_UDF_FILE_NAME_LEN - 1);
  memset(&ICB, 0, sizeof(ICB));
  memset(&File, 0, sizeof(File));

  if(!(GetUDFCache(PartitionCache, 0, &partition) &&
       GetUDFCache(RootICBCache, 0, &RootICB))) {
    /* Find partition, 0 is the standard location for DVD Video.*/
    if( !UDFFindPartition(0, &partition ) ) return 0;
    SetUDFCache(PartitionCache, 0, &partition);

    /* Find root dir ICB */
    lbnum = partition.Start;
    do {
      if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
        TagID = 0;
      else
        UDFDescriptor( LogBlock, &TagID );

       /*
        * It seems that someone added a FileType of 250, which seems to be
        * a "redirect" style file entry. If we discover such an entry, we
        * add on the "location" to partition->Start, and try again.
        * Who added this? Is there any official guide somewhere?
        * 2008/09/17 lundman
        * Should we handle 261(250) as well? FileEntry+redirect
        */
       if( TagID == 266 ) {
           UDFExtFileEntry( LogBlock, &filetype, &partition, &File );
           if (filetype == 250) {
              partition.Start += File.AD_chain[0].Location;
              lbnum = partition.Start;
              SetUDFCache(PartitionCache, 0, &partition);
              continue;
          }
      }

      /* File Set Descriptor */
      if( TagID == 256 )  /* File Set Descriptor */
        UDFLongAD( &LogBlock[ 400 ], &RootICB );
    } while( ( lbnum < partition.Start + partition.Length )
             && ( TagID != 8 ) && ( TagID != 256 ) );

    /* Sanity checks. */
    if( TagID != 256 )
      return NULL;
    /* This following test will fail under UDF2.50 images, as it is no longer
     * valid */
    /*if( RootICB.Partition != 0 )
      return 0;*/
    SetUDFCache(RootICBCache, 0, &RootICB);
  }

  /* Find root dir */
  if( !UDFMapICB( RootICB, &filetype, &partition, &File ) )
    return NULL;
  if( filetype != 4 )
    return NULL;  /* Root dir should be dir */
  {
    int cache_file_info = 0;
    /* Tokenize filepath */
    token = strtok(tokenline, "/");

    while( token != NULL ) {
      if( !UDFScanDir( File, token, &partition, &ICB,
                       cache_file_info))
        return NULL;
      if( !UDFMapICB( ICB, &filetype, &partition, &File ) )
        return NULL;
      if(!strcmp(token, "index.bdmv"))
        cache_file_info = 1;
      token = strtok( NULL, "/" );
    }
  }

  /* Sanity check. */
  if( File.AD_chain[0].Partition != 0 )
    return 0;
  *filesize = File.Length;
  /* Hack to not return partition.Start for empty files. */
  if( !File.AD_chain[0].Location )
    return NULL;

  /* Allocate a new UDF_FILE and return it. */
  result = (struct FileAD *) malloc(sizeof(*result));
  if (!result) return NULL;

  memcpy(result, &File, sizeof(*result));

  result->Partition_Start = partition.Start;

  return result;
}

HANDLE udf25::OpenFile( const char* fullFilename )
{
  uint64_t filesize;
  UDF_FILE file = NULL;
  BD_FILE bdfile = NULL;
  char isoname[256], filename[256];

  if(UDFSplitIsoFile(fullFilename, isoname, filename) !=0 )
    return INVALID_HANDLE_VALUE;

  m_fp = file_open(isoname);
  if(m_fp)
  {
    file = UDFFindFile(filename, &filesize);
    if(file)
    {
      bdfile = (BD_FILE)malloc(sizeof(*bdfile));
      memset(bdfile, 0, sizeof(*bdfile));

      bdfile->file = file;
      bdfile->filesize = filesize;
    }
    else
    {
      CloseFile(NULL);
    }
  }

  return bdfile ? (HANDLE)bdfile : INVALID_HANDLE_VALUE;
}


long udf25::ReadFile(HANDLE hFile, unsigned char *pBuffer, long lSize)
{
  BD_FILE bdfile = (BD_FILE)hFile;
  unsigned char *secbuf_base, *secbuf;
  unsigned int numsec, seek_sector, seek_byte, fileoff;
  int ret;

  /* Check arguments. */
  if( bdfile == NULL || pBuffer == NULL )
    return -1;

  seek_sector =(unsigned int) (bdfile->seek_pos / DVD_VIDEO_LB_LEN);
  seek_byte   = bdfile->seek_pos % DVD_VIDEO_LB_LEN;

  numsec = ( ( seek_byte + lSize ) / DVD_VIDEO_LB_LEN ) +
    ( ( ( seek_byte + lSize ) % DVD_VIDEO_LB_LEN ) ? 1 : 0 );

  secbuf_base = (unsigned char *) malloc( numsec * DVD_VIDEO_LB_LEN + 2048 );
  secbuf = (unsigned char *)(((uintptr_t)secbuf_base & ~((uintptr_t)2047)) + 2048);
  if( !secbuf_base ) {
    CLog::Log(LOGERROR, "udf25::ReadFile - Can't allocate memory for file read!" );
    return 0;
  }

  fileoff = UDFFileBlockPos(bdfile->file, seek_sector);
  fileoff -= 32;

  ret = UDFReadBlocksRaw( fileoff, (size_t) numsec, secbuf, 0 );

  if( ret != (int) numsec ) {
    free( secbuf_base );
    return ret < 0 ? ret : 0;
  }

  memcpy( pBuffer, &(secbuf[ seek_byte ]), lSize );
  free( secbuf_base );

  DVDFileSeekForce(bdfile, bdfile->seek_pos + lSize, -1);

  return lSize;
}

void udf25::CloseFile(HANDLE hFile)
{
  BD_FILE bdfile = (BD_FILE)hFile;
  if(bdfile)
  {
    free(bdfile->file);
    free(bdfile);
  }

  free(m_udfcache);
  m_udfcache = NULL;

  if(m_fp)
  {
    file_close(m_fp);
    delete(m_fp);
    m_fp = NULL;
  }
}

int64_t udf25::Seek(HANDLE hFile, int64_t lOffset, int whence)
{
  BD_FILE bdfile = (BD_FILE)hFile;

  if(bdfile == NULL)
    return -1;

  int64_t seek_pos = bdfile->seek_pos;
  switch (whence)
  {
  case SEEK_SET:
    // cur = pos
    bdfile->seek_pos = lOffset;
    break;

  case SEEK_CUR:
    // cur += pos
    bdfile->seek_pos += lOffset;
    break;
  case SEEK_END:
    // end += pos
    bdfile->seek_pos = bdfile->filesize + lOffset;
    break;
  }

  if (bdfile->seek_pos > bdfile->filesize)
  {
    bdfile->seek_pos = seek_pos;
    return bdfile->seek_pos;
  }

  return bdfile->seek_pos;
}

int64_t udf25::GetFileSize(HANDLE hFile)
{
  BD_FILE bdfile = (BD_FILE)hFile;

  if(bdfile == NULL)
    return -1;

  return bdfile->filesize;
}

int64_t udf25::GetFilePosition(HANDLE hFile)
{
  BD_FILE bdfile = (BD_FILE)hFile;

  if(bdfile == NULL)
    return -1;

  return bdfile->seek_pos;
}

udf_dir_t *udf25::OpenDir( const char *subdir )
{
  udf_dir_t *result;
  BD_FILE bd_file;

  bd_file = (BD_FILE)OpenFile(subdir);

  if (bd_file == (BD_FILE)INVALID_HANDLE_VALUE)
  {
    return NULL;
  }
  result = (udf_dir_t *)malloc(sizeof(*result));
  if (!result) {
    UDFFreeFile(bd_file->file);
    free(bd_file);
    return NULL;
  }

  memset(result, 0, sizeof(*result));

  result->dir_location = UDFFileBlockPos(bd_file->file, 0);
  result->dir_current  = UDFFileBlockPos(bd_file->file, 0);
  result->dir_length   = (uint32_t) bd_file->filesize;
  UDFFreeFile(bd_file->file);
  free(bd_file);

  return result;
}

udf_dirent_t *udf25::ReadDir( udf_dir_t *dirp )
{
  if (!UDFScanDirX(dirp)) {
    dirp->current_p = 0;
    dirp->dir_current = dirp->dir_location; // this is a rewind, wanted?
    return NULL;
  }

  return &dirp->entry;
}

int udf25::CloseDir( udf_dir_t *dirp )
{
  if (!dirp) return 0;

  free(dirp);

  CloseFile(NULL);

  return 0;
}
