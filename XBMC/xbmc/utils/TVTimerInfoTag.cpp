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
  * CTVTimerInfoTag is part of the PVRManager to support scheduled recordings.
  *
  * The timer information tag holds data about current programmed timers for
  * the PVRManager. It is possible to create timers directly based upon
  * a EPG entry by giving the EPG information tag or as instant timer
  * on currently tuned channel, or give a blank tag to modify later.
  *
  * With exception of the blank one, the tag can easily and unmodified added
  * by the PVRManager function "bool AddTimer(const CFileItem &item)" to
  * the backend server.
  *
  * It is a also CVideoInfoTag but only used for information usage, the
  * filename inside the tag is for reference only and gives the index number
  * of the tag reported by the PVR backend and can not be played!
  *
  *
  * USED SETUP VARIABLES:
  *
  * ------------- Name -------|---Type--|-default-|--Description-----
  * pvr.instantrecordtime     = Integer = 180     = Length of a instant timer in minutes
  * pvr.defaultpriority       = Integer = 50      = Default Priority
  * pvr.defaultlifetime       = Integer = 99      = Liftime of the timer in days
  * pvr.marginstart           = Integer = 2       = Minutes to start record earlier
  * pvr.marginstop            = Integer = 10      = Minutes to stop record later
  *
  *
  * TODO:
  * Nothing in the moment. Any ideas?
  *
  */

#include "stdafx.h"
#include "TVTimerInfoTag.h"
#include "TVEPGInfoTag.h"
#include "GUISettings.h"
#include "PVRManager.h"
#include "Util.h"


/**
 * Create a blank unmodified timer tag
 */
CTVTimerInfoTag::CTVTimerInfoTag() {
    Reset();
}

/**
 * Creates a instant timer on current date and channel if "bool Init"
 * is set one hour later as now otherwise a blank CTVTimerInfoTag is
 * given.
 * \param bool Init             = Initialize as instant timer if set
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
CTVTimerInfoTag::CTVTimerInfoTag(bool Init) {

    Reset();

    /* Check if instant flag is set otherwise return */
    if (!Init) {
        CLog::Log(LOGERROR, "CTVTimerInfoTag: Can't initialize tag, Init flag not set!");
        return;
    }

    /* Get setup variables */
    int rectime     = g_guiSettings.GetInt("pvrrecord.instantrecordtime");
    int defprio     = g_guiSettings.GetInt("pvrrecord.defaultpriority");
    int deflifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
    if (!rectime) {
        rectime     = 180; /* Default 180 minutes */
    }
    if (!defprio) {
        defprio     = 50;  /* Default */
    }
    if (!deflifetime) {
        deflifetime = 99;  /* Default 99 days */
    }

    /* Set default timer */
    m_Index         = -1;
    m_Active        = true;
    //if (CPVRManager::GetInstance()->IsPlayingTV()) {
    //    m_Radio = false;
    //    m_channelNum = CPVRManager::GetInstance()->GetCurrentChannel(false);
    //    m_clientNum = CPVRManager::GetInstance()->GetClientChannelNumber(m_channelNum, false);
    //}
    //else if (CPVRManager::GetInstance()->IsPlayingRadio()) {
    //    m_Radio = true;
    //    m_channelNum = CPVRManager::GetInstance()->GetCurrentChannel(true);
    //    m_clientNum = CPVRManager::GetInstance()->GetClientChannelNumber(m_channelNum, true);
    //}
    //else {
    //    m_Radio = false;
    //    m_channelNum = 1;
    //    m_clientNum = CPVRManager::GetInstance()->GetClientChannelNumber(m_channelNum, false);
    //}
    //if (m_channelNum == -1) m_channelNum = 1;
    //m_strChannel    = CPVRManager::GetInstance()->GetNameForChannel(m_channelNum, m_Radio);
    m_strTitle      = m_strChannel;

    /* Calculate start/stop times */
    CDateTime time  = CDateTime::GetCurrentDateTime();
    m_StartTime     = time;
    m_StopTime      = time + CDateTimeSpan(0, rectime / 60, rectime % 60, 0);   /* Add recording time */

    /* Set priority and lifetime */
    m_Priority      = defprio;
    m_Lifetime      = deflifetime;

    /* Generate summary string */
    m_Summary.Format("%s %s %s %s %s", m_StartTime.GetAsLocalizedDate()
                                     , g_localizeStrings.Get(18078)
                                     , m_StartTime.GetAsLocalizedTime("",false)
                                     , g_localizeStrings.Get(18079)
                                     , m_StopTime.GetAsLocalizedTime("",false));

    m_strFileNameAndPath = "timer://new"; /* Unused only for reference */

    return;
}

