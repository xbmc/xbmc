#include "DemuxTimeline.h"

#include <algorithm>

#include "DVDClock.h"
#include "DVDDemuxUtils.h"
#include "DVDFactoryDemuxer.h"
#include "DVDDemuxFFmpeg.h"
#include "DVDInputStreams/DVDInputStreamFile.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "MatroskaParser.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

CDemuxTimeline::CDemuxTimeline() {}

CDemuxTimeline::~CDemuxTimeline() {}

bool CDemuxTimeline::SwitchToNextDemuxer()
{
  if (m_curChapter->index + 1 == m_chapters.size())
    return false;
  CLog::Log(LOGDEBUG, "TimelineDemuxer: Switch Demuxer");
  m_curChapter = &m_chapters[m_curChapter->index + 1];
  m_curChapter->demuxer->SeekTime(m_curChapter->startSrcTime, true);
  return true;
}

void CDemuxTimeline::Reset()
{
  for (auto &demuxer : m_demuxers)
    demuxer->Reset();
  m_curChapter = m_chapterMap.begin()->second;
  if (m_curChapter->startSrcTime != 0)
    m_curChapter->demuxer->SeekTime(m_curChapter->startSrcTime);
}

void CDemuxTimeline::Abort()
{
  m_curChapter->demuxer->Abort();
}

void CDemuxTimeline::Flush()
{
  m_curChapter->demuxer->Flush();
}

DemuxPacket* CDemuxTimeline::Read()
{
  DemuxPacket *packet = nullptr;
  double pts = std::numeric_limits<double>::infinity();
  double dispPts;

  packet = m_curChapter->demuxer->Read();
  if (packet)
    pts = (packet->dts != DVD_NOPTS_VALUE ? packet->dts : packet->pts);

  while (
    !packet ||
    pts + packet->duration < DVD_MSEC_TO_TIME(m_curChapter->startSrcTime) ||
    pts >= DVD_MSEC_TO_TIME(m_curChapter->stopSrcTime())
  )
  {
    if (!packet || pts >= DVD_MSEC_TO_TIME(m_curChapter->stopSrcTime()))
      if (!SwitchToNextDemuxer())
        return nullptr;
    packet = m_curChapter->demuxer->Read();
    if (packet)
      pts = (packet->dts != DVD_NOPTS_VALUE ? packet->dts : packet->pts);
  }

  dispPts = pts + DVD_MSEC_TO_TIME(m_curChapter->shiftTime());
  packet->dts = dispPts;
  packet->pts = dispPts;
  packet->duration = std::min(packet->duration, DVD_MSEC_TO_TIME(m_curChapter->stopSrcTime()) - pts);
  packet->dispTime = DVD_TIME_TO_MSEC(pts) + m_curChapter->shiftTime();

  return packet;
}

bool CDemuxTimeline::SeekTime(double time, bool backwords, double* startpts)
{
  auto it = m_chapterMap.lower_bound(time);
  if (it == m_chapterMap.end())
    return false;

  CLog::Log(LOGDEBUG, "TimelineDemuxer: Switch Demuxer");
  m_curChapter = it->second;
  bool result = m_curChapter->demuxer->SeekTime(time - m_curChapter->shiftTime(), backwords, startpts);
  if (result && startpts)
    (*startpts) += m_curChapter->shiftTime();
  return result;
}

bool CDemuxTimeline::SeekChapter(int chapter, double* startpts)
{
  --chapter;
  if (chapter < 0 || unsigned(chapter) >= m_chapters.size())
    return false;
  CLog::Log(LOGDEBUG, "TimelineDemuxer: Switch Demuxer");
  m_curChapter = &m_chapters[chapter];
  bool result = m_curChapter->demuxer->SeekTime(m_curChapter->startSrcTime, true, startpts);
  if (result && startpts)
    (*startpts) += m_curChapter->shiftTime();
  return result;
}

int CDemuxTimeline::GetChapterCount()
{
  return m_chapters.size();
}

int CDemuxTimeline::GetChapter()
{
  return m_curChapter->index + 1;
}

void CDemuxTimeline::GetChapterName(std::string& strChapterName, int chapterIdx)
{
  --chapterIdx;
  if (chapterIdx < 0 || unsigned(chapterIdx) >= m_chapters.size())
    return;
  strChapterName = m_chapters[chapterIdx].title;
}

int64_t CDemuxTimeline::GetChapterPos(int chapterIdx)
{
  --chapterIdx;
  if (chapterIdx < 0 || unsigned(chapterIdx) >= m_chapters.size())
    return 0;
  return (m_chapters[chapterIdx].startDispTime + 999) / 1000;
}

void CDemuxTimeline::SetSpeed(int iSpeed)
{
  for (auto &demuxer : m_demuxers)
    demuxer->SetSpeed(iSpeed);
}

int CDemuxTimeline::GetStreamLength()
{
  return m_chapterMap.rbegin()->first;
}

std::vector<CDemuxStream*> CDemuxTimeline::GetStreams() const
{
  return m_primaryDemuxer->GetStreams();
}

int CDemuxTimeline::GetNrOfStreams() const
{
  return m_primaryDemuxer->GetNrOfStreams();
}

std::string CDemuxTimeline::GetFileName()
{
  return m_primaryDemuxer->GetFileName();
}

