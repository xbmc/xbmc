#include "../../lib/c++/xbmcclient.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

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

  CPacketHELO HeloPackage("Example Remote", ICON_PNG, "../../icons/bluetooth.png");
  HeloPackage.Send(sockfd, my_addr);

  sleep(5);
  // Note that we have foo(BUTTON, DEVICEMAP);
  CPacketBUTTON btn1("dpadup", "XG");
  btn1.Send(sockfd, my_addr);

  sleep(5);

  CPacketBUTTON btn2(0x28);
  btn2.Send(sockfd, my_addr);

  sleep(5);

  CPacketBUTTON btn3("right", "KB");
  btn3.Send(sockfd, my_addr);

  sleep(5);
  // Release button
  CPacketBUTTON btn4;
  btn4.Send(sockfd, my_addr);

  // BYE is not required since XBMC would have shut down
  CPacketBYE bye; // CPacketPing if you want to ping
  bye.Send(sockfd, my_addr);
}
