#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>

#define STD_PORT       9777

#define MS_ABSOLUTE    0x01
//#define MS_RELATIVE    0x02

#define BTN_USE_NAME   0x01
#define BTN_DOWN       0x02
#define BTN_UP         0x04
#define BTN_USE_AMOUNT 0x08
#define BTN_QUEUE      0x10
#define BTN_NO_REPEAT  0x20

#define PT_HELO         0x01
#define PT_BYE          0x02
#define PT_BUTTON       0x03
#define PT_MOUSE        0x04
#define PT_PING         0x05
#define PT_BROADCAST    0x06
#define PT_NOTIFICATION 0x07
#define PT_BLOB         0x08
#define PT_LOG          0x09
#define PT_DEBUG        0xFF

#define ICON_NONE       0x00
#define ICON_JPEG       0x01
#define ICON_PNG        0x02
#define ICON_GIF        0x03

#define MAX_PACKET_SIZE  1024
#define HEADER_SIZE      32
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

class CAddress
{
private:
  struct sockaddr_in m_Addr;
public:
  CAddress(int Port = STD_PORT)
  {
    m_Addr.sin_family = AF_INET;
    m_Addr.sin_port = htons(Port);
    m_Addr.sin_addr.s_addr = INADDR_ANY;
    memset(m_Addr.sin_zero, '\0', sizeof m_Addr.sin_zero);
  }

  CAddress(const char *Address, int Port = STD_PORT)
  {
    m_Addr.sin_port = htons(Port);
    
    struct hostent *h;
    if (Address == NULL || (h=gethostbyname(Address)) == NULL)
    {
        if (Address != NULL)
          herror("gethostbyname");

        m_Addr.sin_addr.s_addr  = INADDR_ANY;
        m_Addr.sin_family       = AF_INET;
    }
    else
    {
      m_Addr.sin_family = h->h_addrtype;
      m_Addr.sin_addr = *((struct in_addr *)h->h_addr);
    }
    memset(m_Addr.sin_zero, '\0', sizeof m_Addr.sin_zero);
  }

  const sockaddr *GetAddress()
  {
    return ((struct sockaddr *)&m_Addr);
  }

  void Bind(int Sockfd)
  {
    bind(Sockfd, (struct sockaddr *)&m_Addr, sizeof m_Addr);
  }
};

class CPacket
{
/*   Base class that implements a single event packet.

     - Generic packet structure (maximum 1024 bytes per packet)
     - Header is 32 bytes long, so 992 bytes available for payload
     - large payloads can be split into multiple packets using H4 and H5
       H5 should contain total no. of packets in such a case
     - H6 contains length of P1, which is limited to 992 bytes
     - if H5 is 0 or 1, then H4 will be ignored (single packet msg)
     - H7 must be set to zeros for now

         -----------------------------
         | -H1 Signature ("XBMC")    | - 4  x CHAR                4B
         | -H2 Version (eg. 2.0)     | - 2  x UNSIGNED CHAR       2B
         | -H3 PacketType            | - 1  x UNSIGNED SHORT      2B
         | -H4 Sequence number       | - 1  x UNSIGNED LONG       4B
         | -H5 No. of packets in msg | - 1  x UNSIGNED LONG       4B
         | -H6 Payload size          | - 1  x UNSIGNED SHORT      2B
         | -H7 Reserved              | - 14 x UNSIGNED CHAR      14B
         |---------------------------|
         | -P1 payload               | -
         -----------------------------
*/
public:
  CPacket()
  {
    m_PacketType = 0;
  }
  virtual ~CPacket()
  { }

  bool Send(int Socket, CAddress &Addr)
  {
    if (m_Payload.size() == 0)
      ConstructPayload();
    bool SendSuccessfull = true;
    int NbrOfPackages = (m_Payload.size() / MAX_PAYLOAD_SIZE) + 1;
    int Send = 0;
    int Sent = 0;
    int Left = m_Payload.size();
    for (int Package = 1; Package <= NbrOfPackages; Package++)
    {
      if (Left > MAX_PAYLOAD_SIZE)
      {
        Send = MAX_PAYLOAD_SIZE;
        Left -= Send;
      }
      else
      {
        Send = Left;
        Left = 0;
      }

      ConstructHeader(m_PacketType, NbrOfPackages, Package, Send, m_Header);
      char t[MAX_PACKET_SIZE];
      int i, j;
      for (i = 0; i < 32; i++)
        t[i] = m_Header[i];

      for (j = 0; j < Send; j++)
        t[(32 + j)] = m_Payload[j + Sent];

      int rtn = sendto(Socket, t, (32 + Send), 0, Addr.GetAddress(), sizeof(struct sockaddr));

      if (rtn != (32 + Send))
        SendSuccessfull = false;

      Sent += Send;
    }
    return SendSuccessfull;
  }
protected:
  char            m_Header[HEADER_SIZE];
  unsigned short  m_PacketType;

