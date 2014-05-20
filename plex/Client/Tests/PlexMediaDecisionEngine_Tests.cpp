#include "PlexTest.h"
#include "PlexMediaDecisionEngine.h"

const char itemdetails[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<MediaContainer size=\"1\" allowSync=\"1\" identifier=\"com.plexapp.plugins.library\" librarySectionID=\"1\" librarySectionTitle=\"Movies\" librarySectionUUID=\"8a963b4b-68b5-4e01-9c0b-64cc416fbc3f\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\">"
    "<Video ratingKey=\"1888\" key=\"/library/metadata/1888\" guid=\"com.plexapp.agents.imdb://tt0290334?lang=en\" studio=\"20th Century Fox\" type=\"movie\" title=\"X2\" contentRating=\"PG-13\" summary=\"Professor Charles Xavier and his team of genetically gifted superheroes face a rising tide of anti-mutant sentiment led by Col. William Stryker in this sequel to the Marvel Comics-based blockbuster X-Men. Storm, Wolverine and Jean Grey must join their usual nemeses Magneto and Mystique to unhinge Stryker&apos;s scheme to exterminate all mutants.\" rating=\"6.3000001907348597\" year=\"2003\" tagline=\"The time has come for those who are different to stand united\" thumb=\"/library/metadata/1888/thumb/1391598827\" art=\"/library/metadata/1888/art/1391598827\" duration=\"8027491\" originallyAvailableAt=\"2003-04-24\" addedAt=\"1385840831\" updatedAt=\"1391598827\">"
    "<Media videoResolution=\"1080\" id=\"1846\" duration=\"8027491\" bitrate=\"11675\" width=\"1920\" height=\"800\" aspectRatio=\"2.35\" audioChannels=\"6\" audioCodec=\"dca\" videoCodec=\"h264\" container=\"mkv\" videoFrameRate=\"24p\">"
    "<Part id=\"1950\" key=\"/library/parts/1950/file.mkv\" duration=\"8027491\" file=\"/data/Videos/Movies/X2 X-Men United (2003)/X2 X-Men United.2003.mkv\" size=\"11714969381\" container=\"mkv\" indexes=\"sd\">"
    "<Stream id=\"47867\" streamType=\"1\" codec=\"h264\" index=\"0\" bitrate=\"10164\" language=\"English\" languageCode=\"eng\" bitDepth=\"8\" cabac=\"1\" chromaSubsampling=\"4:2:0\" codecID=\"V_MPEG4/ISO/AVC\" colorSpace=\"yuv\" duration=\"8027486\" frameRate=\"23.976\" frameRateMode=\"cfr\" hasScalingMatrix=\"0\" height=\"800\" level=\"41\" profile=\"high\" refFrames=\"5\" scanType=\"progressive\" title=\"\" width=\"1920\" />"
    "<Stream id=\"47868\" streamType=\"2\" selected=\"1\" codec=\"dca\" index=\"1\" channels=\"6\" bitrate=\"1509\" language=\"English\" languageCode=\"eng\" bitDepth=\"24\" bitrateMode=\"cbr\" codecID=\"A_DTS\" duration=\"8027491\" samplingRate=\"48000\" title=\"\" />"
    "<Stream id=\"47869\" streamType=\"3\" selected=\"1\" default=\"1\" index=\"2\" language=\"English\" languageCode=\"eng\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47870\" streamType=\"3\" index=\"3\" language=\"Nederlands\" languageCode=\"dut\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47871\" streamType=\"3\" index=\"4\" language=\"Français\" languageCode=\"fre\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47872\" streamType=\"3\" index=\"5\" language=\"Italiano\" languageCode=\"ita\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "<Stream id=\"47873\" streamType=\"3\" index=\"6\" language=\"Português\" languageCode=\"por\" codecID=\"S_TEXT/UTF8\" format=\"srt\" title=\"\" />"
    "</Part>" "</Media>"
    "<Genre id=\"21\" tag=\"Thriller\" />"
    "<Genre id=\"2373\" tag=\"Action/Adventure\" />"
    "<Genre id=\"316\" tag=\"Fantasy\" />"
    "<Genre id=\"20\" tag=\"Science Fiction\" />"
    "<Genre id=\"2273\" tag=\"Superhero movie\" />"
    "<Genre id=\"1516\" tag=\"Action film\" />"
    "<Writer id=\"3435\" tag=\"David Hayter\" />"
    "<Writer id=\"3499\" tag=\"Dan Harris\" />"
    "<Writer id=\"3500\" tag=\"Michael Dougherty\" />"
    "<Director id=\"3434\" tag=\"Bryan Singer\" />"
    "<Country id=\"24\" tag=\"USA\" />"
    "<Role id=\"3501\" tag=\"Bryce Hodgson\" role=\"Artie Maddicks\" />"
    "<Role id=\"3502\" tag=\"Daniel Cudmore\" role=\"Piotr Rasputin\" />"
   "<Collection id=\"3931\" tag=\"Marvel\" />"
    "<Field name=\"thumb\" locked=\"1\" />"
    "<Field name=\"collection\" locked=\"1\" />"
    "</Video>"
    "</MediaContainer>";

const char twititem[] =
    "<?xml version='1.0' encoding='utf-8'?>"
    "<MediaContainer size=\"1\" identifier=\"com.plexapp.system\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\" allowSync=\"1\">"
    "  <Video url=\"http://twit.tv/twig/249\" sourceIcon=\"http://resources-cdn.plexapp.com/image/source/com.plexapp.plugins.twitlive.jpg?h=86e8260\" key=\"/system/services/url/lookup?url=http%3A%2F%2Ftwit.tv%2Ftwig%2F249&amp;syncable=1\" type=\"episode\" originallyAvailableAt=\"2014-05-14\" summary=\"The right to be forgotten on Google, the real cost of Glass, things to look for at I/O, and more.\" absoluteIndex=\"249\" title=\"Slow Talkers\" art=\"http://resources-cdn.plexapp.com/image/art/com.plexapp.plugins.twitlive.jpg?h=2cee8fb\" thumb=\"/:/plugins/com.plexapp.system/resources/contentWithFallback?identifier=com.plexapp.plugins.twitlive&amp;urls=http%253A%2F%2Ftwit.tv%2Ffiles%2Fimagecache%2Fslideshow-slide%2Fspiros_twig_0249jpg.jpg%2Chttp%253A%2F%2Ftwit.tv%2Ffiles%2Fcoverart%2Ftwig144_1.jpg\" grandparentTitle=\"This Week in Google\" sourceTitle=\"TWiT.TV\" ratingKey=\"http://twit.tv/twig/249\">"
    "    <Media audioChannels=\"2\" container=\"mp4\" optimizedForStreaming=\"1\" height=\"720\" width=\"1280\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"720\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.twitlive/TWiT.TV/PlayMedia?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzMjMKaHR0cDovL3R3aXQudHYvdHdpZy8yNDlzMwp1cmxzNAo3MjBwczMKZm10cjAK&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%202%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%20720%2C%20%22width%22%3A%201280%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%22720%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"720\" width=\"1280\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" channels=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "    <Media audioChannels=\"2\" container=\"mp4\" optimizedForStreaming=\"1\" height=\"480\" width=\"856\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"480\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.twitlive/TWiT.TV/PlayMedia?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzMjMKaHR0cDovL3R3aXQudHYvdHdpZy8yNDlzMwp1cmxzNAo0ODBwczMKZm10cjAK&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%202%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%20480%2C%20%22width%22%3A%20856%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%22480%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"480\" width=\"856\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" channels=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "    <Media audioChannels=\"2\" container=\"mp4\" optimizedForStreaming=\"1\" height=\"360\" width=\"640\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"sd\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.twitlive/TWiT.TV/PlayMedia?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzMjMKaHR0cDovL3R3aXQudHYvdHdpZy8yNDlzMwp1cmxzMgpzZHMzCmZtdHIwCg__&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%202%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%20360%2C%20%22width%22%3A%20640%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%22sd%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"360\" width=\"640\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" channels=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "  </Video>"
    "</MediaContainer>";

const char twititem2indirect[] =
    "<?xml version='1.0' encoding='utf-8'?>"
    "<MediaContainer size=\"1\" identifier=\"com.plexapp.system\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\">"
    "  <Video sourceIcon=\"http://resources-cdn.plexapp.com/image/source/com.plexapp.plugins.twitlive.jpg?h=86e8260\" key=\"http://www.podtrac.com/pts/redirect.mp4/twit.cachefly.net/video/twig/twig0249/twig0249_h264m_864x480_500.mp4\" type=\"clip\">"
    "    <Media audioChannels=\"2\" optimizedForStreaming=\"1\" height=\"480\" width=\"856\" container=\"mp4\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"480\">"
    "      <Part container=\"mp4\" key=\"http://www.podtrac.com/pts/redirect.mp4/twit.cachefly.net/video/twig/twig0249/twig0249_h264m_864x480_500.mp4\" file=\"\" optimizedForStreaming=\"1\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"480\" width=\"856\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" channels=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "  </Video>"
    "</MediaContainer>";

const char youtubeitem[] =
    "<?xml version='1.0' encoding='utf-8'?>"
    "<MediaContainer size=\"1\" identifier=\"com.plexapp.system\" mediaTagPrefix=\"/system/bundle/media/flags/\" mediaTagVersion=\"1391191269\">"
    "  <Video url=\"http://www.youtube.com/watch?v=g3Rf5qDuq7M\" sourceIcon=\"http://resources-cdn.plexapp.com/image/source/com.plexapp.plugins.youtube.jpg?h=3d66c94\" key=\"/system/services/url/lookup?url=http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3Dg3Rf5qDuq7M\" type=\"clip\" rating=\"9.803748\" duration=\"513000\" title=\"Bars &amp; Melody - Simon Cowell's Golden Buzzer act | Britain's Got Talent 2014\" originallyAvailableAt=\"2014-05-10\" summary=\"See more from Britain's Got Talent at http://itv.com/talent&#10;&#10;Simon finally gets around to pushing his Golden Buzzer for a youthful musical duo. Bars &amp; Melody combine cuteness and originality, charming our audience with their skills.&#10;&#10;SUBSCRIBE: http://bit.ly/BGTsub&#10;Facebook: http://www.facebook.com/BritainsGotTalent&#10;Twitter: http://twitter.com/GotTalent\" sourceTitle=\"YouTube\" art=\"http://resources-cdn.plexapp.com/image/art/com.plexapp.plugins.youtube.jpg?h=50a9c38\" thumb=\"/:/plugins/com.plexapp.system/resources/contentWithFallback?identifier=com.plexapp.plugins.youtube&amp;urls=http%253A%2F%2Fi1.ytimg.com%2Fvi%2Fg3Rf5qDuq7M%2Fhqdefault.jpg\" ratingKey=\"http://www.youtube.com/watch?v=g3Rf5qDuq7M\">"
    "    <Media container=\"mp4\" optimizedForStreaming=\"1\" height=\"1080\" width=\"1920\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"1080\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.youtube/YouTube/PlayVideo?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzNDIKaHR0cDovL3d3dy55b3V0dWJlLmNvbS93YXRjaD92PWczUmY1cUR1cTdNczMKdXJsczUKMTA4MHBzMTEKZGVmYXVsdF9mbXRyMAo_&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%20null%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%201080%2C%20%22width%22%3A%201920%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%221080%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\" postURL=\"http://www.youtube.com/watch?v=g3Rf5qDuq7M\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"1080\" width=\"1920\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "    <Media container=\"mp4\" optimizedForStreaming=\"1\" height=\"720\" width=\"1280\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"720\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.youtube/YouTube/PlayVideo?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzNDIKaHR0cDovL3d3dy55b3V0dWJlLmNvbS93YXRjaD92PWczUmY1cUR1cTdNczMKdXJsczQKNzIwcHMxMQpkZWZhdWx0X2ZtdHIwCg__&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%20null%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%20720%2C%20%22width%22%3A%201280%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%22720%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\" postURL=\"http://www.youtube.com/watch?v=g3Rf5qDuq7M\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"720\" width=\"1280\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "    <Media container=\"mp4\" optimizedForStreaming=\"1\" height=\"360\" width=\"640\" audioCodec=\"aac\" videoCodec=\"h264\" videoResolution=\"360\" indirect=\"1\">"
    "      <Part container=\"mp4\" key=\"/:/plugins/com.plexapp.system/serviceFunction/url/com.plexapp.plugins.youtube/YouTube/PlayVideo?args=Y2VyZWFsMQoxCnR1cGxlCjAKcjAK&amp;kwargs=Y2VyZWFsMQoxCmRpY3QKMgpzNDIKaHR0cDovL3d3dy55b3V0dWJlLmNvbS93YXRjaD92PWczUmY1cUR1cTdNczMKdXJsczQKMzYwcHMxMQpkZWZhdWx0X2ZtdHIwCg__&amp;indirect=1&amp;mediaInfo=%7B%22audio_channels%22%3A%20null%2C%20%22protocol%22%3A%20null%2C%20%22optimized_for_streaming%22%3A%20true%2C%20%22video_frame_rate%22%3A%20null%2C%20%22duration%22%3A%20null%2C%20%22height%22%3A%20360%2C%20%22width%22%3A%20640%2C%20%22container%22%3A%20%22mp4%22%2C%20%22audio_codec%22%3A%20%22aac%22%2C%20%22aspect_ratio%22%3A%20null%2C%20%22video_codec%22%3A%20%22h264%22%2C%20%22video_resolution%22%3A%20%22360%22%2C%20%22bitrate%22%3A%20null%7D\" file=\"\" optimizedForStreaming=\"1\" postURL=\"http://www.youtube.com/watch?v=g3Rf5qDuq7M\">"
    "        <Stream index=\"0\" selected=\"1\" streamType=\"1\" height=\"360\" width=\"640\" codec=\"h264\" id=\"1\"/>"
    "        <Stream index=\"1\" selected=\"1\" streamType=\"2\" codec=\"aac\" id=\"2\"/>"
    "      </Part>"
    "    </Media>"
    "    <Genre tag=\"Entertainment\"/>"
    "  </Video>"
    "</MediaContainer>";

const char itunesItem[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<MediaContainer size=\"14\" art=\"/:/resources/itunes-fanart.jpg\" content=\"songs\" identifier=\"com.plexapp.plugins.itunes\" title1=\"Assemblage 23\" title2=\"Early, Rare, and Unreleased 1988-1998\" viewGroup=\"track\">"
    "<Track ratingKey=\"219\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"219.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Live Intro 96\" index=\"1\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/01-Assemblage_23-Live_Intro_96.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"82076\" size=\"1970219\" />"
    "<Track ratingKey=\"221\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"221.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Mortuary\" index=\"2\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/02-Assemblage_23-Mortuary.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"206628\" size=\"5452951\" />"
    "<Track ratingKey=\"223\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"223.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Anger\" index=\"3\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/03-Assemblage_23-Anger.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"216764\" size=\"5781299\" />"
    "<Track ratingKey=\"225\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"225.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Chemical Restraint\" index=\"4\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/04-Assemblage_23-Chemical_Restraint.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"496666\" size=\"12816477\" />"
    "<Track ratingKey=\"227\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"227.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Straightjacket\" index=\"5\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/05-Assemblage_23-Straightjacket.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"254119\" size=\"6434623\" />"
    "<Track ratingKey=\"229\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"229.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Ambush\" index=\"6\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/06-Assemblage_23-Ambush.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"224600\" size=\"5799478\" />"
    "<Track ratingKey=\"231\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"231.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Soma\" index=\"7\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/07-Assemblage_23-Soma.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"213812\" size=\"5185534\" />"
    "<Track ratingKey=\"233\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"233.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"The Angels Died\" index=\"8\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/08-Assemblage_23-The_Angels_Died.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"165668\" size=\"4073499\" />"
    "<Track ratingKey=\"235\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"235.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Sometimes I Wish I Was Dead\" index=\"9\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/09-Assemblage_23-Sometimes_I_Wish_I_Was_Dead.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"252342\" size=\"5950894\" />"
    "<Track ratingKey=\"237\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"237.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Underneath the Ice\" index=\"10\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/10-Assemblage_23-Underneath_the_Ice.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"294713\" size=\"7278276\" />"
    "<Track ratingKey=\"239\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"239.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"My Burden\" index=\"11\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/11-Assemblage_23-My_Burden.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"153338\" size=\"3669330\" />"
    "<Track ratingKey=\"241\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"241.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"The Fissure King\" index=\"12\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/12-Assemblage_23-The_Fissure_King.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"248084\" size=\"6408801\" />"
    "<Track ratingKey=\"243\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"243.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Void\" index=\"13\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/13-Assemblage_23-Void.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"278047\" size=\"6583788\" />"
    "<Track ratingKey=\"245\" thumb=\"/music/iTunes/thumbs/album/Assemblage%2023.Early,%20Rare,%20and%20Unreleased%201988-1998.jpg\" key=\"245.mp3\" artist=\"Assemblage 23\" album=\"Early, Rare, and Unreleased 1988-1998\" albumArtist=\"Assemblage 23\" track=\"Reqiuem\" index=\"14\" file=\"/Users/tru/Music/Unsorted/Music/Assemblage_23/Early,_Rare,_and_Unreleased_1988-1998/14-Assemblage_23-Reqiuem.mp3\" compilation=\"0\" disc=\"1\" rating=\"0\" totalTime=\"130168\" size=\"3255892\" />"
    "</MediaContainer>";

class PlexMediaDecisionJobFake : public CPlexMediaDecisionJob
{
public:
  PlexMediaDecisionJobFake(const CFileItem& item, const CFileItemPtr& resolvedItem = CFileItemPtr(),
                           const CFileItemPtr& indItem = CFileItemPtr())
    : CPlexMediaDecisionJob(item)
  {
    fakeResolveItem = resolvedItem;
    indirectItem = indItem;
  }

  CFileItemPtr GetUrl(const CStdString& url)
  {
    CURL u(url);
    if (indirectItem && boost::starts_with(u.GetFileName(), ":/plugins/com.plexapp.system/serviceFunction"))
    {
      if (url == indirectItem->GetProperty("indirectUrl").asString())
        return indirectItem;
      return CFileItemPtr();
    }

    EXPECT_TRUE(fakeResolveItem);

    return fakeResolveItem;
  }

  CFileItemPtr fakeResolveItem;
  CFileItemPtr indirectItem;
};

class PlexMediaDecisionEngineTest : public PlexServerManagerTestUtility
{
public:
  void SetUp()
  {
    origItem.SetPlexDirectoryType(PLEX_DIR_TYPE_VIDEO);
    origItem.SetProperty("unprocessed_key", "/library/metadata/123");
    origItem.SetPath("plexserver://abc123/library/foo");
    origItem.SetProperty("type", "movie");

    PlexServerManagerTestUtility::SetUp();
  }

  CFileItemPtr getDetailedItem(const CStdString& xml)
  {
    CFileItemList list;
    PlexTestUtils::listFromXML(xml, list);
    CFileItemPtr detailedItem = list.Get(0);

    EXPECT_TRUE(detailedItem);
    return detailedItem;
  }

  CFileItem origItem;
};

TEST_F(PlexMediaDecisionEngineTest, basicResolv)
{
  PlexMediaDecisionJobFake job(origItem, getDetailedItem(itemdetails));

  EXPECT_TRUE(job.DoWork());
  CFileItem& media = job.m_choosenMedia;
  EXPECT_STREQ("1888", media.GetProperty("ratingKey").asString().c_str());
  EXPECT_STREQ("plexserver://abc123/library/parts/1950/file.mkv", media.GetPath());
}

TEST_F(PlexMediaDecisionEngineTest, channelResolv)
{
  CFileItemPtr indirectItem = getDetailedItem(twititem2indirect);
  CFileItemPtr channelItem = getDetailedItem(twititem);
  int selectedMediaItem = 1;

  indirectItem->SetProperty("indirectUrl", channelItem->m_mediaItems[selectedMediaItem]->m_mediaParts[0]->GetPath());
  origItem.SetProperty("selectedMediaItem", selectedMediaItem);

  PlexMediaDecisionJobFake job(origItem, channelItem, indirectItem);

  EXPECT_TRUE(job.DoWork());
  CFileItem& media = job.m_choosenMedia;
  EXPECT_STREQ(indirectItem->m_mediaItems[0]->m_mediaParts[0]->GetPath(), media.GetPath());
}

TEST_F(PlexMediaDecisionEngineTest, itunesResolve)
{
  CFileItemPtr item = getDetailedItem(itunesItem);
  EXPECT_TRUE(item->GetProperty("isSynthesized").asBoolean());

  PlexMediaDecisionJobFake job(*item);
  EXPECT_TRUE(job.DoWork());
  EXPECT_STREQ("plexserver://abc123/library/sections/1/all/219.mp3", job.m_choosenMedia.GetPath());
}
