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

#include "SlingboxLib.h"

#if defined _WIN32 || defined _WIN64
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

// ********************
// Public Functions
// ********************

// Default constructor.
CSlingbox::CSlingbox()
{
  // initialize all members
  init();
}

// Alternative constructor used to set the address (IP or hostname) and port of the
// Slingbox we want to connect to. If no port is specified default port 5001 is used.
CSlingbox::CSlingbox(const char * szAddress, unsigned int uiPort /* = 5001 */)
{
  // initialize all members
  init();

  // Set address and port variables to passed values
  SetAddress(szAddress, uiPort);
}

// Destructor.
CSlingbox::~CSlingbox()
{
  // Close all connections if they are still open
  if (m_socStream != INVALID_SOCKET)
    CloseSocket(m_socStream);
  if (m_socCommunication != INVALID_SOCKET)
    CloseSocket(m_socCommunication);
}

// Function used to initialize class members
void CSlingbox::init()
{
  // Set all variables to default/invalid values
  m_socCommunication = INVALID_SOCKET;
  m_socStream = INVALID_SOCKET;
  memset(m_szAddress, 0, sizeof(m_szAddress));
  m_uiPort = 0;
  m_usCode = 0;
  m_usSequence = 0;
  m_iChannel = -1;
  m_iInput = -1;
  memset(&m_receivedMessages, 0, sizeof(m_receivedMessages));
}
// Function used to find a Slingbox.  Address and port of found Slingbox can be
// retrieved with the GetAddress function afterwards.
bool CSlingbox::FindSlingbox(unsigned int uiTimeout /* = 10 */)
{
  // Open a UDP connection
  SOCKET socSocket = OpenSocket(NULL, 0, true);
  if (socSocket != INVALID_SOCKET)
  {
    // Prepare and send data
    uint32_t uiData[8];
    memset(uiData, 0, sizeof(uiData));
    uiData[0] = 0x00000101;
    uiData[1] = 0x00000002;
    if (Broadcast(socSocket, 5004, uiData, sizeof(uiData), uiTimeout) <= 0)
      return false;

    // Reset address and port variables
    memset(m_szAddress, 0, sizeof(m_szAddress));
    m_uiPort = 0;

    // Give the Slingbox time to respond
    Wait(250);

    // Look for correct return message and properly set variables
    if (!ReceiveMessage(socSocket, true, uiTimeout) ||
      !m_receivedMessages.bFindMessage ||
      strlen(m_szAddress) == 0 || m_uiPort == 0)
    {
      CloseSocket(socSocket);
      return false;
    }

    // Close socket
    CloseSocket(socSocket);
  }
  else
  {
    return false;
  }

  return true;
}

// Function used to retrieve the address and port of a Slingbox found with the
// FindSlingbox function.
void CSlingbox::GetAddress(char * szAddress, unsigned int uiAddressLength,
  unsigned int * uiPort)
{
  // Copy requested data into passed pointers
  memset(szAddress, 0, uiAddressLength);
  strncpy(szAddress, m_szAddress, uiAddressLength - 1);
  *uiPort = m_uiPort;
}

// Function used to set the address (IP or hostname) and port of the Slingbox we
// want to connect to.  If no port is specified default port 5001 is used.
void CSlingbox::SetAddress(const char * szAddress, unsigned int uiPort /* = 5001 */)
{
  // Set address and port variables to passed values
  strncpy(m_szAddress, szAddress, sizeof(m_szAddress) - 1);
  m_uiPort = uiPort;
}

