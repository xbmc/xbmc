#include "PlexTest.h"
#include "XMLChoice.h"
#include "PlexDirectory.h"

const char playQueueXMLNoMixed[] =
    "<?xml version=\"1.0\" ?>"
    "<MediaContainer identifier=\"com.plexapp.plugins.library\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\" playQueueID=\"483\" playQueueSelectedItemID=\"12093\" playQueueSelectedItemOffset=\"0\" playQueueTotalCount=\"2\" playQueueVersion=\"9\" size=\"2\">"
    "<Video addedAt=\"1391592946\" art=\"/library/metadata/2446/art/1391593013\" duration=\"3620224\" guid=\"local://2446\" key=\"/library/metadata/2446\" lastViewedAt=\"1399451703\" playQueueItemID=\"12093\" ratingKey=\"2446\" summary=\"\" thumb=\"/library/metadata/2446/thumb/1391593013\" title=\"Meckar Micke Och Det Hemliga Kalaset\" type=\"movie\" updatedAt=\"1391593013\" viewCount=\"696\" viewOffset=\"2395065\">"
    "<Media aspectRatio=\"1.78\" audioChannels=\"2\" audioCodec=\"aac\" bitrate=\"1388\" container=\"mp4\" duration=\"3620224\" has64bitOffsets=\"1\" height=\"576\" id=\"2432\" optimizedForStreaming=\"0\" videoCodec=\"h264\" videoFrameRate=\"PAL\" videoResolution=\"576\" width=\"702\">"
    "<Part container=\"mp4\" duration=\"3620224\" file=\"/data/Videos/Barn/Movies/Meckar Micke och det hemliga kalaset.mp4\" has64bitOffsets=\"1\" hasChapterTextStream=\"1\" id=\"2563\" indexes=\"sd\" key=\"/library/parts/2563/file.mp4\" optimizedForStreaming=\"0\" size=\"628270610\">"
    "<Stream anamorphic=\"1\" bitDepth=\"8\" bitrate=\"995\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codec=\"h264\" codecID=\"avc1\" colorSpace=\"yuv\" duration=\"3620094\" frameRate=\"25.000\" frameRateMode=\"vfr\" hasScalingMatrix=\"0\" height=\"576\" id=\"47570\" index=\"0\" level=\"30\" pixelAspectRatio=\"20480:14391\" profile=\"high\" refFrames=\"4\" scanType=\"progressive\" streamType=\"1\" title=\"\" width=\"702\"/>"
    "<Stream bitrate=\"165\" bitrateMode=\"vbr\" channels=\"2\" codec=\"aac\" codecID=\"40\" duration=\"3620224\" id=\"47571\" index=\"1\" profile=\"lc\" samplingRate=\"48000\" selected=\"1\" streamType=\"2\" title=\"\"/>"
    "<Stream bitDepth=\"16\" bitrate=\"224\" bitrateMode=\"cbr\" channels=\"2\" codec=\"ac3\" codecID=\"ac-3\" dialogNorm=\"-27\" duration=\"3620192\" id=\"47572\" index=\"2\" samplingRate=\"48000\" streamType=\"2\" title=\"\"/>"
    "</Part>" "</Media>" "</Video>"
    "<Video addedAt=\"1391592946\" art=\"/library/metadata/2444/art/1391593003\" contentRating=\"PG\" duration=\"4945960\" guid=\"com.plexapp.agents.imdb://tt0351283?lang=sv\" key=\"/library/metadata/2444\" lastViewedAt=\"1398529929\" originalTitle=\"Madagascar\" originallyAvailableAt=\"2005-05-25\" playQueueItemID=\"12099\" rating=\"6.0999999046325701\" ratingKey=\"2444\" studio=\"Pacific Data Images\" summary=\"Lejonet Alex, zebran Marty, den hypokondriske giraffen Melman och flodh?sten Gloria har levt hela sina liv i djurparken i Central Park. Men n?r Marty fyller 10 ?r blir han alltmer orolig och rastl?s och t?nker mycket p? hur det skulle vara att leva ute i det fria. N?r han f?r h?ra att pingvinerna t?nker rymma best?mmer han sig f?r att h?nga p?.\" tagline=\"Ton On The Run\" thumb=\"/library/metadata/2444/thumb/1391593003\" title=\"Madagaskar\" type=\"movie\" updatedAt=\"1391593003\" viewCount=\"563\" viewOffset=\"102869\" year=\"2005\">"
    "<Media aspectRatio=\"1.85\" audioChannels=\"2\" audioCodec=\"aac\" bitrate=\"1094\" container=\"mp4\" duration=\"4945960\" has64bitOffsets=\"0\" height=\"320\" id=\"2430\" optimizedForStreaming=\"1\" videoCodec=\"h264\" videoFrameRate=\"PAL\" videoResolution=\"sd\" width=\"592\">"
    "<Part container=\"mp4\" duration=\"4945960\" file=\"/data/Videos/Barn/Movies/Madagaskar.mp4\" has64bitOffsets=\"0\" id=\"2561\" indexes=\"sd\" key=\"/library/parts/2561/file.mp4\" optimizedForStreaming=\"1\" size=\"676563943\">"
    "<Stream bitDepth=\"8\" bitrate=\"896\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codec=\"h264\" codecID=\"avc1\" colorSpace=\"yuv\" duration=\"4945960\" frameRate=\"25.000\" frameRateMode=\"cfr\" hasScalingMatrix=\"0\" height=\"320\" id=\"47578\" index=\"0\" level=\"21\" profile=\"high\" refFrames=\"4\" scanType=\"progressive\" streamType=\"1\" title=\"\" width=\"592\"/>"
    "<Stream bitrate=\"192\" bitrateMode=\"cbr\" channels=\"2\" codec=\"aac\" codecID=\"40\" duration=\"4944000\" id=\"47579\" index=\"1\" profile=\"lc\" samplingRate=\"48000\" selected=\"1\" streamType=\"2\" title=\"\"/>"
    "</Part>" "</Media>" "</Video>"
    "</MediaContainer>";

