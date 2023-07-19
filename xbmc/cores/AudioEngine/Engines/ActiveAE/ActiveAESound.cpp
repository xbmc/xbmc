/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActiveAESound.h"

#include "ActiveAE.h"
#include "cores/AudioEngine/Interfaces/AESound.h"
#include "filesystem/File.h"
#include "utils/log.h"

extern "C" {
#include <libavutil/avutil.h>
}

using namespace ActiveAE;
using namespace XFILE;

CActiveAESound::CActiveAESound(const std::string& filename, CActiveAE* ae)
  : IAESound(filename), m_filename(filename)
{
  m_orig_sound = NULL;
  m_dst_sound = NULL;
  m_pFile = NULL;
  m_isSeekPossible = false;
  m_fileSize = 0;
  m_isConverted = false;
  m_activeAE = ae;
}

CActiveAESound::~CActiveAESound()
{
  delete m_orig_sound;
  delete m_dst_sound;
  Finish();
}

void CActiveAESound::Play()
{
  m_activeAE->PlaySound(this);

}

void CActiveAESound::Stop()
{
  m_activeAE->StopSound(this);
}

bool CActiveAESound::IsPlaying()
{
  //! @todo implement
  return false;
}

uint8_t** CActiveAESound::InitSound(bool orig, SampleConfig config, int nb_samples)
{
  CSoundPacket **info;
  if (orig)
    info = &m_orig_sound;
  else
    info = &m_dst_sound;

  delete *info;
  *info = new CSoundPacket(config, nb_samples);

  (*info)->nb_samples = 0;
  m_isConverted = false;
  return (*info)->data;
}

bool CActiveAESound::StoreSound(bool orig, uint8_t **buffer, int samples, int linesize)
{
  CSoundPacket **info;
  if (orig)
    info = &m_orig_sound;
  else
    info = &m_dst_sound;

  if ((*info)->nb_samples + samples > (*info)->max_nb_samples)
  {
    CLog::Log(LOGERROR, "CActiveAESound::StoreSound - exceeded max samples");
    return false;
  }

  int bytes_to_copy = samples * (*info)->bytes_per_sample * (*info)->config.channels;
  bytes_to_copy /= (*info)->planes;
  int start = (*info)->nb_samples * (*info)->bytes_per_sample * (*info)->config.channels;
  start /= (*info)->planes;

  for (int i=0; i<(*info)->planes; i++)
  {
    memcpy((*info)->data[i]+start, buffer[i], bytes_to_copy);
  }
  (*info)->nb_samples += samples;

  return true;
}

CSoundPacket *CActiveAESound::GetSound(bool orig)
{
  if (orig)
    return m_orig_sound;
  else
    return m_dst_sound;
}

bool CActiveAESound::Prepare()
{
  unsigned int flags = READ_TRUNCATED | READ_CHUNKED;
  m_pFile = new CFile();

  if (!m_pFile->Open(m_filename, flags))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_isSeekPossible = m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL) != 0;
  m_fileSize = m_pFile->GetLength();
  return true;
}

void CActiveAESound::Finish()
{
  delete m_pFile;
  m_pFile = NULL;
}

int CActiveAESound::GetChunkSize()
{
  return m_pFile->GetChunkSize();
}

int CActiveAESound::Read(void *h, uint8_t* buf, int size)
{
  CFile *pFile = static_cast<CActiveAESound*>(h)->m_pFile;
  int len = pFile->Read(buf, size);
  if (len == 0)
    return AVERROR_EOF;
  else
    return len;
}

int64_t CActiveAESound::Seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CActiveAESound*>(h)->m_pFile;
  if(whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}