/**
 * Create Timer based upon an TVEPGInfoTag
 * \param const CFileItem& item = reference to CTVEPGInfoTag class
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
CTVTimerInfoTag::CTVTimerInfoTag(const CFileItem& item) {

    Reset();

    ///* Is file item a CTVEPGInfoTag ? */
    //if (!item.IsTVEPG()) {
    //    CLog::Log(LOGERROR, "CTVTimerInfoTag: Can't initialize tag, no EPGInfoTag given!");
    //    return;
    //}

    //const CTVEPGInfoTag* tag = item.GetTVEPGInfoTag();

    ///* Check epg end date is in the future */
    //if (tag->m_endTime < CDateTime::GetCurrentDateTime()) {
    //    CLog::Log(LOGERROR, "CTVTimerInfoTag: Can't initialize tag, EPGInfoTag is in the past!");
    //    return;
    //}

    ///* Get setup variables */
    //int defprio     = g_guiSettings.GetInt("pvrrecord.defaultpriority");
    //int deflifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
    //int marginstart = g_guiSettings.GetInt("pvrrecord.marginstart");
    //int marginstop  = g_guiSettings.GetInt("pvrrecord.marginstop");
    //if (!defprio) {
    //    defprio     = 50;  /* Default */
    //}
    //if (!deflifetime) {
    //    deflifetime = 99;  /* Default 99 days */
    //}
    //if (!deflifetime) {
    //    marginstart = 2;   /* Default start 2 minutes earlier */
    //}
    //if (!deflifetime) {
    //    marginstop  = 10;  /* Default stop 10 minutes later */
    //}

    ///* Set timer based on EPG entry */
    //m_Index         = -1;
    //m_Active        = true;
    //m_channelNum    = tag->m_channelNum;
    ///*m_clientNum     = CPVRManager::GetInstance()->GetClientChannelNumber(m_channelNum, tag->m_isRadio);
    //m_strChannel    = CPVRManager::GetInstance()->GetNameForChannel(m_channelNum, tag->m_isRadio);*/
    //m_strTitle      = tag->m_strTitle;
    //if (m_strTitle.IsEmpty()) {
    //    m_strTitle  = m_strChannel;
    //}

    ///* Calculate start/stop times */
    //m_StartTime     = tag->m_startTime - CDateTimeSpan(0, marginstart / 60, marginstart % 60, 0);
    //m_StopTime      = tag->m_endTime  + CDateTimeSpan(0, marginstop / 60, marginstop % 60, 0);

    ///* Set priority and lifetime */
    //m_Priority      = defprio;
    //m_Lifetime      = deflifetime;

    ///* Generate summary string */
    //m_Summary.Format("%s %s %s %s %s", m_StartTime.GetAsLocalizedDate()
    //                                 , g_localizeStrings.Get(18078)
    //                                 , m_StartTime.GetAsLocalizedTime("",false)
    //                                 , g_localizeStrings.Get(18079)
    //                                 , m_StopTime.GetAsLocalizedTime("",false));

    //m_strFileNameAndPath = "timer://new"; /* Unused only for reference */

    return;
}