// Function used to login into the Slingbox.
bool CSlingbox::Connect(bool bLoginAsAdmin, const char * szPassword)
{
  // Check if a communication connection is already open
  if (m_socCommunication != INVALID_SOCKET)
    return false;

#if defined _WIN32 || defined _WIN64
  // Enable use of the Winsock DLL
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    return false;
#endif

  // Open the communication connection
  m_socCommunication = OpenSocket(m_szAddress, m_uiPort);
  if (m_socCommunication == INVALID_SOCKET)
  {
#if defined _WIN32 || defined _WIN64
    // Finish using the Winsock DLL
    WSACleanup();
#endif

    return false;
  }

  // Prepare and send string
  char * szString = "GET /stream.asf HTTP/1.1\r\nAccept: */*\r\n"
    "Pragma: Sling-Connection-Type=Control, Session-Id=0\r\n\r\n";
  if (Send(m_socCommunication, (void *)szString, strlen(szString)) <= 0)
  {
    // Close the communication connection
    CloseSocket(m_socCommunication);
    m_socCommunication = INVALID_SOCKET;

#if defined _WIN32 || defined _WIN64
    // Finish using the Winsock DLL
    WSACleanup();
#endif

    return false;
  }

  // Invalidate variables
  m_usCode = 0;
  m_usSequence = 0;
  m_iChannel = -1;
  m_iInput = -1;

  // Prepare and send the connect message
  ConnectMessage message(bLoginAsAdmin, szPassword);
  if (!SendReceiveMessage(m_socCommunication, &message) ||
    !m_receivedMessages.bConnectMessage)
  {
    // Close the communication connection
    CloseSocket(m_socCommunication);
    m_socCommunication = INVALID_SOCKET;

#if defined _WIN32 || defined _WIN64
    // Finish using the Winsock DLL
    WSACleanup();
#endif

    return false;
  }

  // Check if we got a valid code
  if (m_usCode == 0)
  {
    // Close the communication connection
    CloseSocket(m_socCommunication);
    m_socCommunication = INVALID_SOCKET;

#if defined _WIN32 || defined _WIN64
    // Finish using the Winsock DLL
    WSACleanup();
#endif

    return false;
  }

  return true;
}

// Function used to send the initialization message and initialize the stream.
bool CSlingbox::InitializeStream()
{
  // Prepare and send start message
  InitializationMessage message;
  return SendReceiveMessage(m_socCommunication, &message) &&
    m_receivedMessages.bInitializationMessage;
}

// Function used to set the stream settings.
bool CSlingbox::StreamSettings(Resolution eResolution /* = RESOLUTION320X240 */,
  uint32_t uiVideoBitrate /* = 704 */, uint32_t uiFrameRate /* = 30 */,
  uint32_t uiVideoSmoothing /* = 50 */, uint32_t uiAudioBitrate /* = 64 */,
  uint32_t uiIFrameInterval /* = 10 */)
{
  // Check if a resolution was specified that requires encryption
  if (eResolution == RESOLUTION320X480 ||
    eResolution == RESOLUTION640X240 ||
    eResolution == RESOLUTION640X480)
  {
    // Enable encryption to enable requested resolution
    EncryptionMessage message;
    if (!SendReceiveMessage(m_socCommunication, &message) ||
      !m_receivedMessages.bEncryptionMessage)
      return false;
  }

  // Create and send video message with all the stream details
  SettingsMessage message(eResolution, uiVideoBitrate, uiFrameRate, uiVideoSmoothing,
    uiAudioBitrate, uiIFrameInterval);
  if (!SendReceiveMessage(m_socCommunication, &message) ||
    !m_receivedMessages.bSettingsMessage)
    return false;

  // Give the Slingbox time to change settings
  Wait(500);

  return true;
}

