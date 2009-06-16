/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * DESCRIPTION:
 *
 * DVDPlayer input stream interface to PVRManager to play live tv and
 * recorded streams from pvr/tv server backend on the local machine
 * or from the network inside XBMC.
 *
 * Channel zapping and switching is also supported by numeric keys
 * and the channel up/down button on the remote.
 *
 * Streams are directly readed from PVRManager class over the control
 * class of the selected type of backend.
 *
 * Input file string must be in the following format.
 * For live tv:
 * "tv://1" or "pvr://1" where the number is the channel number to open.
 * For recordings:
 * "record://1" where the number is recording number stored by the
 * backend.
 *
 */

#include "stdafx.h"
#include "DVDInputStreamPVRManager.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "PVRManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogFullScreenInfo.h"
#include "Application.h"

/************************************************************************
 * Description: Class constructor, initialize member variables
 *              public class is CDVDInputStream
 */
CDVDInputStreamPVRManager::CDVDInputStreamPVRManager() : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER)
{
  m_eof               = true;
  m_isPlayRecording   = false;
  m_playingItem       = NULL;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();
}

/************************************************************************
 * Description: Get the end of file flag
 * \return bool                     = true means end of file
 */
bool CDVDInputStreamPVRManager::IsEOF()
{
  return m_eof;
}

/************************************************************************
 * Description: Open a stream over PVRManager from the backend
 * \param const char* strFile       = URL of the stream, see note
 * \param const std::string& content=
 * \return bool                     = false if open fails
 *
 * Note:
 * Input file string must be in the following format.
 * For live tv:
 * "tv://1" where the number is the channel number to open.
 * For recordings:
 * "record://1" where the number is recording number stored by the
 * backend.
 */
bool CDVDInputStreamPVRManager::Open(const char* strFile, const std::string& content)
{
  bool isRadio = false;

  if (!CDVDInputStream::Open(strFile, content)) return false;

  /* Find out which type of pvr data must be played
     and get its id for recordings or channelnumber
     for live tv or radio */
  std::string filename = strFile;

  if (filename.substr(0, 5) == "tv://")              /* Live TV */
  {
    filename.erase(0, 5);
    m_playingItem = atoi(filename.c_str());
    m_isPlayRecording = false;
  }
  else if (filename.substr(0, 8) == "radio://")     /* Live Radio */
  {
    filename.erase(0, 8);
    m_playingItem = atoi(filename.c_str());
    m_isPlayRecording = false;
    isRadio = true;
  }
  else if (filename.substr(0, 9) == "record://")     /* Recorded TV */
  {
    filename.erase(0, 9);
    m_playingItem = atoi(filename.c_str());
    m_isPlayRecording = true;
  }

  /* Open the stream from PVRManager to DVDPlayer */
  if (!m_isPlayRecording)
  {
    if (!CPVRManager::GetInstance()->OpenLiveStream(m_playingItem, isRadio))
    {
      return false;
    }
  }
  else
  {
    if (!CPVRManager::GetInstance()->OpenRecordedStream(m_playingItem))
    {
      return false;
    }
  }

  m_eof = false;

  return true;
}

/************************************************************************
 * Description: Close opened stream and reset everyting
 */
void CDVDInputStreamPVRManager::Close()
{
  /* Close the stream from PVRManager to DVDPlayer */
  if (!m_isPlayRecording)
  {
    CPVRManager::GetInstance()->CloseLiveStream();
  }
  else
  {
    CPVRManager::GetInstance()->CloseRecordedStream();
  }

  CDVDInputStream::Close();

  m_eof = true;
}

/************************************************************************
 * Description: Read stream from PVRManager into buffer
 * \param BYTE* buf                 = pointer to buffer
 * \param int buf_size              = size of buffer
 * \return int                      = Bytes readen (-1 for fail)
 *
 * Note:
 * The end of file flag is set if a failure occours and result by
 * stopping stream
 */