  std::vector<char> m_Payload;

  static void ConstructHeader(int PacketType, int NumberOfPackets, int CurrentPacket, unsigned short PayloadSize, char *Header)
  {
    sprintf(Header, "XBMC");
    for (int i = 4; i < HEADER_SIZE; i++)
      Header[i] = 0;
    Header[4]  = MAJOR_VERSION;
    Header[5]  = MINOR_VERSION;
    if (CurrentPacket == 1)
    {
      Header[6]  = ((PacketType & 0xff00) >> 8);
      Header[7]  =  (PacketType & 0x00ff);
    }
    else
    {
      Header[6]  = ((PT_BLOB & 0xff00) >> 8);
      Header[7]  =  (PT_BLOB & 0x00ff);
    }
    Header[8]  = ((CurrentPacket & 0xff000000) >> 24);
    Header[9]  = ((CurrentPacket & 0x00ff0000) >> 16);
    Header[10] = ((CurrentPacket & 0x0000ff00) >> 8);
    Header[11] =  (CurrentPacket & 0x000000ff);

    Header[12] = ((NumberOfPackets & 0xff000000) >> 24);
    Header[13] = ((NumberOfPackets & 0x00ff0000) >> 16);
    Header[14] = ((NumberOfPackets & 0x0000ff00) >> 8);
    Header[15] =  (NumberOfPackets & 0x000000ff);

    Header[16] = ((PayloadSize & 0xff00) >> 8);
    Header[17] =  (PayloadSize & 0x00ff);
  }

  virtual void ConstructPayload()
  { }
};

class CPacketHELO : public CPacket
{
    /************************************************************************/
    /* Payload format                                                       */
    /* %s -  device name (max 128 chars)                                    */
    /* %c -  icontype ( 0=>NOICON, 1=>JPEG , 2=>PNG , 3=>GIF )              */
    /* %s -  my port ( 0=>not listening )                                   */
    /* %d -  reserved1 ( 0 )                                                */
    /* %d -  reserved2 ( 0 )                                                */
    /* XX -  imagedata ( can span multiple packets )                        */
    /************************************************************************/
private:
  std::vector<char> m_DeviceName;
  unsigned short m_IconType;
  char *m_IconData;
  unsigned short m_IconSize;
public:
  virtual void ConstructPayload()
  {
    m_Payload.clear();

    for (unsigned int i = 0; i < m_DeviceName.size(); i++)
      m_Payload.push_back(m_DeviceName[i]);

    m_Payload.push_back('\0');

    m_Payload.push_back(m_IconType);

    m_Payload.push_back(0);
    m_Payload.push_back('\0');

    for (int j = 0; j < 8; j++)
      m_Payload.push_back(0);

    for (int ico = 0; ico < m_IconSize; ico++)
      m_Payload.push_back(m_IconData[ico]);
  }

  CPacketHELO(const char *DevName, unsigned short IconType, const char *IconFile = NULL) : CPacket()
  {
    m_PacketType = PT_HELO;

    unsigned int len = strlen(DevName);
    for (unsigned int i = 0; i < len; i++)
      m_DeviceName.push_back(DevName[i]);    

    m_IconType = IconType;

    if (IconType == ICON_NONE || IconFile == NULL)
    {
      m_IconData = NULL;
      m_IconSize = 0;
      return;
    }

    std::ifstream::pos_type size;

    std::ifstream file (IconFile, std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
      size = file.tellg();
      m_IconData = new char [size];
      file.seekg (0, std::ios::beg);
      file.read (m_IconData, size);
      file.close();
      m_IconSize = size;
    }
    else
    {
      m_IconType = ICON_NONE;
      m_IconSize = 0;
    }
  }

  virtual ~CPacketHELO()
  {
    m_DeviceName.clear();
    if (m_IconData)
      free(m_IconData);
  }
};