// Function used to start the stream.
bool CSlingbox::StartStream()
{
  // Check if a stream connection is already open
  if (m_socStream != INVALID_SOCKET)
    return false;

  // Open a new connection
  m_socStream = OpenSocket(m_szAddress, m_uiPort);
  if (m_socStream != INVALID_SOCKET)
  {
    // Prepare and send string
    char szString[128] = "GET /stream.asf HTTP/1.1\r\nAccept: */*\r\n"
      "Pragma: Sling-Connection-Type=Stream, Session-Id=";
    sprintf(&szString[strlen(szString)], "%u", m_usCode);
    strcpy(&szString[strlen(szString)], "\r\n\r\n");
    if (Send(m_socStream, (void *)szString, strlen(szString)) <= 0)
    {
      // Close the stream connection
      CloseSocket(m_socStream);
      m_socStream = INVALID_SOCKET;

      return false;
    }

    // Invalidate channel and input variables
    m_iChannel = -1;
    m_iInput = -1;

    // Give the Slingbox time to respond
    Wait(250);

    // Check for correct return message
    if (!ReceiveMessage(m_socCommunication) ||
      !(m_receivedMessages.bChannelStatusMessage ||
      m_receivedMessages.bInputStatusMessage))
    {
      // Close the stream connection
      CloseSocket(m_socStream);
      m_socStream = INVALID_SOCKET;

      return false;
    }
  }
  else
  {
    return false;
  }

  return true;
}

// Function used to read the stream.  Returns number of bytes actually received
// or -1 if an error occured.
int CSlingbox::ReadStream(void * pBuffer, unsigned int uiSize)
{
  return Receive(m_socStream, pBuffer, uiSize);
}

// Function used to stop the stream.
bool CSlingbox::StopStream()
{
  // Close the stream connection
  bool bSuccess = CloseSocket(m_socStream);
  m_socStream = INVALID_SOCKET;

  // Invalidate channel and input variables
  m_iChannel = -1;
  m_iInput = -1;

  return bSuccess;
}

// Function used to disconnect from the Slingbox
bool CSlingbox::Disconnect()
{
  // Prepare variables
  bool bSuccess = true;

  // Prepare and send the disconnect message
  DisconnectMessage message;
  if (!SendReceiveMessage(m_socCommunication, &message) ||
    !m_receivedMessages.bDisconnectMessage)
    bSuccess = false;

  // Close the stream connection if it's still active
  if (m_socStream != INVALID_SOCKET && !StopStream())
    bSuccess = false;

  // Close the communication connection
  if (!CloseSocket(m_socCommunication))
    bSuccess = false;
  m_socCommunication = INVALID_SOCKET;

#if defined _WIN32 || defined _WIN64
  // Finish using the Winsock DLL
  if (WSACleanup() != 0)
    bSuccess = false;
#endif

  // Invalidate variables
  m_usCode = 0;
  m_usSequence = 0;
  m_iChannel = -1;
  m_iInput = -1;

  return bSuccess;
}

// Function used to check if a connection to the Slingbox is active
bool CSlingbox::IsConnected()
{
  // Prepare and send status message
  StatusMessage message;
  return SendReceiveMessage(m_socCommunication, &message) &&
    m_receivedMessages.bStatusMessage;
}

// Function used to change the channel up.
bool CSlingbox::ChannelUp()
{
  // Prepare and send channel message
  ChannelMessage message;
  message.Up();
  if (!SendMessage(m_socCommunication, &message))
    return false;

  // Invalidate channel variable
  m_iChannel = -1;

  // Give the Slingbox time to change things
  Wait(1000);

  // Check for return message
  return ReceiveMessage(m_socCommunication) &&
    m_receivedMessages.bChannelMessage;
}

// Function used to change the channel down.
bool CSlingbox::ChannelDown()
{
  // Prepare and send channel message
  ChannelMessage message;
  message.Down();
  if (!SendMessage(m_socCommunication, &message))
    return false;

  // Invalidate channel variable
  m_iChannel = -1;

  // Give the Slingbox time to change things
  Wait(1000);

  // Check for return message
  return ReceiveMessage(m_socCommunication) &&
    m_receivedMessages.bChannelMessage;
}

// Function used to set the channel.
bool CSlingbox::SetChannel(unsigned int uiChannel)
{
  // Prepare and send channel message
  ChannelMessage message(uiChannel);
  if (!SendMessage(m_socCommunication, &message))
    return false;

  // Invalidate channel variable
  m_iChannel = -1;

  // Give the Slingbox time to change things
  Wait(1000);

  // Check for return message
  return ReceiveMessage(m_socCommunication) &&
    m_receivedMessages.bChannelMessage;
}

