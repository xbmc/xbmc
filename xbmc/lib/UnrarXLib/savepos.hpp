#ifndef _RAR_SAVEPOS_
#define _RAR_SAVEPOS_

class SaveFilePos
{
  private:
    File *SaveFile;
    Int64 SavePos;
    uint CloseCount;
  public:
    SaveFilePos(File &SaveFile);
    ~SaveFilePos();
};

#endif
