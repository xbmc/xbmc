//
// Copyright (C) 2010-2011 Stonyx
// http://www.stonyx.com
//
// This library is free software. You can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 (or at your
// option any later version) as published by The Free Software Foundation.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// If you did not received a copy of the GNU General Public License along
// with this library see http://www.gnu.org/copyleft/gpl.html or write to
// The Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <stdint.h>
#include <string.h>

#if defined _WIN32 || defined _WIN64
#include <winsock2.h>
#elif !defined SOCKET
typedef int SOCKET;
#endif

class CSlingbox
{
public:
  // Enum that represents all the possible Slingbox resolutions
  enum Resolution
  {
    NOVIDEO = 0x00000000,
    RESOLUTION128X96 = 0x00000008,
    RESOLUTION160X120 = 0x00000002,
    RESOLUTION176X120 = 0x00000004,
    RESOLUTION224X176 = 0x00000009,
    RESOLUTION256X192 = 0x0000000B,
    RESOLUTION320X240 = 0x00000001,
    RESOLUTION352X240 = 0x00000003,
    RESOLUTION320X480 = 0x00000007,
    RESOLUTION640X240 = 0x00000006,
    RESOLUTION640X480 = 0x00000005
  };

  // Constructors and destructors
  CSlingbox();
  CSlingbox(const char * szAddress, unsigned int uiPort = 5001);
  ~CSlingbox();

  // Following function can be called instead of the SetAddress function and is used to
  // find a Slingbox instead of manually setting address and port information
  bool FindSlingbox(unsigned int uiTimeout = 10);

  // Function used to retrive information about a found Slingbox
  void GetAddress(char * szAddress, unsigned int uiAddressLength, unsigned int * uiPort);

  // Following public functions are listed in order of suggested call sequence
  void SetAddress(const char * szAddress, unsigned int uiPort = 5001);
  bool Connect(bool bLoginAsAdmin, const char * szPassword);
  bool InitializeStream();
  bool StreamSettings(Resolution eResolution = RESOLUTION320X240,
    uint32_t uiVideoBitrate = 704, uint32_t uiFrameRate = 30,
    uint32_t uiVideoSmoothing = 50, uint32_t uiAudioBitrate = 64,
    uint32_t uiIFrameInterval = 10);
  bool StartStream();
  int ReadStream(void * pBuffer, unsigned int uiSize);
  bool StopStream();
  bool Disconnect();

  // Function used to find out if a connecton to the Slingbox is active
  bool IsConnected();

  // Set channel/input related functions can be called anytime after InitializeStream
  // has succeeded
  bool ChannelUp();
  bool ChannelDown();
  bool SetChannel(unsigned int uiChannel);
  bool SetInput(unsigned int uiInput);

  // Get channel function will usually return valid data after InitializeStream
  // has succeeded
  int GetChannel();

  // Get input function will usually return valid data only when a stream is active
  // (ie: after StartStream has succeeded and before StopStream is called)
  int GetInput();

  // Function used to send an IR command
  bool SendIRCommand(uint8_t ucCommand);

protected:
  void init();
  // Function used to send and receive messages to and from the Slingbox
  struct MessageHeader;
  bool SendReceiveMessage(SOCKET socSocket, MessageHeader * pHeader,
    bool bEncrypt = true, unsigned int uiTimeout = 10);
  bool SendMessage(SOCKET socSocket, MessageHeader * pHeader,
    bool bEncrypt = true, unsigned int uiTimeout = 10);
  bool ReceiveMessage(SOCKET socSocket, bool bUDPMessage = false,
    unsigned int uiTimeout = 10);

  // Functions used to encode and decode data to and from the Slingbox
  void Encode(void * pData, unsigned int uiSize);
  void Decode(void * pData, unsigned int uiSize);

  // Connection related functions
  SOCKET OpenSocket(const char * szAddress, unsigned int uiPort, bool bUDP = false);
  int Broadcast(SOCKET socSocket, unsigned int uiPort, void * pBuffer,
    unsigned int uiSize, unsigned int uiTimeout = 10);
  int Send(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
    unsigned int uiTimeout = 10);
  int SendTo(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
    unsigned int uiTimeout, struct sockaddr * pSocketAddress);
  int Receive(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
    unsigned int uiTimeout = 10);
  int ReceiveFrom(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
    unsigned int uiTimeout, struct sockaddr * pSocketAddress);
  bool CloseSocket(SOCKET socSocket);

  // Function used to wait on the Slingbox for various reasons
  void Wait(unsigned int uiMilliseconds);

  // Protected member variables
  SOCKET m_socCommunication;
  SOCKET m_socStream;
  char m_szAddress[1024];
  unsigned int m_uiPort;
  uint16_t m_usCode;
  uint16_t m_usSequence;
  int m_iChannel;
  int m_iInput;