// Function used to set the input.
bool CSlingbox::SetInput(unsigned int uiInput)
{
  // Prepare and send input message
  InputMessage message(uiInput);
  if (!SendMessage(m_socCommunication, &message))
    return false;

  // Invalidate input variable
  m_iInput = -1;

  // Give the Slingbox time to change things
  Wait(1000);

  // Check for return message
  return ReceiveMessage(m_socCommunication) &&
    m_receivedMessages.bInputMessage;
}

// Function used to get the current channel.  Returns -1 if valid data is not
// available.
int CSlingbox::GetChannel()
{
  return m_iChannel;
}

// Function used to get the current input.  Returns -1 if valid data is not
// available.
int CSlingbox::GetInput()
{
  return m_iInput;
}

// Function used to send IR commands.
bool CSlingbox::SendIRCommand(uint8_t ucCode)
{
  // Prepare and send IR message
  IRMessage message(ucCode);
  if (!SendMessage(m_socCommunication, &message))
    return false;

  // Give the Slingbox time to send command
  Wait(1000);

  // Check for return message
  return ReceiveMessage(m_socCommunication) &&
    m_receivedMessages.bIRMessage;
}

// ********************
// Protected Functions
// ********************

// Function used to send a message and receive messages.
bool CSlingbox::SendReceiveMessage(SOCKET socSocket, MessageHeader * pHeader,
  bool bEncrypt /* = true */, unsigned int uiTimeout /* = 10 */)
{
  // Send message
  if (!SendMessage(socSocket, pHeader, bEncrypt, uiTimeout))
    return false;

  // Give the Slingbox time to respond
  Wait(250);

  // Receive messages
  if (!ReceiveMessage(socSocket, false, uiTimeout))
    return false;

  return true;
}

// Function used to send a messages to the Slingbox.
bool CSlingbox::SendMessage(SOCKET socSocket, MessageHeader * pHeader,
  bool bEncode /* = true */, unsigned int uiTimeout /* = 10 */)
{
  // Set message code and sequence numbers
  pHeader->m_usCode = m_usCode;
  pHeader->m_usSequence = m_usSequence;
  m_usSequence++;

  // Check if we need to encode the data from the message
  if (bEncode)
  {
    void* pPointer = ((uint8_t *)pHeader) + sizeof(MessageHeader);
    Encode(pPointer, pHeader->m_usSize);
    pHeader->m_usEncoded = 0x2000;
  }

  // Send the message
  if (Send(socSocket, pHeader, sizeof(MessageHeader) +
    pHeader->m_usSize, uiTimeout) <= 0)
    return false;

  return true;
}

