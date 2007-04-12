#ifndef _RAR_HEADERS_
#define _RAR_HEADERS_

#define  SIZEOF_MARKHEAD         7
#define  SIZEOF_OLDMHD           7
#define  SIZEOF_NEWMHD          13
#define  SIZEOF_OLDLHD          21
#define  SIZEOF_NEWLHD          32
#define  SIZEOF_SHORTBLOCKHEAD   7
#define  SIZEOF_LONGBLOCKHEAD   11
#define  SIZEOF_SUBBLOCKHEAD    14
#define  SIZEOF_COMMHEAD        13
#define  SIZEOF_PROTECTHEAD     26
#define  SIZEOF_AVHEAD          14
#define  SIZEOF_SIGNHEAD        15
#define  SIZEOF_UOHEAD          18
#define  SIZEOF_MACHEAD         22
#define  SIZEOF_EAHEAD          24
#define  SIZEOF_BEEAHEAD        24
#define  SIZEOF_STREAMHEAD      26

#define  PACK_VER               29
#define  PACK_CRYPT_VER         29
#define  UNP_VER                29
#define  CRYPT_VER              29
#define  AV_VER                 20
#define  PROTECT_VER            20

#define  MHD_VOLUME         0x0001
#define  MHD_COMMENT        0x0002
#define  MHD_LOCK           0x0004
#define  MHD_SOLID          0x0008
#define  MHD_PACK_COMMENT   0x0010
#define  MHD_NEWNUMBERING   0x0010
#define  MHD_AV             0x0020
#define  MHD_PROTECT        0x0040
#define  MHD_PASSWORD       0x0080
#define  MHD_FIRSTVOLUME    0x0100

#define  LHD_SPLIT_BEFORE   0x0001
#define  LHD_SPLIT_AFTER    0x0002
#define  LHD_PASSWORD       0x0004
#define  LHD_COMMENT        0x0008
#define  LHD_SOLID          0x0010

#define  LHD_WINDOWMASK     0x00e0
#define  LHD_WINDOW64       0x0000
#define  LHD_WINDOW128      0x0020
#define  LHD_WINDOW256      0x0040
#define  LHD_WINDOW512      0x0060
#define  LHD_WINDOW1024     0x0080
#define  LHD_WINDOW2048     0x00a0
#define  LHD_WINDOW4096     0x00c0
#define  LHD_DIRECTORY      0x00e0

#define  LHD_LARGE          0x0100
#define  LHD_UNICODE        0x0200
#define  LHD_SALT           0x0400
#define  LHD_VERSION        0x0800
#define  LHD_EXTTIME        0x1000
#define  LHD_EXTFLAGS       0x2000

#define  SKIP_IF_UNKNOWN    0x4000
#define  LONG_BLOCK         0x8000

#define  EARC_NEXT_VOLUME   0x0001
#define  EARC_DATACRC       0x0002
#define  EARC_REVSPACE      0x0004
#define  EARC_VOLNUMBER     0x0008

enum HEADER_TYPE {
  MARK_HEAD=0x72,MAIN_HEAD=0x73,FILE_HEAD=0x74,COMM_HEAD=0x75,AV_HEAD=0x76,
  SUB_HEAD=0x77,PROTECT_HEAD=0x78,SIGN_HEAD=0x79,NEWSUB_HEAD=0x7a,
  ENDARC_HEAD=0x7b
};

enum { EA_HEAD=0x100,UO_HEAD=0x101,MAC_HEAD=0x102,BEEA_HEAD=0x103,
       NTACL_HEAD=0x104,STREAM_HEAD=0x105 };

enum HOST_SYSTEM {
  HOST_MSDOS=0,HOST_OS2=1,HOST_WIN32=2,HOST_UNIX=3,HOST_MACOS=4,
  HOST_BEOS=5,HOST_MAX
};

#define SUBHEAD_TYPE_CMT      "CMT"
#define SUBHEAD_TYPE_ACL      "ACL"
#define SUBHEAD_TYPE_STREAM   "STM"
#define SUBHEAD_TYPE_UOWNER   "UOW"
#define SUBHEAD_TYPE_AV       "AV"
#define SUBHEAD_TYPE_RR       "RR"
#define SUBHEAD_TYPE_OS2EA    "EA2"
#define SUBHEAD_TYPE_BEOSEA   "EABE"

