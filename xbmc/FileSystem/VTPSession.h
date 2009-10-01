
#include <string>
#include <vector>

#include "system.h" // for SOCKET

//#define VTP_STANDALONE

class CVTPSession
{
public:
  CVTPSession();
  ~CVTPSession();
  bool Open(const std::string &host, int port);
  void Close();

  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

  struct Channel
  {
    int         index;
    std::string name;
    std::string network;
  };

  bool   GetChannels(std::vector<Channel> &channels);

  SOCKET   GetStreamLive(int channel);
  void   AbortStreamLive();
  bool     CanStreamLive(int channel);

private:
  bool   OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool AcceptStreamSocket(SOCKET& socket);

  SOCKET m_socket;
};