// Function used to receive messages from the Slingbox and process them.
bool CSlingbox::ReceiveMessage(SOCKET socSocket, bool bUDPMessage /* = false */,
  unsigned int uiTimeout /* = 10 */)
{
  // Prepare variables
  bool bMessageReceived = false;
  int iReceived;
  int iProcessed = 0;
  uint8_t ucBuffer[1024];
  sockaddr sSocketAddress;
  memset(&sSocketAddress, 0, sizeof(sSocketAddress));

  // Reset received messages struct
  memset(&m_receivedMessages, 0, sizeof(m_receivedMessages));

  // Get the data
  if (bUDPMessage)
  {
    iReceived = ReceiveFrom(socSocket, ucBuffer, sizeof(ucBuffer), uiTimeout,
      &sSocketAddress);
  }
  else
  {
    iReceived = Receive(socSocket, ucBuffer, sizeof(ucBuffer), uiTimeout);
  }

  // Check for errors
  if (iReceived == SOCKET_ERROR)
    return false;

  // Loop until we've processed all received data
  while (iProcessed < iReceived)
  {
    // Prepare pointer
    MessageHeader * pHeader = (MessageHeader *)(ucBuffer + iProcessed);

    // Make sure it's a message and we got enough data to work with
    if (pHeader->m_usHeader == 0x0101 && (iReceived - iProcessed) >=
      (int)(sizeof(MessageHeader) + pHeader->m_usSize))
    {
      // Signal that we've recevied a message
      bMessageReceived = true;

      // Decode the message data
      if (pHeader->m_usEncoded == 0x2000)
      {
        Decode((uint8_t *)pHeader + sizeof(MessageHeader), pHeader->m_usSize);
        pHeader->m_usEncoded = 0x0000;
      }

      // Find message
      if (pHeader->m_usMessageID == 0x0002 && pHeader->m_usSize == 0x005C)
      {
        m_receivedMessages.bFindMessage = true;

        // Remove the port information from the structure received from ReceiveFrom
        if (sSocketAddress.sa_family == AF_INET)
          ((sockaddr_in *)&sSocketAddress)->sin_port = 0;
        else if (sSocketAddress.sa_family == AF_INET6)
          ((sockaddr_in6 *)&sSocketAddress)->sin6_port = 0;

        // Convert the address received from ReceiveFrom to a string
#if defined _WIN32 || defined _WIN64
        unsigned long ulSizeOfAddress = sizeof(m_szAddress);
        WSAAddressToString(&sSocketAddress, sizeof(sSocketAddress), NULL,
          m_szAddress, &ulSizeOfAddress);
#else
        socklen_t iSizeOfAddress = sizeof(m_szAddress);
        if (sSocketAddress.sa_family == AF_INET)
          inet_ntop(sSocketAddress.sa_family,
          &((sockaddr_in *)&sSocketAddress)->sin_addr, m_szAddress, iSizeOfAddress);
        else if (sSocketAddress.sa_family == AF_INET6)
          inet_ntop(sSocketAddress.sa_family,
          &((sockaddr_in6 *)&sSocketAddress)->sin6_addr, m_szAddress, iSizeOfAddress);
#endif

        // Get the port information from the received data
        m_uiPort = ucBuffer[120] + (ucBuffer[121] << 8);
      }
      // Connect message
      else if (pHeader->m_usMessageID == 0x0067 && pHeader->m_usSize == 0x0008)
      {
        m_receivedMessages.bConnectMessage = true;

        m_usCode = pHeader->m_usCode;
      }
      // Initialization message
      else if (pHeader->m_usMessageID == 0x007E)
      {
        m_receivedMessages.bInitializationMessage = true;
      }
      // Encryption message
      else if (pHeader->m_usMessageID == 0x00A6)
      {
        m_receivedMessages.bEncryptionMessage = true;
      }
      // Settings message
      else if (pHeader->m_usMessageID == 0x00B5)
      {
        m_receivedMessages.bSettingsMessage = true;
      }
      // Disconnect message
      else if (pHeader->m_usMessageID == 0x0068)
      {
        m_receivedMessages.bDisconnectMessage = true;
      }
      // Status message
      else if (pHeader->m_usMessageID == 0x0065 && pHeader->m_usSize == 0x0000)
      {
        m_receivedMessages.bStatusMessage = true;
      }
      // Channel message
      else if (pHeader->m_usMessageID == 0x0089)
      {
        m_receivedMessages.bChannelMessage = true;
      }
      // Channel status massage
      else if (pHeader->m_usMessageID == 0x0065 && pHeader->m_usSize == 0x0078)
      {
        m_receivedMessages.bChannelStatusMessage = true;

        m_iChannel = *(uint16_t *)((uint8_t *)pHeader + sizeof(MessageHeader));
      }
      // Input message
      else if (pHeader->m_usMessageID == 0x0085)
      {
        m_receivedMessages.bInputMessage = true;
      }
      // Input status message
      else if (pHeader->m_usMessageID == 0x0065 && pHeader->m_usSize == 0x0008)
      {
        m_receivedMessages.bInputStatusMessage = true;

        m_iInput = *((uint8_t *)pHeader + sizeof(MessageHeader));
      }
      // IR message
      else if (pHeader->m_usMessageID == 0x0087)
      {
        m_receivedMessages.bIRMessage = true;
      }

      // Increase our counter
      iProcessed += sizeof(MessageHeader) + pHeader->m_usSize;
    }
    else
    {
      return bMessageReceived;
    }
  }

  return bMessageReceived;
}

