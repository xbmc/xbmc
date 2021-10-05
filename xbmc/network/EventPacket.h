/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <stdlib.h>
#include <vector>

namespace EVENTPACKET
{
  const int PACKET_SIZE       = 1024;
  const int HEADER_SIZE       = 32;
  const char HEADER_SIG[]     = "XBMC";
  const int HEADER_SIG_LENGTH = 4;

  /************************************************************************/
  /*                                                                      */
  /* - Generic packet structure (maximum 1024 bytes per packet)           */
  /* - Header is 32 bytes long, so 992 bytes available for payload        */
  /* - large payloads can be split into multiple packets using H4 and H5  */
  /*   H5 should contain total no. of packets in such a case              */
  /* - H6 contains length of P1, which is limited to 992 bytes            */
  /* - if H5 is 0 or 1, then H4 will be ignored (single packet msg)       */
  /* - H7 must be set to zeros for now                                    */
  /*                                                                      */
  /*     -----------------------------                                    */
  /*     | -H1 Signature ("XBMC")    | - 4  x CHAR                4B      */
  /*     | -H2 Version (eg. 2.0)     | - 2  x UNSIGNED CHAR       2B      */
  /*     | -H3 PacketType            | - 1  x UNSIGNED SHORT      2B      */
  /*     | -H4 Sequence number       | - 1  x UNSIGNED LONG       4B      */
  /*     | -H5 No. of packets in msg | - 1  x UNSIGNED LONG       4B      */
  /*     | -H6 Payload size          | - 1  x UNSIGNED SHORT      2B      */
  /*     | -H7 Client's unique token | - 1  x UNSIGNED LONG       4B      */
  /*     | -H8 Reserved              | - 10 x UNSIGNED CHAR      10B      */
  /*     |---------------------------|                                    */
  /*     | -P1 payload               | -                                  */
  /*     -----------------------------                                    */
  /************************************************************************/

  /************************************************************************
     The payload format for each packet type is described below each
     packet type.

     Legend:
            %s - null terminated ASCII string (strlen + '\0' bytes)
                 (empty string is represented as a single byte NULL '\0')
            %c - single byte
            %i - network byte ordered short unsigned integer (2 bytes)
            %d - network byte ordered long unsigned integer  (4 bytes)
            XX - binary data prefixed with %d size
                 (can span multiple packets with <raw>)
           raw - raw binary data
   ************************************************************************/

  enum LogoType
  {
    LT_NONE = 0x00,
    LT_JPEG = 0x01,
    LT_PNG  = 0x02,
    LT_GIF  = 0x03
  };

  enum ButtonFlags
  {
    PTB_USE_NAME   = 0x01,
    PTB_DOWN       = 0x02,
    PTB_UP         = 0x04,
    PTB_USE_AMOUNT = 0x08,
    PTB_QUEUE      = 0x10,
    PTB_NO_REPEAT  = 0x20,
    PTB_VKEY       = 0x40,
    PTB_AXIS       = 0x80,
    PTB_AXISSINGLE = 0x100,
    PTB_UNICODE    = 0x200
  };

  enum MouseFlags
  {
    PTM_ABSOLUTE = 0x01
    /* PTM_RELATIVE = 0x02 */
  };

  enum ActionType
  {
    AT_EXEC_BUILTIN = 0x01,
    AT_BUTTON       = 0x02
  };

  enum PacketType
  {
    PT_HELO          = 0x01,
    /************************************************************************/
    /* Payload format                                                       */
    /* %s -  device name (max 128 chars)                                    */
    /* %c -  icontype ( 0=>NOICON, 1=>JPEG , 2=>PNG , 3=>GIF )              */
    /* %s -  my port ( 0=>not listening )                                   */
    /* %d -  reserved1 ( 0 )                                                */
    /* %d -  reserved2 ( 0 )                                                */
    /* XX -  imagedata ( can span multiple packets )                        */
    /************************************************************************/

    PT_BYE           = 0x02,
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/

