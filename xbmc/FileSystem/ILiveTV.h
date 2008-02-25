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

  virtual CVideoInfoTag* GetVideoInfoTag() = 0;
};
}