// Function used to encode data being send to the Slingbox.
void CSlingbox::Encode(void * pData, unsigned int iSize)
{
  for (unsigned int i = 0; i < iSize; i += 8)
  {
    // Prepare variables
    const uint32_t ulKey[] = {0xBCDEAAAA, 0x87FBBBBA, 0x7CCCCFFA, 0xDDDDAABC};
    uint32_t ulVar1 = ((uint32_t *)&((uint8_t *)pData)[i])[0];
    uint32_t ulVar2 = ((uint32_t *)&((uint8_t *)pData)[i])[1];
    uint32_t ulVar3 = 0;

    // Encode
    for (unsigned int j = 0; j < 32; j++)
    {
      ulVar3 -= 0x61C88647;
      ulVar1 += ((ulVar2 >> 5) + ulKey[1]) ^ ((ulVar2 << 4) + ulKey[0]) ^ (ulVar3 +
        ulVar2);
      ulVar2 += ((ulVar1 >> 5) + ulKey[3]) ^ ((ulVar1 << 4) + ulKey[2]) ^ (ulVar3 +
        ulVar1);
    }

    // Finish up
    ((uint32_t *)&((uint8_t *)pData)[i])[0] = ulVar1;
    ((uint32_t *)&((uint8_t *)pData)[i])[1] = ulVar2;
  }
}

// Function used to decode data being received from the Slingbox.
void CSlingbox::Decode(void * pData, unsigned int iSize)
{
  for (unsigned int i = 0 ; i < iSize; i += 8 )
  {
    // Prepare variables
    const uint32_t ulKey[] = {0xBCDEAAAA, 0x87FBBBBA, 0x7CCCCFFA, 0xDDDDAABC};
    uint32_t ulVar1 = ((uint32_t *)&((uint8_t *)pData)[i])[0];
    uint32_t ulVar2 = ((uint32_t *)&((uint8_t *)pData)[i])[1];
    uint32_t ulVar3 = 0xC6EF3720;

    // Decode
    for (unsigned int j = 0; j < 32; j++)
    {
      ulVar2 -= ((ulVar1 >> 5) + ulKey[3]) ^ ((ulVar1 << 4) + ulKey[2]) ^ (ulVar3 +
        ulVar1);
      ulVar1 -= ((ulVar2 >> 5) + ulKey[1]) ^ ((ulVar2 << 4) + ulKey[0]) ^ (ulVar3 +
        ulVar2);
      ulVar3 += 0x61C88647;
    }

    // Finish up
    ((uint32_t *)&((uint8_t *)pData)[i])[0] = ulVar1;
    ((uint32_t *)&((uint8_t *)pData)[i])[1] = ulVar2;
  }
}

