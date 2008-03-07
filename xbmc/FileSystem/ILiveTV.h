namespace XFILE
{
class ILiveTVInterface
{
public:
  virtual ~ILiveTVInterface() {}
  virtual bool           NextChannel() = 0;
  virtual bool           PrevChannel() = 0;

  virtual int            GetTotalTime() = 0;
  virtual int            GetStartTime() = 0;

  virtual bool           UpdateItem(CFileItem& item)=0;
};

class IRecordable
{
public:
  virtual ~IRecordable() {}

  virtual bool CanRecord() = 0;
  virtual bool IsRecording() = 0;
  virtual bool Record(bool bOnOff) = 0;
};

}