bool CTVTimerInfoTag::operator ==(const CTVTimerInfoTag& right) const {

    if (this == &right) return true;

    return (m_Index                 == right.m_Index &&
            m_Active                == right.m_Active &&
            m_Summary               == right.m_Summary &&
            m_channelNum            == right.m_channelNum &&
            m_clientNum             == right.m_clientNum &&
            m_Radio                 == right.m_Radio &&
            m_strChannel            == right.m_strChannel &&
            m_Repeat                == right.m_Repeat &&
            m_StartTime             == right.m_StartTime &&
            m_StopTime              == right.m_StopTime &&
            m_FirstDay              == right.m_FirstDay &&
            m_Repeat_Mon            == right.m_Repeat_Mon &&
            m_Repeat_Tue            == right.m_Repeat_Tue &&
            m_Repeat_Wed            == right.m_Repeat_Wed &&
            m_Repeat_Thu            == right.m_Repeat_Thu &&
            m_Repeat_Fri            == right.m_Repeat_Fri &&
            m_Repeat_Sat            == right.m_Repeat_Sat &&
            m_Repeat_Sun            == right.m_Repeat_Sun &&
            m_recStatus             == right.m_recStatus &&
            m_Priority              == right.m_Priority &&
            m_Lifetime              == right.m_Lifetime &&
            m_strFileNameAndPath    == right.m_strFileNameAndPath &&
            m_strTitle              == right.m_strTitle);
}

bool CTVTimerInfoTag::operator !=(const CTVTimerInfoTag& right) const {

    if (this == &right) return false;

    return (m_Index                 != right.m_Index &&
            m_Active                != right.m_Active &&
            m_Summary               != right.m_Summary &&
            m_channelNum            != right.m_channelNum &&
            m_clientNum             != right.m_clientNum &&
            m_Radio                 != right.m_Radio &&
            m_strChannel            != right.m_strChannel &&
            m_Repeat                != right.m_Repeat &&
            m_StartTime             != right.m_StartTime &&
            m_StopTime              != right.m_StopTime &&
            m_FirstDay              != right.m_FirstDay &&
            m_Repeat_Mon            != right.m_Repeat_Mon &&
            m_Repeat_Tue            != right.m_Repeat_Tue &&
            m_Repeat_Wed            != right.m_Repeat_Wed &&
            m_Repeat_Thu            != right.m_Repeat_Thu &&
            m_Repeat_Fri            != right.m_Repeat_Fri &&
            m_Repeat_Sat            != right.m_Repeat_Sat &&
            m_Repeat_Sun            != right.m_Repeat_Sun &&
            m_recStatus             != right.m_recStatus &&
            m_Priority              != right.m_Priority &&
            m_Lifetime              != right.m_Lifetime &&
            m_strFileNameAndPath    != right.m_strFileNameAndPath &&
            m_strTitle              != right.m_strTitle);
}

/**
 * Initialize blank CTVTimerInfoTag
 */
void CTVTimerInfoTag::Reset() {

    m_Index         = -1;
    m_Active        = false;

    m_Summary       = "";

    m_channelNum    = -1;
    m_clientNum     = -1;
    m_Radio         = false;
    m_strChannel    = "";

    m_Repeat        = false;
    m_StartTime     = NULL;
    m_StopTime      = NULL;
    m_FirstDay      = NULL;
    m_iStartTime    = NULL;
    m_iStopTime     = NULL;
    m_iFirstDay     = NULL;
    m_Repeat_Mon    = false;
    m_Repeat_Tue    = false;
    m_Repeat_Wed    = false;
    m_Repeat_Thu    = false;
    m_Repeat_Fri    = false;
    m_Repeat_Sat    = false;
    m_Repeat_Sun    = false;

    m_recStatus     = false;

    m_Priority      = -1;
    m_Lifetime      = -1;

    m_strFileNameAndPath = "";

    CVideoInfoTag::Reset();
}
