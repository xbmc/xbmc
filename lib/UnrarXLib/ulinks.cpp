#include "rar.hpp"
#include "Util.h"
#include "utils/URIUtils.h"


int ExtractLink(ComprDataIO &DataIO,Archive &Arc,char *DestName,uint &LinkCRC,bool Create)
{
#if defined(SAVE_LINKS) && defined(_UNIX)
  char FileName[NM];
  if (IsLink(Arc.NewLhd.FileAttr))
  {
    uint DataSize=Min(Arc.NewLhd.PackSize,sizeof(FileName)-1);
    DataIO.UnpRead((byte *)FileName,DataSize);
    FileName[DataSize]=0;
    if (Create)
    {
      std::string strPath = URIUtils::GetDirectory(DestName);
      CUtil::CreateDirectoryEx(strPath);
      if (symlink(FileName,DestName)==-1)
      {
        if (errno==EEXIST)
          Log(Arc.FileName,St(MSymLinkExists),DestName);
        else
        {
          Log(Arc.FileName,St(MErrCreateLnk),DestName);
          ErrHandler.SetErrorCode(WARNING);
        }
      }
    }
    int NameSize=Min(DataSize,strlen(FileName));
    LinkCRC=CRC(0xffffffff,FileName,NameSize);    return(1);
  }
#endif
  return(0);
}