// Function used to open a new socket.
SOCKET CSlingbox::OpenSocket(const char * szAddress, unsigned int uiPort,
  bool bUDP /* = false */)
{
  // Prepare needed variables
  SOCKET socSocket = INVALID_SOCKET;
  struct addrinfo * pAddressInfo;
  struct addrinfo addressInfoHints;
  struct addrinfo * pPointer;
  memset(&addressInfoHints, 0, sizeof(addressInfoHints));
  addressInfoHints.ai_family = bUDP ? AF_INET : AF_UNSPEC;
  addressInfoHints.ai_socktype = bUDP ? SOCK_DGRAM : SOCK_STREAM;
  addressInfoHints.ai_protocol = bUDP ? IPPROTO_UDP : IPPROTO_TCP;
  addressInfoHints.ai_flags = bUDP ? AI_PASSIVE : 0;
  char szPort[32];
  sprintf(szPort, "%u", uiPort);

  // Get address info
  if (getaddrinfo(szAddress, szPort, &addressInfoHints, &pAddressInfo) != 0)
    return INVALID_SOCKET;

  // Loop thru all the received address info
  for (pPointer = pAddressInfo; pPointer != NULL; pPointer = pPointer->ai_next)
  {
    // Open a new socket
    socSocket = socket(pPointer->ai_family, pPointer->ai_socktype,
      pPointer->ai_protocol);
    if (socSocket != INVALID_SOCKET)
    {
      // Bind if we're using UDP
      if (bUDP)
      {
        if (bind(socSocket, pAddressInfo->ai_addr, pAddressInfo->ai_addrlen) == 0)
        {
          break;
        }
        else
        {
          CloseSocket(socSocket);
          socSocket = INVALID_SOCKET;
        }
      }
      // Open a new connection if we're using TCP
      else
      {
        if (connect(socSocket, pAddressInfo->ai_addr, pAddressInfo->ai_addrlen) == 0)
        {
          break;
        }
        else
        {
          CloseSocket(socSocket);
          socSocket = INVALID_SOCKET;
        }
      }
    }
  }

  // Free variables
  freeaddrinfo(pAddressInfo);

  return socSocket;
}

// Function used to broadcast data to the local subnet.  Returns the number of bytes
// actually sent or -1 if an error occured.
int CSlingbox::Broadcast(SOCKET socSocket, unsigned int uiPort, void * pBuffer,
  unsigned int uiSize, unsigned int uiTimeout /* = 10 */)
{
  // Enable broadcasting
#if defined _WIN32 || defined _WIN64
  char vBroadcast = '1';
#else
  int vBroadcast = 1;
#endif
  if (setsockopt(socSocket, SOL_SOCKET, SO_BROADCAST, &vBroadcast,
    sizeof(vBroadcast)) != 0)
    return SOCKET_ERROR;

  // Prepare variable
  struct sockaddr sSocketAddress;
  memset(&sSocketAddress, 0, sizeof(sSocketAddress));
  ((sockaddr_in *)&sSocketAddress)->sin_family = AF_INET;
#if defined _WIN32 || defined _WIN64
  ((sockaddr_in *)&sSocketAddress)->sin_addr.S_un.S_addr = htonl(0xFFFFFFFF);
#else
  ((sockaddr_in *)&sSocketAddress)->sin_addr.s_addr = htonl(0xFFFFFFFF);
#endif
  ((sockaddr_in *)&sSocketAddress)->sin_port = htons(uiPort);

  // Send the data
  return SendTo(socSocket, pBuffer, uiSize, uiTimeout, &sSocketAddress);
}

// Function used to send data.  Returns the number of bytes actually sent
// or -1 if an error occured.
int CSlingbox::Send(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
  unsigned int uiTimeout /* = 10 */)
{
  // Prepare variables
  int iFDS = socSocket + 1;
  fd_set sendFDS;
  FD_ZERO(&sendFDS);
  FD_SET(socSocket, &sendFDS);
  timeval timeOut;
  timeOut.tv_sec = uiTimeout;
  timeOut.tv_usec = 0;
  int iResult;
  unsigned int uiBytesSent = 0;
  unsigned int uiBytesLeft = uiSize;

  // Loop until all data is sent
  while (uiBytesSent < uiSize)
  {
    // Send the data once the socket is ready
    if (select(iFDS, NULL, &sendFDS, NULL, &timeOut) > 0)
      iResult = send(socSocket, (const char *)pBuffer + uiBytesSent, uiBytesLeft, 0);
    else
      return uiBytesSent;

    // Check for any errors
    if (iResult == SOCKET_ERROR)
      return SOCKET_ERROR;

    // Adjust variables
    uiBytesSent += iResult;
    uiBytesLeft -= iResult;
  }

  return uiBytesSent;
}

