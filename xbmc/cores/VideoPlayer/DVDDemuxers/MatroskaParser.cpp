#include "MatroskaParser.h"

//#include "Matroska/EbmlParser.h"

/*
* Matroska element IDs, max. 32 bits
*/

/* toplevel segment */
static const EbmlId MATROSKA_ID_SEGMENT = 0x18538067;

/* Matroska top-level master IDs */
static const EbmlId MATROSKA_ID_INFO = 0x1549A966;
static const EbmlId MATROSKA_ID_TRACKS = 0x1654AE6B;
static const EbmlId MATROSKA_ID_CUES = 0x1C53BB6B;
static const EbmlId MATROSKA_ID_TAGS = 0x1254C367;
static const EbmlId MATROSKA_ID_SEEKHEAD = 0x114D9B74;
static const EbmlId MATROSKA_ID_ATTACHMENTS = 0x1941A469;
static const EbmlId MATROSKA_ID_CLUSTER = 0x1F43B675;
static const EbmlId MATROSKA_ID_CHAPTERS = 0x1043A770;

/* IDs in the info master */
static const EbmlId MATROSKA_ID_TIMECODESCALE = 0x2AD7B1;
static const EbmlId MATROSKA_ID_DURATION = 0x4489;
static const EbmlId MATROSKA_ID_TITLE = 0x7BA9;
static const EbmlId MATROSKA_ID_WRITINGAPP = 0x5741;
static const EbmlId MATROSKA_ID_MUXINGAPP = 0x4D80;
static const EbmlId MATROSKA_ID_DATEUTC = 0x4461;
static const EbmlId MATROSKA_ID_SEGMENTUID = 0x73A4;

/* IDs in the tags master */
static const EbmlId MATROSKA_ID_TAG = 0x7373;
static const EbmlId MATROSKA_ID_SIMPLETAG = 0x67C8;
static const EbmlId MATROSKA_ID_TAGNAME = 0x45A3;
static const EbmlId MATROSKA_ID_TAGSTRING = 0x4487;
static const EbmlId MATROSKA_ID_TAGLANG = 0x447A;
static const EbmlId MATROSKA_ID_TAGDEFAULT = 0x4484;
static const EbmlId MATROSKA_ID_TAGDEFAULT_BUG = 0x44B4;
static const EbmlId MATROSKA_ID_TAGTARGETS = 0x63C0;
static const EbmlId MATROSKA_ID_TAGTARGETS_TYPE = 0x63CA;
static const EbmlId MATROSKA_ID_TAGTARGETS_TYPEVALUE = 0x68CA;
static const EbmlId MATROSKA_ID_TAGTARGETS_TRACKUID = 0x63C5;
static const EbmlId MATROSKA_ID_TAGTARGETS_CHAPTERUID = 0x63C4;
static const EbmlId MATROSKA_ID_TAGTARGETS_ATTACHUID = 0x63C6;

/* IDs in the seekhead master */
static const EbmlId MATROSKA_ID_SEEKENTRY = 0x4DBB;

/* IDs in the seekpoint master */
static const EbmlId MATROSKA_ID_SEEKID = 0x53AB;
static const EbmlId MATROSKA_ID_SEEKPOSITION = 0x53AC;

/* IDs in the chapters master */
static const EbmlId MATROSKA_ID_EDITIONENTRY = 0x45B9;
static const EbmlId MATROSKA_ID_EDITIONUID = 0x45BC;
static const EbmlId MATROSKA_ID_EDITIONFLAGHIDDEN = 0x45BD;
static const EbmlId MATROSKA_ID_EDITIONFLAGDEFAULT = 0x45DB;
static const EbmlId MATROSKA_ID_EDITIONFLAGORDERED = 0x45DD;
static const EbmlId MATROSKA_ID_CHAPTERATOM = 0xB6;
static const EbmlId MATROSKA_ID_CHAPTERUID = 0x73C4;
static const EbmlId MATROSKA_ID_CHAPTERTIMESTART = 0x91;
static const EbmlId MATROSKA_ID_CHAPTERTIMEEND = 0x92;
static const EbmlId MATROSKA_ID_CHAPTERFLAGHIDDEN = 0x98;
static const EbmlId MATROSKA_ID_CHAPTERFLAGENABLED = 0x4598;
static const EbmlId MATROSKA_ID_CHAPTERSEGMENTUID = 0x6E67;
static const EbmlId MATROSKA_ID_CHAPTERDISPLAY = 0x80;
static const EbmlId MATROSKA_ID_CHAPSTRING = 0x85;
static const EbmlId MATROSKA_ID_CHAPLANG = 0x437C;
static const EbmlId MATROSKA_ID_CHAPCOUNTRY = 0x437E;
static const EbmlId MATROSKA_ID_CHAPTERPHYSEQUIV = 0x63C3;

struct MatroskaSeekEntry
{
  EbmlId seekId = 0;
  uint64_t seekPos = -1;
};

EbmlMasterParser BindMatroskaSeekEntryParser(MatroskaSeekEntry *seekEntry)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_SEEKID] = BindEbmlUintParser(&seekEntry->seekId);
  master.parser[MATROSKA_ID_SEEKPOSITION] = BindEbmlUintParser(&seekEntry->seekPos);
  master.parsed = [seekEntry]() { return (seekEntry->seekId != 0) && (seekEntry->seekPos >= 0); };
  return master;
}

