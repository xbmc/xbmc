/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebSocket.h"

#include "utils/EndianSwap.h"
#include "utils/HttpParser.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdint>
#include <sstream>
#include <string>

#define MASK_FIN      0x80
#define MASK_RSV1     0x40
#define MASK_RSV2     0x20
#define MASK_RSV3     0x10
#define MASK_RSV      (MASK_RSV1 | MASK_RSV2 | MASK_RSV3)
#define MASK_OPCODE   0x0F
#define MASK_MASK     0x80
#define MASK_LENGTH   0x7F

#define CONTROL_FRAME 0x08

#define LENGTH_MIN    0x2

CWebSocketFrame::CWebSocketFrame(const char* data, uint64_t length)
{
  reset();

  if (data == NULL || length < LENGTH_MIN)
    return;

  m_free = false;
  m_data = data;
  m_lengthFrame = length;

  // Get the FIN flag
  m_final = ((m_data[0] & MASK_FIN) == MASK_FIN);
  // Get the RSV1 - RSV3 flags
  m_extension |= m_data[0] & MASK_RSV1;
  m_extension |= (m_data[0] & MASK_RSV2) << 1;
  m_extension |= (m_data[0] & MASK_RSV3) << 2;
  // Get the opcode
  m_opcode = (WebSocketFrameOpcode)(m_data[0] & MASK_OPCODE);
  if (m_opcode >= WebSocketUnknownFrame)
  {
    CLog::Log(LOGINFO, "WebSocket: Frame with invalid opcode {:2X} received", m_opcode);
    reset();
    return;
  }
  if ((m_opcode & CONTROL_FRAME) == CONTROL_FRAME && !m_final)
  {
    CLog::Log(LOGINFO, "WebSocket: Fragmented control frame (opcode {:2X}) received", m_opcode);
    reset();
    return;
  }

  // Get the MASK flag
  m_masked = ((m_data[1] & MASK_MASK) == MASK_MASK);

  // Get the payload length
  m_length = (uint64_t)(m_data[1] & MASK_LENGTH);
  if ((m_length <= 125 && m_lengthFrame  < m_length + LENGTH_MIN) ||
      (m_length == 126 && m_lengthFrame < LENGTH_MIN + 2) ||
      (m_length == 127 && m_lengthFrame < LENGTH_MIN + 8))
  {
    CLog::Log(LOGINFO, "WebSocket: Frame with invalid length received");
    reset();
    return;
  }

  if (IsControlFrame() && (m_length > 125 || !m_final))
  {
    CLog::Log(LOGWARNING, "WebSocket: Invalid control frame received");
    reset();
    return;
  }

  int offset = 0;
  if (m_length == 126)
  {
    uint16_t length;
    std::memcpy(&length, m_data + 2, 2);
    m_length = Endian_SwapBE16(length);
    offset = 2;
  }
  else if (m_length == 127)
  {
    std::memcpy(&m_length, m_data + 2, 8);
    m_length = Endian_SwapBE64(m_length);
    offset = 8;
  }

  if (m_lengthFrame < LENGTH_MIN + offset + m_length)
  {
    CLog::Log(LOGINFO, "WebSocket: Frame with invalid length received");
    reset();
    return;
  }

  // Get the mask
  if (m_masked)
  {
    std::memcpy(&m_mask, m_data + LENGTH_MIN + offset, 4);
    offset += 4;
  }

  if (m_lengthFrame != LENGTH_MIN + offset + m_length)
    m_lengthFrame = LENGTH_MIN + offset + m_length;

  // Get application data
  if (m_length > 0)
    m_applicationData = const_cast<char *>(m_data + LENGTH_MIN + offset);
  else
    m_applicationData = NULL;

  // Unmask the application data if necessary
  if (m_masked)
  {
    for (uint64_t index = 0; index < m_length; index++)
      m_applicationData[index] = m_applicationData[index] ^ ((char *)(&m_mask))[index % 4];
  }

  m_valid = true;
}

CWebSocketFrame::CWebSocketFrame(WebSocketFrameOpcode opcode, const char* data /* = NULL */, uint32_t length /* = 0 */,
                                 bool final /* = true */, bool masked /* = false */, int32_t mask /* = 0 */, int8_t extension /* = 0 */)
{
  reset();

  if (opcode >= WebSocketUnknownFrame)
    return;

  m_free = true;
  m_opcode = opcode;

  m_length = length;

  m_masked = masked;
  m_mask = mask;
  m_final = final;
  m_extension = extension;

  std::string buffer;
  char dataByte = 0;

  // Set the FIN flag
  if (m_final)
    dataByte |= MASK_FIN;

  // Set RSV1 - RSV3 flags
  if (m_extension != 0)
    dataByte |= (m_extension << 4) & MASK_RSV;

  // Set opcode flag
  dataByte |= opcode & MASK_OPCODE;

  buffer.push_back(dataByte);
  dataByte = 0;

  // Set MASK flag
  if (m_masked)
    dataByte |= MASK_MASK;

  // Set payload length
  if (m_length < 126)
  {
    dataByte |= m_length & MASK_LENGTH;
    buffer.push_back(dataByte);
  }
  else if (m_length <= 65535)
  {
    dataByte |= 126 & MASK_LENGTH;
    buffer.push_back(dataByte);

    uint16_t dataLength = Endian_SwapBE16((uint16_t)m_length);
    buffer.append((const char*)&dataLength, 2);
  }
  else
  {
    dataByte |= 127 & MASK_LENGTH;
    buffer.push_back(dataByte);

    uint64_t dataLength = Endian_SwapBE64(m_length);
    buffer.append((const char*)&dataLength, 8);
  }

  uint64_t applicationDataOffset = 0;
  if (data)
  {
    // Set masking key
    if (m_masked)
    {
      buffer.append((char *)&m_mask, sizeof(m_mask));
      applicationDataOffset = buffer.size();

      for (uint64_t index = 0; index < m_length; index++)
        buffer.push_back(data[index] ^ ((char *)(&m_mask))[index % 4]);
    }
    else
    {
      applicationDataOffset = buffer.size();
      buffer.append(data, (unsigned int)length);
    }
  }

  // Get the whole data
  m_lengthFrame = buffer.size();
  m_data = new char[(uint32_t)m_lengthFrame];
  memcpy(const_cast<char *>(m_data), buffer.c_str(), (uint32_t)m_lengthFrame);

  if (data)
  {
    m_applicationData = const_cast<char *>(m_data);
    m_applicationData += applicationDataOffset;
  }

  m_valid = true;
}