// Function used to send data to a specific address.  Returns the number of bytes
// actually sent or -1 if an error occured.
int CSlingbox::SendTo(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
  unsigned int uiTimeout, struct sockaddr * pSocketAddress)
{
  // Prepare variables
  int iFDS = socSocket + 1;
  fd_set sendFDS;
  FD_ZERO(&sendFDS);
  FD_SET(socSocket, &sendFDS);
  timeval timeOut;
  timeOut.tv_sec = uiTimeout;
  timeOut.tv_usec = 0;
  int iResult;
  unsigned int uiBytesSent = 0;
  unsigned int uiBytesLeft = uiSize;

  // Loop until all data is sent
  while (uiBytesSent < uiSize)
  {
    // Send the data once the socket is ready
    if (select(iFDS, NULL, &sendFDS, NULL, &timeOut) > 0)
      iResult = sendto(socSocket, (const char *)pBuffer + uiBytesSent, uiBytesLeft, 0,
        pSocketAddress, sizeof(*pSocketAddress));
    else
      return uiBytesSent;

    // Check for any errors
    if (iResult == SOCKET_ERROR)
      return SOCKET_ERROR;

    // Adjust variables
    uiBytesSent += iResult;
    uiBytesLeft -= iResult;
  }

  return uiBytesSent;
}

// Function used to receive data.  Returns the number of bytes actually received
// or -1 if an error occured.
int CSlingbox::Receive(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
  unsigned int uiTimeout /* = 10 */)
{
  // Prepare needed variables
  int iFDS = socSocket + 1;
  fd_set readFDS;
  FD_ZERO(&readFDS);
  FD_SET(socSocket, &readFDS);
  timeval timeValue;
  timeValue.tv_sec = uiTimeout;
  timeValue.tv_usec = 0;

  // Do the actual receiving of data after waiting for socket to be readable
  if (select(iFDS, &readFDS, NULL, NULL, &timeValue) > 0)
    return recv(socSocket, (char *)pBuffer, uiSize, 0);
  else
    return 0;
}

// Function used to receive data and tell us from where.  Returns the number of bytes
// actually received or -1 if an error occured.  If successful, populates the
// pSocketAddress variable with details of where the data was received from.
int CSlingbox::ReceiveFrom(SOCKET socSocket, void * pBuffer, unsigned int uiSize,
  unsigned int uiTimeout, struct sockaddr * pSocketAddress)
{
  // Prepare needed variables
  int iFDS = socSocket + 1;
  fd_set readFDS;
  FD_ZERO(&readFDS);
  FD_SET(socSocket, &readFDS);
  timeval timeValue;
  timeValue.tv_sec = uiTimeout;
  timeValue.tv_usec = 0;
  socklen_t iSizeOfAddress = sizeof(*pSocketAddress);

  // Do the actual receiving of data after waiting for socket to be readable
  if (select(iFDS, &readFDS, NULL, NULL, &timeValue) > 0)
    return recvfrom(socSocket, (char *)pBuffer, uiSize, 0, pSocketAddress,
      &iSizeOfAddress);
  else
    return 0;
}

// Function used to close a socket connection.
bool CSlingbox::CloseSocket(SOCKET socSocket)
{
#if defined _WIN32 || defined _WIN64
  if (closesocket(socSocket) != 0)
    return false;
#else
  if (close(socSocket) != 0)
    return false;
#endif

  return true;
}

// Function used to wait on the Slingbox for various reasons
void CSlingbox::Wait(unsigned int uiMilliseconds)
{
#if defined _WIN32 || defined _WIN64
  Sleep(uiMilliseconds);
#else
  struct timespec time;
  time.tv_sec = uiMilliseconds / 1000;
  time.tv_nsec = (uiMilliseconds % 1000) * 1000000;
  while (nanosleep(&time, &time) == -1 && errno == EINTR &&
    (time.tv_sec > 0 || time.tv_nsec > 0));
#endif
}
