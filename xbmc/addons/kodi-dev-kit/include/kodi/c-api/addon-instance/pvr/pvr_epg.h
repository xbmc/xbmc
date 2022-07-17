/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_EPG_H
#define C_API_ADDONINSTANCE_PVR_EPG_H

#include "pvr_defines.h"

#include <time.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 4 - PVR EPG
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT enum EPG_EVENT_CONTENTMASK (and sub types)
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief **EPG entry content event types.**\n
  /// These ID's come from the DVB-SI EIT table "content descriptor"
  /// Also known under the name "E-book genre assignments".
  ///
  /// See [ETSI EN 300 468 V1.14.1 (2014-05)](https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.14.01_60/en_300468v011401p.pdf)
  /// about.
  ///
  /// Values used by this functions:
  /// - @ref kodi::addon::PVREPGTag::SetGenreType()
  /// - @ref kodi::addon::PVREPGTag::SetGenreSubType()
  /// - @ref kodi::addon::PVRRecording::SetGenreType()
  /// - @ref kodi::addon::PVRRecording::SetGenreSubType()
  ///
  /// Following types are listed here:
  /// | emum Type | Description
  /// |-----------|--------------------
  /// | @ref EPG_EVENT_CONTENTMASK | EPG entry main content to use.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_MOVIEDRAMA event types for sub type of <b>"Movie/Drama"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS event types for sub type of <b>"News/Current affairs"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_SHOW | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SHOW event types for sub type of <b>"Show/Game show"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_SPORTS | @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SPORTS event types for sub type of <b>"Sports"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_CHILDRENYOUTH event types for sub type of <b>"Children's/Youth programmes"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE event types for sub type of <b>"Music/Ballet/Dance"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_ARTSCULTURE event types for sub type of <b>"Arts/Culture (without music)"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS event types for sub type of <b>"Social/Political issues/Economics"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE event types for sub type of <b>"Education/Science/Factual topics"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_LEISUREHOBBIES event types for sub type of <b>"Leisure hobbies"</b>.
  /// | @ref EPG_EVENT_CONTENTSUBMASK_SPECIAL | EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SPECIAL event types for sub type of <b>"Special characteristics"</b>.
  ///@{

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry main content to use.
  ///
  ///@{
  typedef enum EPG_EVENT_CONTENTMASK
  {
    /// @brief __0x00__ : Undefined content mask entry.
    EPG_EVENT_CONTENTMASK_UNDEFINED = 0x00,

    /// @brief __0x10__ : Movie/Drama.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA about related sub types.
    EPG_EVENT_CONTENTMASK_MOVIEDRAMA = 0x10,

    /// @brief __0x20__ : News/Current affairs.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS about related sub types.
    EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS = 0x20,

    /// @brief __0x30__ : Show/Game show.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_SHOW about related sub types.
    EPG_EVENT_CONTENTMASK_SHOW = 0x30,

    /// @brief __0x40__ : Sports.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_SPORTS about related sub types.
    EPG_EVENT_CONTENTMASK_SPORTS = 0x40,

    /// @brief __0x50__ : Children's/Youth programmes.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH about related sub types.
    EPG_EVENT_CONTENTMASK_CHILDRENYOUTH = 0x50,

    /// @brief __0x60__ : Music/Ballet/Dance.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE about related sub types.
    EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE = 0x60,

    /// @brief __0x70__ : Arts/Culture (without music).\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE about related sub types.
    EPG_EVENT_CONTENTMASK_ARTSCULTURE = 0x70,

    /// @brief __0x80__ : Social/Political issues/Economics.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS about related sub types.
    EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS = 0x80,

    /// @brief __0x90__ : Education/Science/Factual topics.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE about related sub types.
    EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE = 0x90,

    /// @brief __0xA0__ : Leisure hobbies.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES about related sub types.
    EPG_EVENT_CONTENTMASK_LEISUREHOBBIES = 0xA0,

    /// @brief __0xB0__ : Special characteristics.\n
    /// \n
    /// See @ref EPG_EVENT_CONTENTSUBMASK_SPECIAL about related sub types.
    EPG_EVENT_CONTENTMASK_SPECIAL = 0xB0,

    /// @brief __0xF0__ User defined.
    EPG_EVENT_CONTENTMASK_USERDEFINED = 0xF0,

    /// @brief Used to override standard genre types with a own name about.\n
    /// \n
    /// Set to this value @ref EPG_GENRE_USE_STRING on following places:
    /// - @ref kodi::addon::PVREPGTag::SetGenreType()
    /// - @ref kodi::addon::PVREPGTag::SetGenreSubType()
    /// - @ref kodi::addon::PVRRecording::SetGenreType()
    /// - @ref kodi::addon::PVRRecording::SetGenreSubType()
    ///
    /// @warning Value here is not a [ETSI EN 300 468 V1.14.1 (2014-05)](https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.14.01_60/en_300468v011401p.pdf)
    /// conform.
    ///
    /// @note This is a own Kodi definition to set that genre is given by own
    /// string. Used on @ref kodi::addon::PVREPGTag::SetGenreDescription() and
    /// @ref kodi::addon::PVRRecording::SetGenreDescription()
    EPG_GENRE_USE_STRING = 0x100
  } EPG_EVENT_CONTENTMASK;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_MOVIEDRAMA event
  /// types for sub type of <b>"Movie/Drama"</b>.
  ///
  ///@{
  typedef enum EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA
  {
    /// @brief __0x0__ : Movie/drama (general).
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_GENERAL = 0x0,

    /// @brief __0x1__ : Detective/thriller.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_DETECTIVE_THRILLER = 0x1,

    /// @brief __0x2__ : Adventure/western/war.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_ADVENTURE_WESTERN_WAR = 0x2,

    /// @brief __0x3__ : Science fiction/fantasy/horror.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_SCIENCEFICTION_FANTASY_HORROR = 0x3,

    /// @brief __0x4__ : Comedy.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_COMEDY = 0x4,

    /// @brief __0x5__ : Soap/melodrama/folkloric.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_SOAP_MELODRAMA_FOLKLORIC = 0x5,

    /// @brief __0x6__ : Romance.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_ROMANCE = 0x6,

    /// @brief __0x7__ : Serious/classical/religious/historical movie/drama.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_SERIOUS_CLASSICAL_RELIGIOUS_HISTORICAL = 0x7,

    /// @brief __0x8__ : Adult movie/drama.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_ADULT = 0x8,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS event
  /// types for sub type of <b>"News/Current affairs"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS
  {
    /// @brief __0x0__ : News/current affairs (general).
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_GENERAL = 0x0,

    /// @brief __0x1__ : News/weather report.
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_WEATHER = 0x1,

    /// @brief __0x2__ : News magazine.
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_MAGAZINE = 0x2,

    /// @brief __0x3__ : Documentary.
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_DOCUMENTARY = 0x3,

    /// @brief __0x4__ : Discussion/interview/debate
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_DISCUSSION_INTERVIEW_DEBATE = 0x4,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SHOW event
  /// types for sub type of <b>"Show/Game show"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_SHOW
  {
    /// @brief __0x0__ : Show/game show (general).
    EPG_EVENT_CONTENTSUBMASK_SHOW_GENERAL = 0x0,

    /// @brief __0x1__ : Game show/quiz/contest.
    EPG_EVENT_CONTENTSUBMASK_SHOW_GAMESHOW_QUIZ_CONTEST = 0x1,

    /// @brief __0x2__ : Variety show.
    EPG_EVENT_CONTENTSUBMASK_SHOW_VARIETY_SHOW = 0x2,

    /// @brief __0x3__ : Talk show.
    EPG_EVENT_CONTENTSUBMASK_SHOW_TALK_SHOW = 0x3,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_SHOW_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_SHOW;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SPORTS event
  /// types for sub type of <b>"Sports"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_SPORTS
  {
    /// @brief __0x0__ : Sports (general).
    EPG_EVENT_CONTENTSUBMASK_SPORTS_GENERAL = 0x0,

    /// @brief __0x1__ : Special events (Olympic Games, World Cup, etc.).
    EPG_EVENT_CONTENTSUBMASK_SPORTS_OLYMPICGAMES_WORLDCUP = 0x1,

    /// @brief __0x2__ : Sports magazines.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_SPORTS_MAGAZINES = 0x2,

    /// @brief __0x3__ : Football/soccer.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_FOOTBALL_SOCCER = 0x3,

    /// @brief __0x4__ : Tennis/squash.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_TENNIS_SQUASH = 0x4,

    /// @brief __0x5__ : Team sports (excluding football).
    EPG_EVENT_CONTENTSUBMASK_SPORTS_TEAMSPORTS = 0x5,

    /// @brief __0x6__ : Athletics.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_ATHLETICS = 0x6,

    /// @brief __0x7__ : Motor sport.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_MOTORSPORT = 0x7,

    /// @brief __0x8__ : Water sport.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_WATERSPORT = 0x8,

    /// @brief __0x9__ : Winter sports.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_WINTERSPORTS = 0x9,

    /// @brief __0xA__ : Equestrian.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_EQUESTRIAN = 0xA,

    /// @brief __0xB__ : Martial sports.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_MARTIALSPORTS = 0xB,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_SPORTS_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_SPORTS;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_CHILDRENYOUTH event
  /// types for sub type of <b>"Children's/Youth programmes"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH
  {
    /// @brief __0x0__ : Children's/youth programmes (general).
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_GENERAL = 0x0,

    /// @brief __0x1__ : Pre-school children's programmes.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_PRESCHOOL_CHILDREN = 0x1,

    /// @brief __0x2__ : Entertainment programmes for 6 to 14.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_ENTERTAIN_6TO14 = 0x2,

    /// @brief __0x3__ : Entertainment programmes for 10 to 16.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_ENTERTAIN_10TO16 = 0x3,

    /// @brief __0x4__ : Informational/educational/school programmes.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_INFORMATIONAL_EDUCATIONAL_SCHOOL = 0x4,

    /// @brief __0x5__ : Cartoons/puppets.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_CARTOONS_PUPPETS = 0x5,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE event
  /// types for sub type of <b>"Music/Ballet/Dance"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE
  {
    /// @brief __0x0__ : Music/ballet/dance (general).
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_GENERAL = 0x0,

    /// @brief __0x1__ : Rock/pop.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_ROCKPOP = 0x1,

    /// @brief __0x2__ : Serious music/classical music.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_SERIOUSMUSIC_CLASSICALMUSIC = 0x2,

    /// @brief __0x3__ : Folk/traditional music.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_FOLK_TRADITIONAL_MUSIC = 0x3,

    /// @brief __0x4__ : Jazz.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_JAZZ = 0x4,

    /// @brief __0x5__ : Musical/opera.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_MUSICAL_OPERA = 0x5,

    /// @brief __0x6__ : Ballet.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_BALLET = 0x6,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_ARTSCULTURE event
  /// types for sub type of <b>"Arts/Culture (without music)"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE
  {
    /// @brief __0x0__ : Arts/culture (without music, general).
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_GENERAL = 0x0,

    /// @brief __0x1__ : Performing arts.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_PERFORMINGARTS = 0x1,

    /// @brief __0x2__ : Fine arts.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_FINEARTS = 0x2,

    /// @brief __0x3__ : Religion.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_RELIGION = 0x3,

    /// @brief __0x4__ : Popular culture/traditional arts.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_POPULARCULTURE_TRADITIONALARTS = 0x4,

    /// @brief __0x5__ : Literature.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_LITERATURE = 0x5,

    /// @brief __0x6__ : Film/cinema.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_FILM_CINEMA = 0x6,

    /// @brief __0x7__ : Experimental film/video.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_EXPERIMENTALFILM_VIDEO = 0x7,

    /// @brief __0x8__ : Broadcasting/press.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_BROADCASTING_PRESS = 0x8,

    /// @brief __0x9__ : New media.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_NEWMEDIA = 0x9,

    /// @brief __0xA__ : Arts/culture magazines.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_ARTS_CULTUREMAGAZINES = 0xA,

    /// @brief __0xB__ : Fashion.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_FASHION = 0xB,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS event
  /// types for sub type of <b>"Social/Political issues/Economics"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS
  {
    /// @brief __0x0__ : Social/political issues/economics (general).
    EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS_GENERAL = 0x0,

    /// @brief __0x1__ : Magazines/reports/documentary.
    EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS_MAGAZINES_REPORTS_DOCUMENTARY = 0x1,

    /// @brief __0x2__ : Economics/social advisory.
    EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS_ECONOMICS_SOCIALADVISORY = 0x2,

    /// @brief __0x3__ : Remarkable people.
    EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS_REMARKABLEPEOPLE = 0x3,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE event
  /// types for sub type of <b>"Education/Science/Factual topics"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE
  {
    /// @brief __0x0__ : Education/science/factual topics (general).
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_GENERAL = 0x0,

    /// @brief __0x1__ : Nature/animals/environment.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_NATURE_ANIMALS_ENVIRONMENT = 0x1,

    /// @brief __0x2__ : Technology/natural sciences.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_TECHNOLOGY_NATURALSCIENCES = 0x2,

    /// @brief __0x3__ : Medicine/physiology/psychology.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_MEDICINE_PHYSIOLOGY_PSYCHOLOGY = 0x3,

    /// @brief __0x4__ : Foreign countries/expeditions.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_FOREIGNCOUNTRIES_EXPEDITIONS = 0x4,

    /// @brief __0x5__ : Social/spiritual sciences.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_SOCIAL_SPIRITUALSCIENCES = 0x5,

    /// @brief __0x6__ : Further education.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_FURTHEREDUCATION = 0x6,

    /// @brief __0x7__ : Languages.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_LANGUAGES = 0x7,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_LEISUREHOBBIES event
  /// types for sub type of <b>"Leisure hobbies"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES
  {
    /// @brief __0x0__ : Leisure hobbies (general) .
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_GENERAL = 0x0,

    /// @brief __0x1__ : Tourism/travel.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_TOURISM_TRAVEL = 0x1,

    /// @brief __0x2__ : Handicraft.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_HANDICRAFT = 0x2,

    /// @brief __0x3__ : Motoring.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_MOTORING = 0x3,

    /// @brief __0x4__ : Fitness and health.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_FITNESSANDHEALTH = 0x4,

    /// @brief __0x5__ : Cooking.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_COOKING = 0x5,

    /// @brief __0x6__ : Advertisement/shopping.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_ADVERTISEMENT_SHOPPING = 0x6,

    /// @brief __0x7__ : Gardening.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_GARDENING = 0x7,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT
  /// @brief EPG entry sub content to @ref EPG_EVENT_CONTENTMASK_SPECIAL event
  /// types for sub type of <b>"Special characteristics"</b>.
  ///
  typedef enum EPG_EVENT_CONTENTSUBMASK_SPECIAL
  {
    /// @brief __0x0__ : Special characteristics / Original language (general).
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_GENERAL = 0x0,

    /// @brief __0x1__ : Black and white.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_BLACKANDWHITE = 0x1,

    /// @brief __0x2__ : Unpublished.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_UNPUBLISHED = 0x2,

    /// @brief __0x3__ : Live broadcast.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_LIVEBROADCAST = 0x3,

    /// @brief __0x4__ : Plano-stereoscopic.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_PLANOSTEREOSCOPIC = 0x4,

    /// @brief __0x5__ : Local or regional.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_LOCALORREGIONAL = 0x5,

    /// @brief __0xF__ : User defined.
    EPG_EVENT_CONTENTSUBMASK_SPECIAL_USERDEFINED = 0xF
  } EPG_EVENT_CONTENTSUBMASK_SPECIAL;
  //----------------------------------------------------------------------------

  ///@}

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief Separator to use in strings containing different tokens, for example
  /// writers, directors, actors of an event.
  ///
#define EPG_STRING_TOKEN_SEPARATOR ","
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_epg_EPG_TAG_FLAG enum EPG_TAG_FLAG
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief <b>Bit field of independent flags associated with the EPG entry.</b>\n
  /// Values used by @ref kodi::addon::PVREPGTag::SetFlags().
  ///
  /// Here's example about the use of this:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVREPGTag tag;
  /// tag.SetFlags(EPG_TAG_FLAG_IS_SERIES | EPG_TAG_FLAG_IS_NEW);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum EPG_TAG_FLAG
  {
    /// @brief __0000 0000__ : Nothing special to say about this entry.
    EPG_TAG_FLAG_UNDEFINED = 0,

    /// @brief __0000 0001__ : This EPG entry is part of a series.
    EPG_TAG_FLAG_IS_SERIES = (1 << 0),

    /// @brief __0000 0010__ : This EPG entry will be flagged as new.
    EPG_TAG_FLAG_IS_NEW = (1 << 1),

    /// @brief __0000 0100__ : This EPG entry will be flagged as a premiere.
    EPG_TAG_FLAG_IS_PREMIERE = (1 << 2),

    /// @brief __0000 1000__ : This EPG entry will be flagged as a finale.
    EPG_TAG_FLAG_IS_FINALE = (1 << 3),

    /// @brief __0001 0000__ : This EPG entry will be flagged as live.
    EPG_TAG_FLAG_IS_LIVE = (1 << 4),
  } EPG_TAG_FLAG;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief Special PVREPGTag::SetUniqueBroadcastId value
  ///
  /// Special @ref kodi::addon::PVREPGTag::SetUniqueBroadcastId() value to
  /// indicate that a tag has not a valid EPG event uid.
  ///
#define EPG_TAG_INVALID_UID 0
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief Special @ref kodi::addon::PVREPGTag::SetSeriesNumber(), @ref kodi::addon::PVREPGTag::SetEpisodeNumber()
  /// and @ref kodi::addon::PVREPGTag::SetEpisodePartNumber() value to indicate
  /// it is not to be used.
  ///
#define EPG_TAG_INVALID_SERIES_EPISODE -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief Timeframe value for use with @ref kodi::addon::CInstancePVRClient::SetEPGTimeFrame()
  /// function to indicate "no timeframe".
  ///
#define EPG_TIMEFRAME_UNLIMITED -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_epg_EPG_EVENT_STATE enum EPG_EVENT_STATE
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief **EPG event states.**\n
  /// Used with @ref kodi::addon::CInstancePVRClient::EpgEventStateChange()
  /// callback.
  ///
  ///@{
  typedef enum EPG_EVENT_STATE
  {
    /// @brief __0__ : Event created.
    EPG_EVENT_CREATED = 0,

    /// @brief __1__ : Event updated.
    EPG_EVENT_UPDATED = 1,

    /// @brief __2__ : Event deleted.
    EPG_EVENT_DELETED = 2,
  } EPG_EVENT_STATE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on channel group member.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVREPGTag for description of values.
   */
  typedef struct EPG_TAG
  {
    unsigned int iUniqueBroadcastId;
    unsigned int iUniqueChannelId;
    const char* strTitle;
    time_t startTime;
    time_t endTime;
    const char* strPlotOutline;
    const char* strPlot;
    const char* strOriginalTitle;
    const char* strCast;
    const char* strDirector;
    const char* strWriter;
    int iYear;
    const char* strIMDBNumber;
    const char* strIconPath;
    int iGenreType;
    int iGenreSubType;
    const char* strGenreDescription;
    const char* strFirstAired;
    int iParentalRating;
    const char* strParentalRatingCode;
    int iStarRating;
    int iSeriesNumber;
    int iEpisodeNumber;
    int iEpisodePartNumber;
    const char* strEpisodeName;
    unsigned int iFlags;
    const char* strSeriesLink;
  } EPG_TAG;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_EPG_H */