    PT_BUTTON        = 0x03,
    /************************************************************************/
    /* Payload format                                                       */
    /* %i - button code                                                     */
    /* %i - flags 0x01 => use button map/name instead of code               */
    /*            0x02 => btn down                                          */
    /*            0x04 => btn up                                            */
    /*            0x08 => use amount                                        */
    /*            0x10 => queue event                                       */
    /*            0x20 => do not repeat                                     */
    /*            0x40 => virtual key                                       */
    /*            0x80 => axis key                                          */
    /* %i - amount ( 0 => 65k maps to -1 => 1 )                             */
    /* %s - device map (case sensitive and required if flags & 0x01)        */
    /*      "KB" - Standard keyboard map                                    */
    /*      "XG" - Xbox Gamepad                                             */
    /*      "R1" - Xbox Remote                                              */
    /*      "R2" - Xbox Universal Remote                                    */
    /*      "LI:devicename" -  valid LIRC device map where 'devicename'     */
    /*                         is the actual name of the LIRC device        */
    /*      "JS<num>:joyname" -  valid Joystick device map where            */
    /*                           'joyname'  is the name specified in        */
    /*                           the keymap. JS only supports button code   */
    /*                           and not button name currently (!0x01).     */
    /* %s - button name (required if flags & 0x01)                          */
    /************************************************************************/

    PT_MOUSE         = 0x04,
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - flags                                                           */
    /*    - 0x01 absolute position                                          */
    /* %i - mousex (0-65535 => maps to screen width)                        */
    /* %i - mousey (0-65535 => maps to screen height)                       */
    /************************************************************************/

    PT_PING          = 0x05,
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/

    PT_BROADCAST     = 0x06,
    /************************************************************************/
    /* @todo implement                                                      */
    /* Payload format: TODO                                                 */
    /************************************************************************/

    PT_NOTIFICATION  = 0x07,
    /************************************************************************/
    /* Payload format:                                                      */
    /* %s - caption                                                         */
    /* %s - message                                                         */
    /* %c - icontype ( 0=>NOICON, 1=>JPEG , 2=>PNG , 3=>GIF )               */
    /* %d - reserved ( 0 )                                                  */
    /* XX - imagedata ( can span multiple packets )                         */
    /************************************************************************/

    PT_BLOB          = 0x08,
    /************************************************************************/
    /* Payload format                                                       */
    /* raw - raw binary data                                                */
    /************************************************************************/

    PT_LOG           = 0x09,
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - log type                                                        */
    /* %s - message                                                         */
    /************************************************************************/
    PT_ACTION        = 0x0A,
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - action type                                                     */
    /* %s - action message                                                  */
    /************************************************************************/
    PT_DEBUG         = 0xFF,
    /************************************************************************/
    /* Payload format:                                                      */
    /************************************************************************/

    PT_LAST // NO-OP
  };

  class CEventPacket
  {
  public:
    CEventPacket() = default;

    explicit CEventPacket(int datasize, const void* data) { Parse(datasize, data); }

    virtual ~CEventPacket() = default;
    virtual bool Parse(int datasize, const void *data);
    bool         IsValid() const { return m_bValid; }
    PacketType   Type() const { return m_eType; }
    unsigned int Size() const { return m_iTotalPackets; }
    unsigned int Sequence() const { return m_iSeq; }
    const uint8_t* Payload() const { return m_pPayload.data(); }
    unsigned int PayloadSize() const { return m_pPayload.size(); }
    unsigned int ClientToken() const { return m_iClientToken; }
    void SetPayload(std::vector<uint8_t> payload);

  protected:
    bool m_bValid{false};
    unsigned int m_iSeq{0};
    unsigned int m_iTotalPackets{0};
    unsigned char  m_header[32];
    std::vector<uint8_t> m_pPayload;
    unsigned int m_iClientToken{0};
    unsigned char m_cMajVer{'0'};
    unsigned char m_cMinVer{'0'};
    PacketType m_eType{PT_LAST};
  };

}