class CPacketNOTIFICATION : public CPacket
{
    /************************************************************************/
    /* Payload format:                                                      */
    /* %s - caption                                                         */
    /* %s - message                                                         */
    /* %c - icontype ( 0=>NOICON, 1=>JPEG , 2=>PNG , 3=>GIF )               */
    /* %d - reserved ( 0 )                                                  */
    /* XX - imagedata ( can span multiple packets )                         */
    /************************************************************************/
private:
  std::vector<char> m_Title;
  std::vector<char> m_Message;
  unsigned short m_IconType;
  char *m_IconData;
  unsigned short m_IconSize;
public:
  virtual void ConstructPayload()
  {
    m_Payload.clear();

    for (unsigned int i = 0; i < m_Title.size(); i++)
      m_Payload.push_back(m_Title[i]);

    m_Payload.push_back('\0');

    for (unsigned int i = 0; i < m_Message.size(); i++)
      m_Payload.push_back(m_Message[i]);

    m_Payload.push_back('\0');

    m_Payload.push_back(m_IconType);

    for (int i = 0; i < 4; i++)
      m_Payload.push_back(0);

    for (int ico = 0; ico < m_IconSize; ico++)
      m_Payload.push_back(m_IconData[ico]);
  }

  CPacketNOTIFICATION(const char *Title, const char *Message, unsigned short IconType, const char *IconFile = NULL) : CPacket()
  {
    m_PacketType = PT_NOTIFICATION;
    m_IconData = NULL;

    unsigned int len = 0;
    if (Title != NULL)
    {
      len = strlen(Title);
      for (unsigned int i = 0; i < len; i++)
        m_Title.push_back(Title[i]);
    }

    if (Message != NULL)
    {
      len = strlen(Message);
      for (unsigned int i = 0; i < len; i++)
        m_Message.push_back(Message[i]);
    }
    m_IconType = IconType;

    if (IconType == ICON_NONE || IconFile == NULL)
      return;

    std::ifstream::pos_type size;

    std::ifstream file (IconFile, std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
      size = file.tellg();
      m_IconData = new char [size];
      file.seekg (0, std::ios::beg);
      file.read (m_IconData, size);
      file.close();
      m_IconSize = size;
    }
    else
    {
      m_IconType = ICON_NONE;
      m_IconSize = 0;
    }
  }

  virtual ~CPacketNOTIFICATION()
  {
    m_Title.clear();
    m_Message.clear();
    if (m_IconData)
      free(m_IconData);
  }
};

class CPacketBUTTON : public CPacket
{
    /************************************************************************/
    /* Payload format                                                       */
    /* %i - button code                                                     */
    /* %i - flags 0x01 => use button map/name instead of code               */
    /*            0x02 => btn down                                          */
    /*            0x04 => btn up                                            */
    /*            0x08 => use amount                                        */
    /*            0x10 => queue event                                       */
    /*            0x20 => do not repeat                                     */
    /* %i - amount ( 0 => 65k maps to -1 => 1 )                             */
    /* %s - device map (case sensitive and required if flags & 0x01)        */
    /*      "KB" - Standard keyboard map                                    */
    /*      "XG" - Xbox Gamepad                                             */
    /*      "R1" - Xbox Remote                                              */
    /*      "R2" - Xbox Universal Remote                                    */
    /*      "LI:devicename" -  valid LIRC device map where 'devicename'     */
    /*                         is the actual name of the LIRC device        */
    /* %s - button name (required if flags & 0x01)                          */
    /************************************************************************/
private:
  std::vector<char> m_DeviceMap;
  std::vector<char> m_Button;
  unsigned short m_ButtonCode;
  unsigned short m_Amount;
  bool m_Repeat, m_Down, m_Queue;
public:
  virtual void ConstructPayload()
  {
    m_Payload.clear();
    unsigned short Flags = 0;

    if (m_Button.size() != 0 && m_DeviceMap.size() != 0)
    {
      Flags |= BTN_USE_NAME;
      m_ButtonCode = 0;
    }
    else
    {
      m_DeviceMap.clear();
      m_Button.clear();
    }
    if (m_Amount > 0)
      Flags |= BTN_USE_AMOUNT;

    if (m_Down)
      Flags |= BTN_DOWN;
    else
      Flags |= BTN_UP;

    if (!m_Repeat)
      Flags |= BTN_NO_REPEAT;

    if (m_Queue)
      Flags |= BTN_QUEUE;

    m_Payload.push_back(((m_ButtonCode & 0xff00) >> 8));
    m_Payload.push_back( (m_ButtonCode & 0x00ff));

    m_Payload.push_back(((Flags & 0xff00) >> 8) );
    m_Payload.push_back( (Flags & 0x00ff));

    m_Payload.push_back(((m_Amount & 0xff00) >> 8) );
    m_Payload.push_back( (m_Amount & 0x00ff));


    for (unsigned int i = 0; i < m_DeviceMap.size(); i++)
      m_Payload.push_back(m_DeviceMap[i]);

    m_Payload.push_back('\0');

    for (unsigned int i = 0; i < m_Button.size(); i++)
      m_Payload.push_back(m_Button[i]);

    m_Payload.push_back('\0');
  }