void CDemuxTimeline::EnableStream(int id, bool enable)
{
  for (auto &demuxer : m_demuxers)
    demuxer->EnableStream(demuxer->GetDemuxerId(), id, enable);
}

CDemuxStream* CDemuxTimeline::GetStream(int iStreamId) const
{
  return m_primaryDemuxer->GetStream(m_primaryDemuxer->GetDemuxerId(), iStreamId);
}

std::string CDemuxTimeline::GetStreamCodecName(int iStreamId)
{
  return m_primaryDemuxer->GetStreamCodecName(m_primaryDemuxer->GetDemuxerId(), iStreamId);
}


CDemuxTimeline* CDemuxTimeline::CreateTimeline(CDVDDemux *demuxer)
{
  return CreateTimelineFromMatroskaParser(demuxer);
  return CreateTimelineFromEbml(demuxer);
}

std::string segUidToHex(std::string uid)
{
	const char *hex = "0123456789abcdef";
	std::string result("0x");
	result.reserve(18);
	for (unsigned char twoDigits : uid)
	{
		result.append(1, hex[(twoDigits >> 4) & 0xf]);
		result.append(1, hex[twoDigits & 0xf]);
	}
	return result;
}

CDemuxTimeline* CDemuxTimeline::CreateTimelineFromMatroskaParser(CDVDDemux *primaryDemuxer)
{
  std::unique_ptr<CDVDInputStreamFile> inStream(new CDVDInputStreamFile(CFileItem(primaryDemuxer->GetFileName(), false)));
  if (!inStream->Open())
    return nullptr;
  CDVDInputStream *input = inStream.get();

  MatroskaFile mkv;
  bool result = mkv.Parse(input);
  if (!result)
    return nullptr;

   // multiple editions unsupported (at the moment)
  if (mkv.segment.chapters.editions.size() != 1)
    return nullptr;
  auto &edition = mkv.segment.chapters.editions.front();
  // only handle ordered chapters
  if (!edition.flagOrdered)
    return nullptr;

  std::unique_ptr<CDemuxTimeline> timeline(new CDemuxTimeline);
  timeline->m_primaryDemuxer = primaryDemuxer;
  timeline->m_demuxers.emplace_back(primaryDemuxer);

  // collect needed segment uids
  std::set<MatroskaSegmentUID> neededSegmentUIDs;
  for (auto &chapter : edition.chapterAtoms)
    if (chapter.segUid.size() != 0 && chapter.segUid != mkv.segment.infos.uid)
      neededSegmentUIDs.insert(chapter.segUid);

  // find linked segments
  std::map<MatroskaSegmentUID,CDVDDemux*> segmentDemuxer;
  segmentDemuxer[""] = primaryDemuxer;
  segmentDemuxer[mkv.segment.infos.uid] = primaryDemuxer;
  std::list<std::string> searchDirs({""}); // should be a global setting
  std::string filename = primaryDemuxer->GetFileName();
  std::string dirname = filename.substr(0, filename.rfind('/') + 1);
  for (auto &subDir : searchDirs)
  {
    if (neededSegmentUIDs.size() == 0)
      break;
    CFileItemList files;
    XFILE::CDirectory::GetDirectory(dirname + subDir, files, ".mkv");
    for (auto &file : files.GetList())
    {
      std::unique_ptr<CDVDInputStreamFile> uInput2(new CDVDInputStreamFile(*file));
      CDVDInputStream *input2 = uInput2.get();
      if (!input2->Open())
        continue;
      MatroskaFile mkv2;
      if (!mkv2.Parse(input2))
        continue;
      if (neededSegmentUIDs.erase(mkv2.segment.infos.uid) == 0)
        continue;
      input2->Seek(mkv2.offsetBegin, SEEK_SET);
      std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
      if(demuxer->Open(input2))
      {
        segmentDemuxer[mkv2.segment.infos.uid] = demuxer.get();
        timeline->m_demuxers.emplace_back(std::move(demuxer));
        timeline->m_inputStreams.emplace_back(std::move(uInput2));
      }
      if (neededSegmentUIDs.size() == 0)
        break;
    }
  }

  // build timeline
  for (auto &segUid : neededSegmentUIDs)
    CLog::Log(LOGERROR,
      "TimelineDemuxer: Could not find matroska segment for segment linking: %s",
      segUidToHex(segUid).c_str()
    );

  int dispTime = 0;
  decltype(segmentDemuxer.begin()) it;
  for (auto &chapter : edition.chapterAtoms)
    if ((it = segmentDemuxer.find(chapter.segUid)) != segmentDemuxer.end())
    {
      timeline->m_chapters.emplace_back(
        it->second,
        chapter.timeStart / 1000000,
        dispTime,
        (chapter.timeEnd - chapter.timeStart) / 1000000,
        timeline->m_chapters.size(),
        chapter.displays.GetDefault()
      );
      dispTime += timeline->m_chapters.back().duration;
    }

  if (!timeline->m_chapters.size())
    return nullptr;

  for (auto &chapter : timeline->m_chapters)
    timeline->m_chapterMap[chapter.stopDispTime() - 1] = &chapter;

  timeline->m_curChapter = &timeline->m_chapters.front();
  return timeline.release();
}

// vim: ts=2 sw=2 expandtab