  // Struct to define which messages were received
  struct
  {
    bool bFindMessage;
    bool bConnectMessage;
    bool bInitializationMessage;
    bool bEncryptionMessage;
    bool bSettingsMessage;
    bool bDisconnectMessage;
    bool bStatusMessage;
    bool bChannelMessage;
    bool bInputMessage;
    bool bChannelStatusMessage;
    bool bInputStatusMessage;
    bool bIRMessage;
  } m_receivedMessages;

  // Struct to define the Slingbox message header
  struct MessageHeader
  {
    uint16_t m_usHeader;     // Always 0x0101
    uint16_t m_usCode;       // Grabbed from the first packet from the Slingbox then
                             //   always kept the same
    uint16_t m_usMessageID;  // Unique number to identify the message
    uint16_t m_usVar4;       // Always 0
    uint16_t m_usSequence;   // Sequencial number (answer will have the same number)
    uint16_t m_usDirection;  // 0 from Slingbox and 0x8000 from software
    uint16_t m_usVar7;
    uint16_t m_usVar8;
    uint16_t m_usSize;       // Size of the buffer (without header)
    uint16_t m_usEncoded;    // 0x2000 if buffer is encoded
    uint16_t m_usVar11;
    uint16_t m_usVar12;
    uint16_t m_usVar13;
    uint16_t m_usVar14;
    uint16_t m_usVar15;
    uint16_t m_usVar16;
 
    // Struct constructor that sets all variables to default values
    MessageHeader(uint16_t usMessageID, uint16_t usSize)
    {
      memset(this, 0, sizeof(MessageHeader));
      m_usHeader = 0x0101;
      m_usMessageID = usMessageID;
      m_usSize = usSize - sizeof(MessageHeader);
    }
  };

  // Struct to define the login message
  struct ConnectMessage : public MessageHeader
  {
    uint32_t m_uiUnknown;
    uint16_t m_usAccess[16];
    uint16_t m_usPassword[16];
    uint16_t m_usID[66];

    // Struct constructor that sets variables to default values and copies
    // passed values into variables
    ConnectMessage(bool bLoginAsAdmin, const char * szPassword)
      :MessageHeader(0x0067, sizeof(ConnectMessage))
    {
      m_uiUnknown = 0x00000000;
      CopyCharToShort(m_usAccess, bLoginAsAdmin ? "admin" : "guest", 16);
      CopyCharToShort(m_usPassword, szPassword, 16);
      CopyCharToShort(m_usID, "Slingbox", 66);
    }

    // Function to copy a char array into a short array
    void CopyCharToShort(uint16_t * usTarget, const char * szSource,
      unsigned int uiTargetSize)
    {
      memset(usTarget, 0, uiTargetSize * sizeof(uint16_t));
      for (unsigned int i = 0; i < uiTargetSize && szSource[i] != '\0'; i++)
      {
        usTarget[i] = (uint16_t)szSource[i];
      }
    }
  };

  // Struct to define the initialization message
  struct InitializationMessage : public MessageHeader
  {
    uint32_t m_uiVar1;
    uint32_t m_uiVar2;

    // Struct constructor that sets all variables to default values
    InitializationMessage()
      :MessageHeader(0x007E, sizeof(InitializationMessage))
    {
      m_uiVar1 = 0x00000001;
      m_uiVar2 = 0x00000000;
    }
  };

  // Struct to define the encryption message
  struct EncryptionMessage : public MessageHeader
  {
    uint32_t m_uiData[24];

    // Struct constructor that sets all variables to default values
    EncryptionMessage()
      :MessageHeader(0x00A6, sizeof(EncryptionMessage))
    {
      memset(m_uiData, 0, 96);
      m_uiData[0] = 0x00000100;
      m_uiData[4] = 0x001D0000;
    }
  };

  // Struct to define the settings message
  struct SettingsMessage : public MessageHeader
  {
    uint32_t m_uiData[40];

