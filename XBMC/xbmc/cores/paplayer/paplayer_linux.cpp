#include "../../stdafx.h"
#include "paplayer.h"
#include "CodecFactory.h"
#include "../../utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "../../FileSystem/FileShoutcast.h"
#include "../../Application.h"
#ifdef HAS_KARAOKE
#include "../../CdgParser.h"
#endif

extern XFILE::CFileShoutcast* m_pShoutCastRipper;

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
}

PAPlayer::~PAPlayer()
{
}

void PAPlayer::OnExit()
{
}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  return true;
}

void PAPlayer::UpdateCrossFadingTime(const CFileItem& file)
{
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  return true;
}

bool PAPlayer::QueueNextFile(const CFileItem &file, bool checkCrossFading)
{
  return true;
}


bool PAPlayer::CloseFileInternal(bool bAudioDevice /*= true*/)
{
  return true;
}

void PAPlayer::FreeStream(int stream)
{
}

void PAPlayer::SetupDirectSound(int channels)
{
}

bool PAPlayer::CreateStream(int num, int channels, int samplerate, int bitspersample, CStdString codec)
{
  return true;
}

void PAPlayer::Pause()
{
}

void PAPlayer::SetVolume(long nVolume)
{
}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
}

void PAPlayer::Process()
{
}

void PAPlayer::ToFFRW(int iSpeed)
{
}

void PAPlayer::UpdateCacheLevel()
{
}

bool PAPlayer::ProcessPAP()
{
  return true;
}

void PAPlayer::ResetTime()
{
}

__int64 PAPlayer::GetTime()
{
  return 0;
}

__int64 PAPlayer::GetTotalTime64()
{
  return 0;
}

int PAPlayer::GetTotalTime()
{
  return (int)(GetTotalTime64()/1000);
}

int PAPlayer::GetCacheLevel() const
{
  return 0;
}

int PAPlayer::GetChannels()
{
	return 2;
}

int PAPlayer::GetBitsPerSample()
{
	return 16;
}

int PAPlayer::GetSampleRate()
{
	return 44000;
}

CStdString PAPlayer::GetCodecName()
{
  return "";
}

int PAPlayer::GetBitrate()
{
  return 0;
}

bool PAPlayer::CanSeek()
{
  return ((m_decoder[m_currentDecoder].TotalTime() > 0) && m_decoder[m_currentDecoder].CanSeek());
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
}

float PAPlayer::GetPercentage()
{
	return 0.0;
}

void PAPlayer::HandleSeeking()
{
}

void PAPlayer::FlushStreams()
{
}

bool PAPlayer::HandleFFwdRewd()
{
  return true;
}

void PAPlayer::SetStreamVolume(int stream, long nVolume)
{
}

bool PAPlayer::AddPacketsToStream(int stream, CAudioDecoder &dec)
{
	return true;
}

bool PAPlayer::FindFreePacket( int stream, DWORD* pdwPacket )
{
  return true;
}

void PAPlayer::RegisterAudioCallback(IAudioCallback *pCallback)
{
}

void PAPlayer::UnRegisterAudioCallback()
{
}

void PAPlayer::DoAudioWork()
{
}

void PAPlayer::StreamCallback( LPVOID pPacketContext )
{
}

void CALLBACK StaticStreamCallback( VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus )
{
}

bool PAPlayer::HandlesType(const CStdString &type)
{
  return false;
}

// Skip to next track/item inside the current media (if supported).
bool PAPlayer::SkipNext()
{
  return false;
}

bool PAPlayer::CanRecord()
{
	return false;
}

bool PAPlayer::IsRecording()
{
	return false;
}

bool PAPlayer::Record(bool bOnOff)
{
  return true;
}

void PAPlayer::WaitForStream()
{
}