/* new file inherits a subblock when updating a host file */
#define SUBHEAD_FLAGS_INHERITED    0x80000000

#define SUBHEAD_FLAGS_CMT_UNICODE  0x00000001

struct OldMainHeader
{
  byte Mark[4];
  ushort HeadSize;
  byte Flags;
};


struct OldFileHeader
{
  uint PackSize;
  uint UnpSize;
  ushort FileCRC;
  ushort HeadSize;
  uint FileTime;
  byte FileAttr;
  byte Flags;
  byte UnpVer;
  byte NameSize;
  byte Method;
};


struct MarkHeader
{
  byte Mark[7];
};


struct BaseBlock
{
  ushort HeadCRC;
  HEADER_TYPE HeadType;//byte
  ushort Flags;
  ushort HeadSize;

  bool IsSubBlock()
  {
    if (HeadType==SUB_HEAD)
      return(true);
    if (HeadType==NEWSUB_HEAD && (Flags & LHD_SOLID)!=0)
      return(true);
    return(false);
  }
};

struct BlockHeader:BaseBlock
{
  union {
    uint DataSize;
    uint PackSize;
  };
};


struct MainHeader:BlockHeader
{
  ushort HighPosAV;
  uint PosAV;
};


#define SALT_SIZE     8

struct FileHeader:BlockHeader
{
  uint UnpSize;
  byte HostOS;
  uint FileCRC;
  uint FileTime;
  byte UnpVer;
  byte Method;
  ushort NameSize;
  union {
    uint FileAttr;
    uint SubFlags;
  };
/* optional */
  uint HighPackSize;
  uint HighUnpSize;
/* names */
  char FileName[NM];
  wchar FileNameW[NM];
/* optional */
  Array<byte> SubData;
  byte Salt[SALT_SIZE];

  RarTime mtime;
  RarTime ctime;
  RarTime atime;
  RarTime arctime;
/* dummy */
  Int64 FullPackSize;
  Int64 FullUnpSize;

  void Clear(int SubDataSize)
  {
    SubData.Alloc(SubDataSize);
    Flags=LONG_BLOCK;
    SubFlags=0;
  }

  bool CmpName(const char *Name)
  {
    return(strcmp(FileName,Name)==0);
  }

  FileHeader& operator = (FileHeader &hd)
  {
    SubData.Reset();
    memcpy(this,&hd,sizeof(*this));
    SubData.CleanData();
    SubData=hd.SubData;
    return(*this);
  }
};


struct EndArcHeader:BaseBlock
{
  uint ArcDataCRC;
  ushort VolNumber;
};


struct SubBlockHeader:BlockHeader
{
  ushort SubType;
  byte Level;
};


struct CommentHeader:BaseBlock
{
  ushort UnpSize;
  byte UnpVer;
  byte Method;
  ushort CommCRC;
};


struct ProtectHeader:BlockHeader
{
  byte Version;
  ushort RecSectors;
  uint TotalBlocks;
  byte Mark[8];
};


struct AVHeader:BaseBlock
{
  byte UnpVer;
  byte Method;
  byte AVVer;
  uint AVInfoCRC;
};


struct SignHeader:BaseBlock
{
  uint CreationTime;
  ushort ArcNameSize;
  ushort UserNameSize;
};


struct UnixOwnersHeader:SubBlockHeader
{
  ushort OwnerNameSize;
  ushort GroupNameSize;
/* dummy */
  char OwnerName[NM];
  char GroupName[NM];
};


struct EAHeader:SubBlockHeader
{
  uint UnpSize;
  byte UnpVer;
  byte Method;
  uint EACRC;
};


struct StreamHeader:SubBlockHeader
{
  uint UnpSize;
  byte UnpVer;
  byte Method;
  uint StreamCRC;
  ushort StreamNameSize;
/* dummy */
  byte StreamName[NM];
};


struct MacFInfoHeader:SubBlockHeader
{
  uint fileType;
  uint fileCreator;
};


#endif