EbmlMasterParser BindMatroskaSeekMapParser(MatroskaSeekMap *seekMap, int64_t basePos)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_SEEKENTRY] = [seekMap,basePos](CDVDInputStream *input, uint64_t tagLen)
    {
      MatroskaSeekEntry seekEntry;
      if (!BindMatroskaSeekEntryParser(&seekEntry)(input, tagLen))
        return false;
      seekMap->insert(MatroskaSeekMap::value_type(seekEntry.seekId, basePos + seekEntry.seekPos));
      return true;
    };
  return master;
}

struct MatroskaChapterDisplay
{
  std::string chapString;
  std::list<std::string> chapLangs;
};

EbmlMasterParser BindMatroskaChapterDisplayParser(MatroskaChapterDisplay *chapDisplay)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_CHAPSTRING] = BindEbmlStringParser(&chapDisplay->chapString, 4096);
  master.parser[MATROSKA_ID_CHAPLANG] = BindEbmlListParser(&chapDisplay->chapLangs, BindEbmlStringParser, 32);
  return master;
}

EbmlParserFunctor BindMatroskaChapterDisplayMapParser(MatroskaChapterDisplayMap *displays)
{
  return [displays](CDVDInputStream *input, uint64_t tagLen)
    {
      MatroskaChapterDisplay disp;
      if (!BindMatroskaChapterDisplayParser(&disp)(input, tagLen))
        return false;
      for (auto &elem : disp.chapLangs)
        (*displays)[elem] = disp.chapString;
      return true;
    };
}

std::string MatroskaChapterDisplayMap::GetDefault()
{
  if (this->size() == 0)
    return std::string();
  if (this->size() == 1)
    return this->begin()->second;
  decltype(this->begin()) iterator;
  //if (this->end() != (iterator = this->find(GUILanguage))) // where to get the gui language from
  //  return iterator->second;
  if (this->end() != (iterator = this->find("eng")))
    return iterator->second;
  if (this->end() != (iterator = this->find("und")))
    return iterator->second;
  return this->begin()->second;
}

EbmlMasterParser BindMatroskaChapterAtomParser(MatroskaChapterAtom *chapAtom)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_CHAPTERUID] = BindEbmlUintParser(&chapAtom->uid);
  master.parser[MATROSKA_ID_CHAPTERTIMESTART] = BindEbmlUintParser(&chapAtom->timeStart);
  master.parser[MATROSKA_ID_CHAPTERTIMEEND] = BindEbmlUintParser(&chapAtom->timeEnd);
  master.parser[MATROSKA_ID_CHAPTERFLAGHIDDEN] = BindEbmlUintParser(&chapAtom->flagHidden);
  master.parser[MATROSKA_ID_CHAPTERFLAGENABLED] = BindEbmlUintParser(&chapAtom->flagEnabled);
  master.parser[MATROSKA_ID_CHAPTERSEGMENTUID] = BindEbmlRawParser(&chapAtom->segUid, 16);
  master.parser[MATROSKA_ID_CHAPTERDISPLAY] = BindMatroskaChapterDisplayMapParser(&chapAtom->displays);
  master.parser[MATROSKA_ID_CHAPTERATOM] = BindEbmlListParser(&chapAtom->subChapters, BindMatroskaChapterAtomParser);
  return master;
}

EbmlMasterParser BindMatroskaEditionParser(MatroskaEdition *edition)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_EDITIONUID] = BindEbmlUintParser(&edition->uid);
  master.parser[MATROSKA_ID_EDITIONFLAGHIDDEN] = BindEbmlUintParser(&edition->flagHidden);
  master.parser[MATROSKA_ID_EDITIONFLAGDEFAULT] = BindEbmlUintParser(&edition->flagDefault);
  master.parser[MATROSKA_ID_EDITIONFLAGORDERED] = BindEbmlUintParser(&edition->flagOrdered);
  master.parser[MATROSKA_ID_CHAPTERATOM] = BindEbmlListParser(&edition->chapterAtoms, BindMatroskaChapterAtomParser);
  return master;
}

EbmlMasterParser BindMatroskaChaptersParser(MatroskaChapters *chapters)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_EDITIONENTRY] = BindEbmlListParser(&chapters->editions, BindMatroskaEditionParser);
  return master;
}

EbmlMasterParser BindMatroskaInfoParser(MatroskaSegmentInfo *infos)
{
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_SEGMENTUID] = BindEbmlRawParser(&infos->uid, 16);
  master.parser[MATROSKA_ID_TIMECODESCALE] = BindEbmlUintParser(&infos->timecodeScale);
  return master;
}

bool MatroskaSegment::Parse(CDVDInputStream *input)
{
  uint64_t len;
  if (EbmlReadId(input) != MATROSKA_ID_SEGMENT)
    return false;
  if (!EbmlReadLen(input, &len))
    return false;
  offsetBegin = input->Seek(0, SEEK_CUR);
  offsetEnd = offsetBegin + len;
  EbmlMasterParser master;
  master.parser[MATROSKA_ID_SEEKHEAD] = BindMatroskaSeekMapParser(&this->seekMap, this->offsetBegin);
  master.parser[MATROSKA_ID_INFO] = BindMatroskaInfoParser(&this->infos);
  master.parser[MATROSKA_ID_CHAPTERS] = BindMatroskaChaptersParser(&this->chapters);
  master.parser[MATROSKA_ID_CLUSTER]; // break on cluster
  return master(input, len);
}

bool MatroskaFile::Parse(CDVDInputStream *input)
{
  offsetBegin = input->Seek(0, SEEK_CUR);
  if (!ebmlHeader.Parse(input))
    return false;
  input->Seek(ebmlHeader.offsetEnd, SEEK_SET);
  if (!segment.Parse(input))
    return false;
  offsetEnd = segment.offsetEnd;
  return true;
}

// vim: ts=2 sw=2 expandtab
