#include "GetProgramGuideResult.h"
#include <stdlib.h>

#include "tinyXML/tinyxml.h"
#include "utils/log.h"
/*
static const SContentType g_content_group[] =
{ { 0x10, "Movie/Drama" }
, { 0x20, "News/Current Affairs" }
, { 0x30, "Show/Game show" }
, { 0x40, "Sports" }
, { 0x50, "Children's/Youth" }
, { 0x60, "Music/Ballet/Dance" }
, { 0x70, "Arts/Culture (without music)" }
, { 0x80, "Social/Political issues/Economics" }
, { 0x90, "Childrens/Youth Education/Science/Factual" }
, { 0xa0, "Leisure hobbies" }
, { 0xb0, "Misc" }
, { 0xf0, "Unknown" }
};

static const SContentType g_content_type[] =
{
// movie/drama
  { 0x11, "Detective/Thriller" }
, { 0x12, "Adventure/Western/War" }
, { 0x13, "Science Fiction/Fantasy/Horror" }
, { 0x14, "Comedy" }
, { 0x15, "Soap/Melodrama/Folkloric" }
, { 0x16, "Romance" }
, { 0x17, "Serious/ClassicalReligion/Historical" }
, { 0x18, "Adult Movie/Drama" }

// news/current affairs
, { 0x21, "News/Weather Report" }
, { 0x22, "Magazine" }
, { 0x23, "Documentary" }
, { 0x24, "Discussion/Interview/Debate" }

// show/game show
, { 0x31, "Game show/Quiz/Contest" }
, { 0x32, "Variety" }
, { 0x33, "Talk" }

// sports
, { 0x41, "Special Event (Olympics/World cup/...)" }
, { 0x42, "Magazine" }
, { 0x43, "Football/Soccer" }
, { 0x44, "Tennis/Squash" }
, { 0x45, "Team sports (excluding football)" }
, { 0x46, "Athletics" }
, { 0x47, "Motor Sport" }
, { 0x48, "Water Sport" }
, { 0x49, "Winter Sports" }
, { 0x4a, "Equestrian" }
, { 0x4b, "Martial sports" }

// childrens/youth
, { 0x51, "Pre-school" }
, { 0x52, "Entertainment (6 to 14 year-olds)" }
, { 0x53, "Entertainment (10 to 16 year-olds)" }
, { 0x54, "Informational/Educational/Schools" }
, { 0x55, "Cartoons/Puppets" }

// music/ballet/dance
, { 0x61, "Rock/Pop" }
, { 0x62, "Serious music/Classical Music" }
, { 0x63, "Folk/Traditional music" }
, { 0x64, "Jazz" }
, { 0x65, "Musical/Opera" }
, { 0x66, "Ballet" }

// arts/culture
, { 0x71, "Performing Arts" }
, { 0x72, "Fine Arts" }
, { 0x73, "Religion" }
, { 0x74, "Popular Culture/Tradital Arts" }
, { 0x75, "Literature" }
, { 0x76, "Film/Cinema" }
, { 0x77, "Experimental Film/Video" }
, { 0x78, "Broadcasting/Press" }
, { 0x79, "New Media" }
, { 0x7a, "Magazine" }
, { 0x7b, "Fashion" }

// social/political/economic
, { 0x81, "Magazine/Report/Documentary" }
, { 0x82, "Economics/Social Advisory" }
, { 0x83, "Remarkable People" }

// children's youth: educational/science/factual
, { 0x91, "Nature/Animals/Environment" }
, { 0x92, "Technology/Natural sciences" }
, { 0x93, "Medicine/Physiology/Psychology" }
, { 0x94, "Foreign Countries/Expeditions" }
, { 0x95, "Social/Spiritual Sciences" }
, { 0x96, "Further Education" }
, { 0x97, "Languages" }

// leisure hobbies
, { 0xa1, "Tourism/Travel" }
, { 0xa2, "Handicraft" }
, { 0xa3, "Motoring" }
, { 0xa4, "Fitness & Health" }
, { 0xa5, "Cooking" }
, { 0xa6, "Advertisement/Shopping" }
, { 0xa7, "Gardening" }

// misc
, { 0xb0, "Original Language" }
, { 0xb1, "Black and White" }
, { 0xb2, "Unpublished" }
, { 0xb3, "Live Broadcast" }
};
*/

