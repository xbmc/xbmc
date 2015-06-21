#pragma once
/*
 *      Copyright (C) 2005-2015 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Software; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <deque>

#include "IVideoPlayer.h"
#include "DVDMessageQueue.h"
#include "FileItem.h"
#include "threads/Thread.h"
#include "utils/Stopwatch.h"

class CDVDStreamInfo;

/// --- CDVDRadioRDSData ------------------------------------------------------------

#define UECP_DATA_START               0xFE    /*!< A data record starts with the start byte */
#define UECP_DATA_STOP                0xFF    /*!< A data record stops with the stop byte */
#define UECP_SIZE_MAX                 263     /*!< The Max possible size of a UECP packet
                                                   max. 255(MSG)+4(ADD/SQC/MFL)+2(CRC)+2(Start/Stop) of RDS-data */
#define RT_MEL                        65
#define MAX_RTPC                      50
#define MAX_RADIOTEXT_LISTSIZE        6

class CDVDRadioRDSData : public CThread, public IDVDStreamPlayer
{
public:
  CDVDRadioRDSData(CProcessInfo &processInfo);
  virtual ~CDVDRadioRDSData();

  bool CheckStream(CDVDStreamInfo &hints);
  bool OpenStream(CDVDStreamInfo &hints);
  void CloseStream(bool bWaitForBuffers);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers() { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData() const { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0) { if(m_messageQueue.IsInited()) m_messageQueue.Put(pMsg, priority); }
  void FlushMessages() { m_messageQueue.Flush(); }
  bool IsInited() const { return true; }
  bool IsStalled() const { return true; }

  std::string GetRadioText(unsigned int line);

protected:
  virtual void OnExit();
  virtual void Process();

private:
  void ResetRDSCache();
  void ProcessUECP(const unsigned char *Data, unsigned int Length);

  inline unsigned int DecodePI(uint8_t *msgElement);
  inline unsigned int DecodePS(uint8_t *msgElement);
  inline unsigned int DecodeDI(uint8_t *msgElement);
  inline unsigned int DecodeTA_TP(uint8_t *msgElement);
  inline unsigned int DecodeMS(uint8_t *msgElement);
  inline unsigned int DecodePTY(uint8_t *msgElement);
  inline unsigned int DecodePTYN(uint8_t *msgElement);
  inline unsigned int DecodeRT(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeRTC(uint8_t *msgElement);
  inline unsigned int DecodeODA(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeRTPlus(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeTMC(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeEPPTransmitterInfo(uint8_t *msgElement);
  inline unsigned int DecodeSlowLabelingCodes(uint8_t *msgElement);
  inline unsigned int DecodeDABDynLabelCmd(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeDABDynLabelMsg(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeAF(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeEonAF(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeTDC(uint8_t *msgElement, unsigned int len);

  void SendTMCSignal(unsigned int flags, uint8_t *data);
  void SetRadioStyle(std::string genre);

  PVR::CPVRRadioRDSInfoTagPtr m_currentInfoTag;
  PVR::CPVRChannelPtr         m_currentChannel;
  bool                        m_currentFileUpdate;
  int                         m_speed;
  CCriticalSection            m_critSection;
  CDVDMessageQueue            m_messageQueue;

  uint8_t                     m_UECPData[UECP_SIZE_MAX+1];
  unsigned int                m_UECPDataIndex;
  bool                        m_UECPDataStart;
  bool                        m_UECPDatabStuff;
  bool                        m_UECPDataDeadBreak;

  bool                        m_RDS_IsRBDS;
  bool                        m_RDS_SlowLabelingCodesPresent;

  uint16_t                    m_PI_Current;
  unsigned int                m_PI_CountryCode;
  unsigned int                m_PI_ProgramType;
  unsigned int                m_PI_ProgramReferenceNumber;

  unsigned int                m_EPP_TM_INFO_ExtendedCountryCode;

  #define PS_TEXT_ENTRIES     12
  bool                        m_PS_Present;
  int                         m_PS_Index;
  char                        m_PS_Text[PS_TEXT_ENTRIES][9];

  bool                        m_DI_IsStereo;
  bool                        m_DI_ArtificialHead;
  bool                        m_DI_Compressed;
  bool                        m_DI_DynamicPTY;

  bool                        m_TA_TP_TrafficAdvisory;
  float                       m_TA_TP_TrafficVolume;

  bool                        m_MS_SpeechActive;

  int                         m_PTY;
  char                        m_PTYN[9];
  bool                        m_PTYN_Present;

  bool                        m_RT_Present;
  std::deque<std::string>     m_RT;
  int                         m_RT_Index;
  int                         m_RT_MaxSize;
  bool                        m_RT_NewItem;
  char                        m_RT_Text[6][RT_MEL+1];

  bool                        m_RTPlus_Present;
  uint8_t                     m_RTPlus_WorkText[RT_MEL+1];
  bool                        m_RTPlus_TToggle;
  int                         m_RTPlus_iDiffs;
  CStopWatch                  m_RTPlus_iTime;
  bool                        m_RTPlus_GenrePresent;
  char                        m_RTPlus_Temptext[RT_MEL];
  bool                        m_RTPlus_Show;
  char                        m_RTPlus_Title[RT_MEL];
  char                        m_RTPlus_Artist[RT_MEL];
  int                         m_RTPlus_iToggle;
  unsigned int                m_RTPlus_ItemToggle;
  time_t                      m_RTPlus_Starttime;

  CDateTime                   m_RTC_DateTime;                 ///< From RDS transmitted date / time data

  uint8_t                     m_TMC_LastData[5];
};
