#ifndef _RAR_RAWREAD_
#define _RAR_RAWREAD_

class RawRead
{
  private:
    Array<byte> Data;
    File *SrcFile;
    int DataSize;
    int ReadPos;
#ifndef SHELL_EXT
    CryptData *Crypt;
#endif
  public:
    RawRead(File *SrcFile);
    void Read(int Size);
    void Read(byte *SrcData,int Size);
    void Get(byte &Field);
    void Get(ushort &Field);
    void Get(uint &Field);
    void Get8(Int64 &Field);
    void Get(byte *Field,int Size);
    void Get(wchar *Field,int Size);
    uint GetCRC(bool ProcessedOnly);
    int Size() {return DataSize;}
    int PaddedSize() {return Data.Size()-DataSize;}
#ifndef SHELL_EXT
    void SetCrypt(CryptData *Crypt) {RawRead::Crypt=Crypt;}
#endif
};

#endif
