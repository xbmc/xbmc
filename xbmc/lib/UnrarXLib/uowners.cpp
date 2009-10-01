

void ExtractUnixOwner(Archive &Arc,char *FileName)
{
  if (Arc.HeaderCRC!=Arc.UOHead.HeadCRC)
  {
    Log(Arc.FileName,St(MOwnersBroken),FileName);
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }

  struct passwd *pw;
  if ((pw=getpwnam(Arc.UOHead.OwnerName))==NULL)
  {
    Log(Arc.FileName,St(MErrGetOwnerID),Arc.UOHead.OwnerName);
    ErrHandler.SetErrorCode(WARNING);
    return;
  }
  uid_t OwnerID=pw->pw_uid;

  struct group *gr;
  if ((gr=getgrnam(Arc.UOHead.GroupName))==NULL)
  {
    Log(Arc.FileName,St(MErrGetGroupID),Arc.UOHead.GroupName);
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }
  uint Attr=GetFileAttr(FileName,NULL);
  gid_t GroupID=gr->gr_gid;
#if defined(SAVE_LINKS) && !defined(_APPLE)
  if (lchown(FileName,OwnerID,GroupID)!=0)
#else
  if (chown(FileName,OwnerID,GroupID)!=0)
#endif
  {
    Log(Arc.FileName,St(MSetOwnersError),FileName);
    ErrHandler.SetErrorCode(CRC_ERROR);
  }
  SetFileAttr(FileName,NULL,Attr);
}


void ExtractUnixOwnerNew(Archive &Arc,char *FileName)
{
  char *OwnerName=(char *)&Arc.SubHead.SubData[0];
  int OwnerSize=strlen(OwnerName)+1;
  int GroupSize=Arc.SubHead.SubData.Size()-OwnerSize;
  char GroupName[NM];
  strncpy(GroupName,(char *)&Arc.SubHead.SubData[OwnerSize],GroupSize);
  GroupName[GroupSize]=0;

  struct passwd *pw;
  if ((pw=getpwnam(OwnerName))==NULL)
  {
    Log(Arc.FileName,St(MErrGetOwnerID),OwnerName);
    ErrHandler.SetErrorCode(WARNING);
    return;
  }
  uid_t OwnerID=pw->pw_uid;

  struct group *gr;
  if ((gr=getgrnam(GroupName))==NULL)
  {
    Log(Arc.FileName,St(MErrGetGroupID),GroupName);
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }
  uint Attr=GetFileAttr(FileName,NULL);
  gid_t GroupID=gr->gr_gid;
#if defined(SAVE_LINKS) && !defined(_APPLE)
  if (lchown(FileName,OwnerID,GroupID)!=0)
#else
  if (chown(FileName,OwnerID,GroupID)!=0)
#endif
  {
    Log(Arc.FileName,St(MSetOwnersError),FileName);
    ErrHandler.SetErrorCode(CRC_ERROR);
  }
  SetFileAttr(FileName,NULL,Attr);
}
