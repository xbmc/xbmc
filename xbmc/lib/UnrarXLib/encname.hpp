#ifndef _RAR_ENCNAME_
#define _RAR_ENCNAME_

class EncodeFileName
{
  private:
    void AddFlags(int Value);

    byte *EncName;
    byte Flags;
    int FlagBits;
    int FlagsPos;
    int DestSize;
  public:
    EncodeFileName();
    int Encode(char *Name,wchar *NameW,byte *EncName);
    void Decode(char *Name,byte *EncName,int EncSize,wchar *NameW,int MaxDecSize);
};

#endif