struct GenrePair
{
  GenrePair()
  {
	genretype_ = 0xf;
	genresubtype_ = 0xb2;
  };
  
  GenrePair(int type, int subtype)
  {
	genretype_ = type;
	genresubtype_ = subtype;
  };
  
  int genretype_;
  int genresubtype_;
};



class GenreIdMapper 
{
public:
  GenreIdMapper()
  {
	genreTypeIdMap_["Auction"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Awards"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Biography"] = GenrePair (0x70, 0x74);
	genreTypeIdMap_["Educational"] = GenrePair (0x90, 0x96);
	genreTypeIdMap_["Entertainment"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Holiday"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Holiday special"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Home improvement"] = GenrePair (0xA0, 0xa7);
	genreTypeIdMap_["How-to"] = GenrePair (0x90, 0x96);
	genreTypeIdMap_["Music"] = GenrePair (0x60, 0x61);
	genreTypeIdMap_["Music special"] = GenrePair (0x60, 0x61);
	genreTypeIdMap_["Music talk"] = GenrePair (0x60, 0x61);
	genreTypeIdMap_["Shopping"] = GenrePair (0xA0, 0xa6);
	genreTypeIdMap_["Sitcom"] = GenrePair (0x10, 0x14);
	genreTypeIdMap_["Soap"] = GenrePair (0x10, 0x15);
	genreTypeIdMap_["Soap talk"] = GenrePair (0x10, 0x15);
	genreTypeIdMap_["Golf"] = GenrePair (0x40, 0x41);
	genreTypeIdMap_["Lacrosse"] = GenrePair (0x40, 0x45);
	genreTypeIdMap_["Law"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Card games"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Collectibles"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Community"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Computers"] = GenrePair (0x90, 0x92);
	genreTypeIdMap_["Consumer"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Fundraiser"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Gaming"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Gay/lesbian"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Military"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Miniseries"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Opera"] = GenrePair (0x60, 0x65);
	genreTypeIdMap_["Parade"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Paranormal"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Parenting"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Poker"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Reality"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Self improvement"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Special"] = GenrePair (0xB0, 0xb2);
	genreTypeIdMap_["Standup"] = GenrePair (0x10, 0x14);
	genreTypeIdMap_["Politics"] = GenrePair (0x80, 0x82);
	genreTypeIdMap_["Public affairs"] = GenrePair (0x80, 0x82);
	genreTypeIdMap_["Historical drama"] = GenrePair (0x10, 0x17);
	genreTypeIdMap_["History"] = GenrePair (0x10, 0x17);
	genreTypeIdMap_["Boat"] = GenrePair (0xA0, 0xa3);
	genreTypeIdMap_["Bus./financial"] = GenrePair (0x80, 0x82);
	genreTypeIdMap_["Auto"] = GenrePair (0xA0, 0xa3);
	genreTypeIdMap_["Aviation"] = GenrePair (0xA0, 0xa3);
	genreTypeIdMap_["Nature"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Agriculture"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Animals"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Environment"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["French"] = GenrePair (0x90, 0x97);
	genreTypeIdMap_["Horse"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Outdoors"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Science"] = GenrePair (0x90, 0x92);
	genreTypeIdMap_["Technology"] = GenrePair (0x90, 0x92);
	genreTypeIdMap_["Medical"] = GenrePair (0x90, 0x93);
	genreTypeIdMap_["Hunting"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Fishing"] = GenrePair (0x90, 0x91);
	genreTypeIdMap_["Health"] = GenrePair (0xA0, 0xa4);
	genreTypeIdMap_["Cooking"] = GenrePair (0xA0, 0xa5);
	genreTypeIdMap_["House/garden"] = GenrePair (0xA0, 0xa7);
	genreTypeIdMap_["Motorcycle"] = GenrePair (0xA0, 0xa3);
	genreTypeIdMap_["Travel"] = GenrePair (0xA0, 0xa1);
	genreTypeIdMap_["Aerobics"] = GenrePair (0xA0, 0xa4);
	genreTypeIdMap_["Exercise"] = GenrePair (0xA0, 0xa4);
	genreTypeIdMap_["Anthology"] = GenrePair (0x70, 0x74);
	genreTypeIdMap_["Art"] = GenrePair (0x70, 0x72);
	genreTypeIdMap_["Arts/crafts"] = GenrePair (0x70, 0x74);
	genreTypeIdMap_["Fashion"] = GenrePair (0x70, 0x7b);
	genreTypeIdMap_["Performing arts"] = GenrePair (0x70, 0x71);
	genreTypeIdMap_["Spanish"] = GenrePair (0x90, 0x97);
	genreTypeIdMap_["Religious"] = GenrePair (0x70, 0x73);
	genreTypeIdMap_["Dance"] = GenrePair (0x60, 0x66);
	genreTypeIdMap_["Animated"] = GenrePair (0x50, 0x55);
	genreTypeIdMap_["Anime"] = GenrePair (0x50, 0x55);
	genreTypeIdMap_["Children"] = GenrePair (0x50, 0x52);
	genreTypeIdMap_["Children-music"] = GenrePair (0x50, 0x52);
	genreTypeIdMap_["Children-special"] = GenrePair (0x50, 0x53);
	genreTypeIdMap_["Holiday-children"] = GenrePair (0x50, 0x52);
	genreTypeIdMap_["Holiday-children special"] = GenrePair (0x50, 0x52);
	genreTypeIdMap_["Game show"] = GenrePair (0x30, 0x31);
	genreTypeIdMap_["Talk"] = GenrePair (0x30, 0x33);
	genreTypeIdMap_["Variety"] = GenrePair (0x30, 0x32);
	genreTypeIdMap_["Debate"] = GenrePair (0x20, 0x24);
	genreTypeIdMap_["Docudrama"] = GenrePair (0x20, 0x23);
	genreTypeIdMap_["Documentary"] = GenrePair (0x20, 0x23);
	genreTypeIdMap_["Interview"] = GenrePair (0x20, 0x24);
	genreTypeIdMap_["News"] = GenrePair (0x20, 0x21);
	genreTypeIdMap_["Newsmagazine"] = GenrePair (0x20, 0x21);
	genreTypeIdMap_["Weather"] = GenrePair (0x20, 0x21);
	genreTypeIdMap_["Action"] = GenrePair (0x10, 0x12);
	genreTypeIdMap_["Adults only"] = GenrePair (0x10, 0x18);
	genreTypeIdMap_["Adventure"] = GenrePair (0x10, 0x12);
	genreTypeIdMap_["Comedy"] = GenrePair (0x10, 0x14);
	genreTypeIdMap_["Comedy-drama"] = GenrePair (0x10, 0x14);
	genreTypeIdMap_["Crime"] = GenrePair (0x10, 0x11);
	genreTypeIdMap_["Crime drama"] = GenrePair (0x10, 0x11);
	genreTypeIdMap_["Drama"] = GenrePair (0x10, 0x18);
	genreTypeIdMap_["Fantasy"] = GenrePair (0x10, 0x13);
	genreTypeIdMap_["Horror"] = GenrePair (0x10, 0x13);
	genreTypeIdMap_["Musical"] = GenrePair (0x60, 0x65);
	genreTypeIdMap_["Musical comedy"] = GenrePair (0x60, 0x65);
	genreTypeIdMap_["Mystery"] = GenrePair (0x10, 0x12);
	genreTypeIdMap_["Romance"] = GenrePair (0x10, 0x16);
	genreTypeIdMap_["Romance-comedy"] = GenrePair (0x10, 0x16);
	genreTypeIdMap_["Science fiction"] = GenrePair (0x10, 0x13);
	genreTypeIdMap_["Suspense"] = GenrePair (0x10, 0x11);
	genreTypeIdMap_["War"] = GenrePair (0x10, 0x12);
	genreTypeIdMap_["Western"] = GenrePair (0x10, 0x12);
	genreTypeIdMap_["Action sports"] = GenrePair (0x40, 0x4b);
  };
  ~GenreIdMapper()
  {
  };
  
  const GenrePair& getGenreTypeId(CStdString& genre)
  {
	std::map<CStdString, GenrePair>::iterator it;
	it = genreTypeIdMap_.find(genre);
	if(it != genreTypeIdMap_.end())
	  return it->second;
	return c_unknown_;
  };
  
private:
  std::map<CStdString, GenrePair> genreTypeIdMap_;
  const GenrePair c_unknown_;
};

GenreIdMapper GetProgramGuideResult::s_mapper_;
 
GetProgramGuideResult::GetProgramGuideResult() {
}

GetProgramGuideResult::~GetProgramGuideResult() {
}

void GetProgramGuideResult::parseData(const CStdString& xmlData) {
	TiXmlDocument xml;
	xml.Parse(xmlData.c_str(), 0, TIXML_ENCODING_LEGACY);

	TiXmlElement* rootXmlNode = xml.RootElement();

	if (!rootXmlNode) {
		errors_.push_back(" No root node parsed");
		CLog::Log(LOGDEBUG, "MythXML GetProgramGuideResult - No root node parsed");
	    return;
	}

	TiXmlElement* programGuideResponseNode = NULL;
	CStdString strValue = rootXmlNode->Value();
	if (strValue.Find("GetProgramGuideResponse") >= 0 ) {
		programGuideResponseNode = rootXmlNode;
	}
	else if (strValue.Find("detail") >= 0 ) {
		// process the error.
		TiXmlElement* errorCodeXmlNode = rootXmlNode->FirstChildElement("errorCode");
		TiXmlElement* errorDescXmlNode = rootXmlNode->FirstChildElement("errorDescription");
		CStdString error;
		error.Format("ErrorCode [%i] - %s", errorCodeXmlNode->GetText(), errorDescXmlNode->GetText());
		errors_.push_back(error);
		return;
	 }
	else
		return;

	TiXmlElement* programGuideNode = programGuideResponseNode->FirstChildElement("ProgramGuide");
	TiXmlElement* channelsNode = programGuideNode->FirstChildElement("Channels");
	TiXmlElement* channelNode = NULL;
	TiXmlElement* programNode = NULL;
	for( channelNode = channelsNode->FirstChildElement("Channel"); channelNode; channelNode = channelNode->NextSiblingElement("Channel")){
	  int chanId = atoi(channelNode->Attribute("chanId"));
      for( programNode = channelNode->FirstChildElement("Program"); programNode; programNode = programNode->NextSiblingElement("Program")){
		CStdString category = programNode->Attribute("category");
		CStdString itemStart = programNode->Attribute("startTime");
		CStdString itemEnd = programNode->Attribute("endTime");
		const GenrePair& genres = s_mapper_.getGenreTypeId(category);
		SEpg epg;
		epg.chan_num = chanId;
		epg.description = programNode->GetText();
		epg.title = programNode->Attribute("title");
		epg.subtitle =  programNode->Attribute("subTitle");
		epg.genre_type = genres.genretype_;
		epg.genre_subtype = genres.genresubtype_;
		epg.start_time = MythXmlCommandResult::convertTimeStringToObject(itemStart);
		epg.end_time = MythXmlCommandResult::convertTimeStringToObject(itemEnd);
		epg_.push_back(epg);
	  }
	}
}
