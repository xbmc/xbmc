/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

enum WebSocketFrameOpcode
{
  WebSocketContinuationFrame  = 0x00,
  WebSocketTextFrame          = 0x01,
  WebSocketBinaryFrame        = 0x02,
  //0x3 - 0x7 are reserved for non-control frames
  WebSocketConnectionClose    = 0x08,
  WebSocketPing               = 0x09,
  WebSocketPong               = 0x0A,
  //0xB - 0xF are reserved for control frames
  WebSocketUnknownFrame       = 0x10
};

enum WebSocketState
{
  WebSocketStateNotConnected    = 0,
  WebSocketStateHandshaking     = 1,
  WebSocketStateConnected       = 2,
  WebSocketStateClosing         = 3,
  WebSocketStateClosed          = 4
};

enum WebSocketCloseReason
{
  WebSocketCloseNormal          = 1000,
  WebSocketCloseLeaving         = 1001,
  WebSocketCloseProtocolError   = 1002,
  WebSocketCloseInvalidData     = 1003,
  WebSocketCloseFrameTooLarge   = 1004,
  // Reserved status code       = 1005,
  // Reserved status code       = 1006,
  WebSocketCloseInvalidUtf8     = 1007
};

class CWebSocketFrame
{
public:
  CWebSocketFrame(const char* data, uint64_t length);
  CWebSocketFrame(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0, bool final = true, bool masked = false, int32_t mask = 0, int8_t extension = 0);
  virtual ~CWebSocketFrame();

  virtual bool IsValid() const { return m_valid; }
  virtual uint64_t GetFrameLength() const { return m_lengthFrame; }
  virtual bool IsFinal() const { return m_final; }
  virtual int8_t GetExtension() const { return m_extension; }
  virtual WebSocketFrameOpcode GetOpcode() const { return m_opcode; }
  virtual bool IsControlFrame() const { return (m_valid && (m_opcode & 0x8) == 0x8); }
  virtual bool IsMasked() const { return m_masked; }
  virtual uint64_t GetLength() const { return m_length; }
  virtual int32_t GetMask() const { return m_mask; }
  virtual const char* GetFrameData() const { return m_data; }
  virtual const char* GetApplicationData() const { return m_applicationData; }

protected:
  bool m_free;
  const char *m_data;
  uint64_t m_lengthFrame;
  uint64_t m_length;
  bool m_valid;
  bool m_final;
  int8_t m_extension;
  WebSocketFrameOpcode m_opcode;
  bool m_masked;
  int32_t m_mask;
  char *m_applicationData;

private:
  void reset();
  CWebSocketFrame(const CWebSocketFrame&) = delete;
  CWebSocketFrame& operator=(const CWebSocketFrame&) = delete;
};

class CWebSocketMessage
{
public:
  CWebSocketMessage();
  virtual ~CWebSocketMessage();

  virtual bool IsFragmented() const { return m_fragmented; }
  virtual bool IsComplete() const { return m_complete; }

  virtual bool AddFrame(const CWebSocketFrame* frame);
  virtual const std::vector<const CWebSocketFrame *>& GetFrames() const { return m_frames; }

  virtual void Clear();

protected:
  std::vector<const CWebSocketFrame *> m_frames;
  bool m_fragmented;
  bool m_complete;
};

class CWebSocket
{
public:
  CWebSocket() { m_state = WebSocketStateNotConnected; m_message = NULL; }
  virtual ~CWebSocket()
  {
    if (m_message)
      delete m_message;
  }

  int GetVersion() { return m_version; }
  WebSocketState GetState() { return m_state; }

  virtual bool Handshake(const char* data, size_t length, std::string &response) = 0;
  virtual const CWebSocketMessage* Handle(const char* &buffer, size_t &length, bool &send);
  virtual const CWebSocketMessage* Send(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0);
  virtual const CWebSocketFrame* Ping(const char* data = NULL) const = 0;
  virtual const CWebSocketFrame* Pong(const char* data, uint32_t length) const = 0;
  virtual const CWebSocketFrame* Close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "") = 0;
  virtual void Fail() = 0;

protected:
  int m_version;
  WebSocketState m_state;
  CWebSocketMessage *m_message;

  virtual CWebSocketFrame* GetFrame(const char* data, uint64_t length) = 0;
  virtual CWebSocketFrame* GetFrame(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0, bool final = true, bool masked = false, int32_t mask = 0, int8_t extension = 0) = 0;
  virtual CWebSocketMessage* GetMessage() = 0;
};