    // Struct constructor that sets variables to default values and copies
    // passed values into variables
    SettingsMessage(Resolution resolution, uint32_t uiVideoBitrate,
      uint32_t uiFrameRate, uint32_t uiVideoSmoothing, uint32_t uiAudioBitrate,
      uint32_t uiIFrameInterval)
      :MessageHeader(0x00B5, sizeof(SettingsMessage))
    {
      // Make sure all our values are within limits
      if (uiVideoBitrate < 50)
        uiVideoBitrate = 50;
      if (uiVideoBitrate > 8000)
        uiVideoBitrate = 8000;

      if (uiFrameRate < 1)
        uiFrameRate = 1;
      else if (uiFrameRate < 6)
        uiFrameRate = 1;
      else if (uiFrameRate < 10)
        uiFrameRate = 6;
      else if (uiFrameRate < 15)
        uiFrameRate = 10;
      else if (uiFrameRate < 20)
        uiFrameRate = 15;
      else if (uiFrameRate < 30)
        uiFrameRate = 20;
      else
        uiFrameRate = 30;

      if (uiVideoSmoothing > 100)
        uiVideoSmoothing = 100;

      if (uiAudioBitrate < 16)
        uiAudioBitrate = 16;
      else if (uiAudioBitrate < 20)
        uiAudioBitrate = 16;
      else if (uiAudioBitrate < 32)
        uiAudioBitrate = 20;
      else if (uiAudioBitrate < 40)
        uiAudioBitrate = 32;
      else if (uiAudioBitrate < 48)
        uiAudioBitrate = 40;
      else if (uiAudioBitrate < 64)
        uiAudioBitrate = 48;
      else if (uiAudioBitrate < 96)
        uiAudioBitrate = 64;
      else
        uiAudioBitrate = 96;

      if (uiIFrameInterval > 30)
        uiIFrameInterval = 30;

      // Set variables to default values and copy passed values into variables
      m_uiData[0] = 0x000000FF;
      m_uiData[1] = 0x000000FF;
      m_uiData[2] = resolution;
      m_uiData[3] = 0x00000001;
      m_uiData[4] = uiVideoBitrate + (uiFrameRate << 16) +
        (uiIFrameInterval << 24);
      m_uiData[5] = 0x00000001 + (uiVideoSmoothing << 8);
      m_uiData[6] = 0x00000003;
      m_uiData[7] = 0x00000001;
      m_uiData[8] = uiAudioBitrate;
      m_uiData[9] = 0x00000003;
      m_uiData[10] = 0x00000001;
      m_uiData[11] = 0x46D4F252;
      m_uiData[12] = 0x4C5D03E4;
      m_uiData[13] = 0x04CA1F8D;
      m_uiData[14] = 0xF1090089;

      for (int i = 15; i < 40; i++)
        m_uiData[i] = 0x00000000;
    }
  };

  // Struct to define the disconnect message
  struct DisconnectMessage : public MessageHeader
  {
    DisconnectMessage()
      :MessageHeader(0x0068, sizeof(DisconnectMessage))
    {
    }
  };

  // Struct to define the status message
  struct StatusMessage : public MessageHeader
  {
    StatusMessage()
      :MessageHeader(0x0065, sizeof(StatusMessage))
    {
    }
  };

  // Struct to define the channel message
  struct ChannelMessage : public MessageHeader
  {
    uint32_t m_uiUpDown;     // 0 = up & 1 = down when channel = 0
    uint32_t m_uiChannel;
    uint32_t m_uiUnknown1;
    uint32_t m_uiUnknown2;

    // Struct constructor that sets variables to default values and copies
    // passed values into variables
    ChannelMessage(uint32_t uiChannel = 0)
      :MessageHeader(0x0089, sizeof(ChannelMessage))
    {
      m_uiUpDown = 0x00000002;
      m_uiChannel = uiChannel;
      m_uiUnknown1 = 0xFF000000;
      m_uiUnknown2 = 0x00000000;
    }

    // Function that sets variables to values required to change the channel up
    void Up()
    {
      m_uiUpDown = 0x00000000;
      m_uiChannel = 0x00000000;
      m_uiUnknown1 = 0x000000FF;
      m_uiUnknown2 = 0x00000000;
    }

    // Function that sets variables to values required to change the channel down
    void Down()
    {
      m_uiUpDown = 0x00000001;
      m_uiChannel = 0x00000000;
      m_uiUnknown1 = 0x000000FF;
      m_uiUnknown2 = 0x00000000;
    }
  };

  // Struct to define the input message
  struct InputMessage : public MessageHeader
  {
    uint8_t m_ucData[8];

    // Struct constructor that sets variables to default values and copies
    // passed values into variables
    InputMessage(uint8_t ucInput)
      :MessageHeader(0x0085, sizeof(InputMessage))
    {
      memset(m_ucData, 0, sizeof(m_ucData));
      m_ucData[0] = ucInput;
    }
  };

  // Struct to define the IR message
  struct IRMessage : public MessageHeader
  {
    uint8_t m_ucData[480];

    // Struct constructor that sets variables to default values and copies
    // passed values into variables
    IRMessage(uint8_t ucCode)
      :MessageHeader(0x0087, sizeof(IRMessage))
    {
      memset(m_ucData, 0, sizeof(m_ucData));
      m_ucData[0] = ucCode;
      m_ucData[472] = 0xFF;
    }
  };
};