CWebSocketFrame::~CWebSocketFrame()
{
  if (!m_valid)
    return;

  if (m_free && m_data != NULL)
  {
    delete[] m_data;
    m_data = NULL;
  }
}

void CWebSocketFrame::reset()
{
  m_free = false;
  m_data = NULL;
  m_lengthFrame = 0;
  m_length = 0;
  m_valid = false;
  m_final = false;
  m_extension = 0;
  m_opcode = WebSocketUnknownFrame;
  m_masked = false;
  m_mask = 0;
  m_applicationData = NULL;
}

CWebSocketMessage::CWebSocketMessage()
{
  Clear();
}

CWebSocketMessage::~CWebSocketMessage()
{
  for (unsigned int index = 0; index < m_frames.size(); index++)
    delete m_frames[index];

  m_frames.clear();
}

bool CWebSocketMessage::AddFrame(const CWebSocketFrame *frame)
{
  if (!frame->IsValid() || m_complete)
    return false;

  if (frame->IsFinal())
    m_complete = true;
  else
    m_fragmented = true;

  m_frames.push_back(frame);

  return true;
}

void CWebSocketMessage::Clear()
{
  m_fragmented = false;
  m_complete = false;

  m_frames.clear();
}

const CWebSocketMessage* CWebSocket::Handle(const char* &buffer, size_t &length, bool &send)
{
  send = false;

  while (length > 0)
  {
    switch (m_state)
    {
      case WebSocketStateConnected:
      {
        CWebSocketFrame *frame = GetFrame(buffer, length);
        if (!frame->IsValid())
        {
          CLog::Log(LOGINFO, "WebSocket: Invalid frame received");
          delete frame;
          return NULL;
        }

        // adjust the length and the buffer values
        length -= (size_t)frame->GetFrameLength();
        buffer += frame->GetFrameLength();

        if (frame->IsControlFrame())
        {
          if (!frame->IsFinal())
          {
            delete frame;
            return NULL;
          }

          CWebSocketMessage *msg = NULL;
          switch (frame->GetOpcode())
          {
            case WebSocketPing:
              msg = GetMessage();
              if (msg != NULL)
                msg->AddFrame(Pong(frame->GetApplicationData(), frame->GetLength()));
              break;

            case WebSocketConnectionClose:
              CLog::Log(LOGINFO, "WebSocket: connection closed by client");

              msg = GetMessage();
              if (msg != NULL)
                msg->AddFrame(Close());

              m_state = WebSocketStateClosed;
              break;

            case WebSocketContinuationFrame:
            case WebSocketTextFrame:
            case WebSocketBinaryFrame:
            case WebSocketPong:
            case WebSocketUnknownFrame:
            default:
              break;
          }

          delete frame;

          if (msg != NULL)
            send = true;

          return msg;
        }

        if (m_message == NULL && (m_message = GetMessage()) == NULL)
        {
          CLog::Log(LOGINFO, "WebSocket: Could not allocate a new websocket message");
          delete frame;
          return NULL;
        }

        m_message->AddFrame(frame);
        if (!m_message->IsComplete())
        {
          if (length > 0)
            continue;
          else
            return NULL;
        }

        CWebSocketMessage *msg = m_message;
        m_message = NULL;
        return msg;
      }

      case WebSocketStateClosing:
      {
        CWebSocketFrame *frame = GetFrame(buffer, length);

        if (frame->IsValid())
        {
          // adjust the length and the buffer values
          length -= (size_t)frame->GetFrameLength();
          buffer += frame->GetFrameLength();
        }

        if (!frame->IsValid() || frame->GetOpcode() == WebSocketConnectionClose)
        {
          CLog::Log(LOGINFO, "WebSocket: Invalid or unexpected frame received (only closing handshake expected)");
          delete frame;
          return NULL;
        }

        m_state = WebSocketStateClosed;
        return NULL;
      }

      case WebSocketStateNotConnected:
      case WebSocketStateClosed:
      case WebSocketStateHandshaking:
      default:
        CLog::Log(LOGINFO, "WebSocket: No frame expected in the current state");
        return NULL;
    }
  }

  return NULL;
}

const CWebSocketMessage* CWebSocket::Send(WebSocketFrameOpcode opcode, const char* data /* = NULL */, uint32_t length /* = 0 */)
{
  CWebSocketFrame *frame = GetFrame(opcode, data, length);
  if (frame == NULL || !frame->IsValid())
  {
    CLog::Log(LOGINFO, "WebSocket: Trying to send an invalid frame");
    return NULL;
  }

  CWebSocketMessage *msg = GetMessage();
  if (msg == NULL)
  {
    CLog::Log(LOGINFO, "WebSocket: Could not allocate a message");
    return NULL;
  }

  msg->AddFrame(frame);
  if (msg->IsComplete())
    return msg;

  return NULL;
}
