namespace JSONRPC
{
  class IClient
  {
  public:
    virtual ~IClient() { };
    virtual int GetPermissionFlags() = 0;
    virtual int GetAnnouncementFlags() = 0;
    virtual bool SetAnnouncementFlags(int flags) = 0;
  };
}
