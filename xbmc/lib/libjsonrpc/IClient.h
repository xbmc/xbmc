namespace JSONRPC
{
  class IClient
  {
  public:
    virtual ~IClient() { };
    virtual int GetPermissionFlags() = 0;
  };
}