  CPacketBUTTON(const char *Button, const char *DeviceMap, bool Queue = false, bool Repeat = true, bool Down = true, unsigned short Amount = 0) : CPacket()
  {
    m_PacketType = PT_BUTTON;

    m_Repeat = Repeat;
    m_Down   = Down;
    m_Queue  = Queue;
    m_Amount = Amount;
    m_ButtonCode = 0;

    unsigned int len = strlen(DeviceMap);
    for (unsigned int i = 0; i < len; i++)
      m_DeviceMap.push_back(DeviceMap[i]);

    len = strlen(Button);
    for (unsigned int i = 0; i < len; i++)
      m_Button.push_back(Button[i]);
  }

  CPacketBUTTON(unsigned short ButtonCode, bool Queue = false, bool Repeat = true, bool Down = true, unsigned short Amount = 0) : CPacket()
  {
    m_PacketType = PT_BUTTON;

    m_Repeat = Repeat;
    m_Down   = Down;
    m_Queue  = Queue;
    m_Amount = Amount;
    m_ButtonCode = ButtonCode;
  }

  // Used to send a release event
  CPacketBUTTON() : CPacket()
  {
    m_PacketType = PT_BUTTON;

    m_Repeat = false;
    m_Down   = false;
    m_Queue  = false;
    m_Amount = 0;
    m_ButtonCode = 0;
  }

  virtual ~CPacketBUTTON()
  {
    m_DeviceMap.clear();
    m_Button.clear();
  }
};

class CPacketPING : public CPacket
{
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/
public:
  CPacketPING() : CPacket()
  {
    m_PacketType = PT_PING;
  }
  virtual ~CPacketPING()
  { }
};

class CPacketBYE : public CPacket
{
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/
public:
  CPacketBYE() : CPacket()
  {
    m_PacketType = PT_BYE;
  }
  virtual ~CPacketBYE()
  { }
};

class CPacketMOUSE : public CPacket
{
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - flags                                                           */
    /*    - 0x01 absolute position                                          */
    /* %i - mousex (0-65535 => maps to screen width)                        */
    /* %i - mousey (0-65535 => maps to screen height)                       */
    /************************************************************************/
private:
  unsigned short m_X;
  unsigned short m_Y;
  unsigned char  m_Flag;
public:
  CPacketMOUSE(int X, int Y, unsigned char Flag = MS_ABSOLUTE)
  {
    m_PacketType = PT_MOUSE;
    m_Flag = Flag;
    m_X = X;
    m_Y = Y;
  }

  virtual void ConstructPayload()
  {
    m_Payload.clear();

    m_Payload.push_back(m_Flag);

    m_Payload.push_back(((m_X & 0xff00) >> 8));
    m_Payload.push_back( (m_X & 0x00ff));

    m_Payload.push_back(((m_Y & 0xff00) >> 8));
    m_Payload.push_back( (m_Y & 0x00ff));
  }
  
  virtual ~CPacketMOUSE()
  { }
};

class CPacketLOG : public CPacket
{
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - log type                                                        */
    /* %s - message                                                         */
    /************************************************************************/
private:
  std::vector<char> m_Message;
  unsigned char  m_LogType;
public:
  CPacketLOG(int LogType, const char *Message)
  {
    m_PacketType = PT_LOG;

    unsigned int len = strlen(Message);
    for (unsigned int i = 0; i < len; i++)
      m_Message.push_back(Message[i]);

    m_LogType = LogType;
  }

  virtual void ConstructPayload()
  {
    m_Payload.clear();

    m_Payload.push_back( (m_LogType & 0x00ff) );

    for (unsigned int i = 0; i < m_Message.size(); i++)
      m_Payload.push_back(m_Message[i]);
    m_Payload.push_back('\0');
  }
  
  virtual ~CPacketLOG()
  { }
};
