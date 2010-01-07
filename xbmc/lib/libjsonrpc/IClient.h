namespace JSONRPC
{
  class IClient
  {
  public:
    virtual ~IClient() { };
    virtual int GetPermissionFlags() = 0;
    virtual int GetBroadcastFlags() = 0;
    virtual bool SetBroadcastFlags(int flags) = 0;
  };
}