const char playQueueXMLMixed[] =
    "<?xml version=\"1.0\" ?>"
    "<MediaContainer identifier=\"com.plexapp.plugins.library\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\" playQueueID=\"483\" playQueueSelectedItemID=\"12093\" playQueueSelectedItemOffset=\"0\" playQueueTotalCount=\"2\" playQueueVersion=\"9\" size=\"2\">"
    "<Video addedAt=\"1391592946\" art=\"/library/metadata/2446/art/1391593013\" duration=\"3620224\" guid=\"local://2446\" key=\"/library/metadata/2446\" lastViewedAt=\"1399451703\" playQueueItemID=\"12093\" ratingKey=\"2446\" summary=\"\" thumb=\"/library/metadata/2446/thumb/1391593013\" title=\"Meckar Micke Och Det Hemliga Kalaset\" type=\"movie\" updatedAt=\"1391593013\" viewCount=\"696\" viewOffset=\"2395065\">"
    "<Media aspectRatio=\"1.78\" audioChannels=\"2\" audioCodec=\"aac\" bitrate=\"1388\" container=\"mp4\" duration=\"3620224\" has64bitOffsets=\"1\" height=\"576\" id=\"2432\" optimizedForStreaming=\"0\" videoCodec=\"h264\" videoFrameRate=\"PAL\" videoResolution=\"576\" width=\"702\">"
    "<Part container=\"mp4\" duration=\"3620224\" file=\"/data/Videos/Barn/Movies/Meckar Micke och det hemliga kalaset.mp4\" has64bitOffsets=\"1\" hasChapterTextStream=\"1\" id=\"2563\" indexes=\"sd\" key=\"/library/parts/2563/file.mp4\" optimizedForStreaming=\"0\" size=\"628270610\">"
    "<Stream anamorphic=\"1\" bitDepth=\"8\" bitrate=\"995\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codec=\"h264\" codecID=\"avc1\" colorSpace=\"yuv\" duration=\"3620094\" frameRate=\"25.000\" frameRateMode=\"vfr\" hasScalingMatrix=\"0\" height=\"576\" id=\"47570\" index=\"0\" level=\"30\" pixelAspectRatio=\"20480:14391\" profile=\"high\" refFrames=\"4\" scanType=\"progressive\" streamType=\"1\" title=\"\" width=\"702\"/>"
    "<Stream bitrate=\"165\" bitrateMode=\"vbr\" channels=\"2\" codec=\"aac\" codecID=\"40\" duration=\"3620224\" id=\"47571\" index=\"1\" profile=\"lc\" samplingRate=\"48000\" selected=\"1\" streamType=\"2\" title=\"\"/>"
    "<Stream bitDepth=\"16\" bitrate=\"224\" bitrateMode=\"cbr\" channels=\"2\" codec=\"ac3\" codecID=\"ac-3\" dialogNorm=\"-27\" duration=\"3620192\" id=\"47572\" index=\"2\" samplingRate=\"48000\" streamType=\"2\" title=\"\"/>"
    "</Part>" "</Media>" "</Video>"
    "<Video addedAt=\"1391592946\" art=\"/library/metadata/2444/art/1391593003\" contentRating=\"PG\" duration=\"4945960\" guid=\"com.plexapp.agents.imdb://tt0351283?lang=sv\" key=\"/library/metadata/2444\" lastViewedAt=\"1398529929\" originalTitle=\"Madagascar\" originallyAvailableAt=\"2005-05-25\" playQueueItemID=\"12099\" rating=\"6.0999999046325701\" ratingKey=\"2444\" studio=\"Pacific Data Images\" summary=\"Lejonet Alex, zebran Marty, den hypokondriske giraffen Melman och flodh?sten Gloria har levt hela sina liv i djurparken i Central Park. Men n?r Marty fyller 10 ?r blir han alltmer orolig och rastl?s och t?nker mycket p? hur det skulle vara att leva ute i det fria. N?r han f?r h?ra att pingvinerna t?nker rymma best?mmer han sig f?r att h?nga p?.\" tagline=\"Ton On The Run\" thumb=\"/library/metadata/2444/thumb/1391593003\" title=\"Madagaskar\" type=\"episode\" updatedAt=\"1391593003\" viewCount=\"563\" viewOffset=\"102869\" year=\"2005\">"
    "<Media aspectRatio=\"1.85\" audioChannels=\"2\" audioCodec=\"aac\" bitrate=\"1094\" container=\"mp4\" duration=\"4945960\" has64bitOffsets=\"0\" height=\"320\" id=\"2430\" optimizedForStreaming=\"1\" videoCodec=\"h264\" videoFrameRate=\"PAL\" videoResolution=\"sd\" width=\"592\">"
    "<Part container=\"mp4\" duration=\"4945960\" file=\"/data/Videos/Barn/Movies/Madagaskar.mp4\" has64bitOffsets=\"0\" id=\"2561\" indexes=\"sd\" key=\"/library/parts/2561/file.mp4\" optimizedForStreaming=\"1\" size=\"676563943\">"
    "<Stream bitDepth=\"8\" bitrate=\"896\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codec=\"h264\" codecID=\"avc1\" colorSpace=\"yuv\" duration=\"4945960\" frameRate=\"25.000\" frameRateMode=\"cfr\" hasScalingMatrix=\"0\" height=\"320\" id=\"47578\" index=\"0\" level=\"21\" profile=\"high\" refFrames=\"4\" scanType=\"progressive\" streamType=\"1\" title=\"\" width=\"592\"/>"
    "<Stream bitrate=\"192\" bitrateMode=\"cbr\" channels=\"2\" codec=\"aac\" codecID=\"40\" duration=\"4944000\" id=\"47579\" index=\"1\" profile=\"lc\" samplingRate=\"48000\" selected=\"1\" streamType=\"2\" title=\"\"/>"
    "</Part>" "</Media>" "</Video>"
    "</MediaContainer>";

TEST(PlexDirectoryReadChildren, noMixedMembers)
{
  rapidxml::xml_document<> doc;
  std::string s(playQueueXMLNoMixed);
  doc.parse<0>((char*)s.c_str());

  XML_ELEMENT* element = doc.first_node();

  CFileItemList item;
  XFILE::CPlexDirectory dir;
  dir.ReadChildren(element, item);

  EXPECT_FALSE(item.GetProperty("hasMixedMembers").asBoolean());
}

TEST(PlexDirectoryReadChildren, mixedMembers)
{
  rapidxml::xml_document<> doc;
  std::string s(playQueueXMLMixed);
  doc.parse<0>((char*)s.c_str());

  XML_ELEMENT* element = doc.first_node();

  CFileItemList item;
  XFILE::CPlexDirectory dir;
  dir.ReadChildren(element, item);

  EXPECT_TRUE(item.GetProperty("hasMixedMembers").asBoolean());
}
