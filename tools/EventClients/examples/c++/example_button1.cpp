#include "../../lib/c++/xbmcclient.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#ifdef _WIN32
#include <Windows.h> // for sleep
#else
#include <unistd.h>
#endif

int main(int argc, char **argv)
{
  /* connect to localhost, port 9777 using a UDP socket
     this only needs to be done once.
     by default this is where XBMC will be listening for incoming
     connections. */
  CAddress my_addr; // Address => localhost on 9777
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    printf("Error creating socket\n");
    return -1;
  }

  my_addr.Bind(sockfd);

  std::string sIconFile = "../../icons/bluetooth.png";
  unsigned short usIconType = ICON_PNG;

  std::ifstream file (sIconFile, std::ios::in|std::ios::binary|std::ios::ate);
  if (!file.is_open())
  {
    sIconFile = "/usr/share/pixmaps/kodi/bluetooth.png";
    file.open(sIconFile, std::ios::in|std::ios::binary|std::ios::ate);

    if (!file.is_open()) {
      usIconType = ICON_NONE;
    }
    else
    {
      file.close();
    }
  }
  else
  {
    file.close();
  }

  CPacketHELO HeloPackage("Example Remote", usIconType, sIconFile.c_str());
  HeloPackage.Send(sockfd, my_addr);

  sleep(5);
  // press 'S'
  CPacketBUTTON btn1('S', true);
  btn1.Send(sockfd, my_addr);

  sleep(2);
  // press the enter key (13 = enter)
  CPacketBUTTON btn2(13, true);
  btn2.Send(sockfd, my_addr);

  // BYE is not required since XBMC would have shut down
  CPacketBYE bye; // CPacketPing if you want to ping
  bye.Send(sockfd, my_addr);
}