int CDVDInputStreamPVRManager::Read(BYTE* buf, int buf_size)
{
  unsigned int ret;

  /* Read the stream from PVRManager to DVDPlayer */

  if (!m_isPlayRecording)
  {
    ret = CPVRManager::GetInstance()->ReadLiveStream(buf, buf_size);
  }
  else
  {
    ret = CPVRManager::GetInstance()->ReadRecordedStream(buf, buf_size);
  }

  /* we currently don't support non completing reads */
  if (ret < 0) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamPVRManager::Seek(__int64 offset, int whence)
{
  if (m_isPlayRecording)
  {
    if (whence == SEEK_POSSIBLE)
    {
      __int64 ret = CPVRManager::GetInstance()->SeekRecordedStream(offset, whence);

      if (ret >= 0)
      {
        return ret;
      }
      else
      {
        if (CPVRManager::GetInstance()->LengthRecordedStream() && CPVRManager::GetInstance()->SeekRecordedStream(0, SEEK_CUR) >= 0)
          return 1;
        else
          return 0;
      }
    }
    else
    {
      return CPVRManager::GetInstance()->SeekRecordedStream(offset, whence);
    }
  }
  else
  {
    if (g_guiSettings.GetBool("pvrrecord.timeshift"))
    {
      return CPVRManager::GetInstance()->SeekLiveStream(offset, whence);
    }

    return 0;
  }
}

__int64 CDVDInputStreamPVRManager::GetLength()
{
  if (m_isPlayRecording)
  {
    return CPVRManager::GetInstance()->LengthRecordedStream();
  }

  return 0;
}

/************************************************************************
 * Description: Get the total time of the current played stream
 * \return int                  = Total time of the stream in milliseconds
 * Note:
 * Total time = unix total time * 1000
 */
int CDVDInputStreamPVRManager::GetTotalTime()
{
  return CPVRManager::GetInstance()->GetTotalTime();
}

/************************************************************************
 * Description: Get the start time of the current played stream
 * \return int                  = Start time of the stream in milliseconds
 *
 * Note:
 * Start time = (unix start time - unix current time) * 1000
 * Must be a negative value
 */
int CDVDInputStreamPVRManager::GetStartTime()
{
  return CPVRManager::GetInstance()->GetStartTime();
}

/************************************************************************
 * Description: Do a channel switch by enter numbers by numeric
 *              keys on the remote or keyboard.
 * \param int iChannel          = integer value of first channelnumber
 *                                (next numbers are entered by
 *                                CGUIDialogNumeric)
 * \return bool                 = true if ok, false if error
 *
 * Note:
 * Not all type of errors give "false" on return, a stream is normally
 * always played and if the switch fails in case of a invalid channel
 * the current tuned channel continue to play.
 *
 * Errors like this opens a DialogWindow to inform the user for the fault.
 */
bool CDVDInputStreamPVRManager::Channel(int iChannel)
{
  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    /** TODO:
     ** Add support for cutting keys (functions becomes the numeric keys as integer)
     **/
    return true;
  }

  if (CPVRManager::GetInstance()->ChannelSwitch(iChannel))
  {
    m_playingItem = iChannel;
    return true;
  }
  else
  {
    return false;
  }
}

/************************************************************************
 * Description: Do a channel switch to next available channel in list
 * \return bool                 = true if ok, false if error
 *
 * Note:
 * Not all type of errors give "false" on return, a stream is normally
 * always played and if the switch fails in case of a invalid channel
 * the current tuned channel continue to play.
 *
 * Errors like this opens a DialogWindow to inform the user for the fault.
 */
bool CDVDInputStreamPVRManager::NextChannel()
{
  unsigned int newchannel;

  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    return true;
  }

  /* Do channel switch and save new channel number, it is not always
   * increased by one in a case if next channel is encrypted or we
   * on the beginning or end of the channel list!
   */
  if (CPVRManager::GetInstance()->ChannelUp(&newchannel))
  {
    m_playingItem = newchannel;
    return true;
  }
  else
  {
    return false;
  }
}

/************************************************************************
 * Description: Do a channel switch to previous available channel in list
 * \return bool                 = true if ok, false if error
 *
 * Note:
 * Not all type of errors give "false" on return, a stream is normally
 * always played and if the switch fails in case of a invalid channel
 * the current tuned channel continue to play.
 *
 * Errors like this opens a DialogWindow to inform the user for the fault.
 */
bool CDVDInputStreamPVRManager::PrevChannel()
{
  unsigned int newchannel;

  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    return true;
  }

  /* Do channel switch and save new channel number, it is not always
   * increased by one in a case if next channel is encrypted or we
   * on the beginning or end of the channel list!
   */
  if (CPVRManager::GetInstance()->ChannelDown(&newchannel))
  {
    m_playingItem = newchannel;
    return true;
  }
  else
  {
    return false;
  }
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  return CPVRManager::GetInstance()->UpdateItem(item);
}

bool CDVDInputStreamPVRManager::SeekTime(int iTimeInMsec)
{
  return false;
}

bool CDVDInputStreamPVRManager::NextStream()
{
  return false;
}

bool CDVDInputStreamPVRManager::CanRecord()
{
  if (m_isPlayRecording)
  {
    return false;
  }

  return false;//CPVRManager::GetInstance()->SupportRecording();
}

bool CDVDInputStreamPVRManager::IsRecording()
{
  return CPVRManager::GetInstance()->IsRecording(m_playingItem);
}

bool CDVDInputStreamPVRManager::Record(bool bOnOff)
{
  return CPVRManager::GetInstance()->RecordChannel(m_playingItem, bOnOff);
}

void CDVDInputStreamPVRManager::Pause(bool OnOff)
{
  CPVRManager::GetInstance()->PauseLiveStream(OnOff);
  return;
}
