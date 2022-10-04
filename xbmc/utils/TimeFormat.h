/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*! \brief TIME_FORMAT enum/bitmask used for formatting time strings
 Note the use of bitmasking, e.g.
  TIME_FORMAT_HH_MM_SS = TIME_FORMAT_HH | TIME_FORMAT_MM | TIME_FORMAT_SS
 \sa StringUtils::SecondsToTimeString
 \note For InfoLabels use the equivalent value listed (bold)
  on the description of each enum value.
 \note<b>Example:</b> 3661 seconds => h=1, hh=01, m=1, mm=01, ss=01, hours=1, mins=61, secs=3661
 <p><hr>
 @skinning_v18 **[Infolabels Updated]** Added <b>secs</b>, <b>mins</b>, <b>hours</b> (total time) and **m** as possible formats for
 InfoLabels that support the definition of a time format. Examples are:
   - \link Player_SeekOffset_format `Player.SeekOffset(format)`\endlink
   - \link Player_TimeRemaining_format `Player.TimeRemaining(format)`\endlink
   - \link Player_Time_format `Player.Time(format)`\endlink
   - \link Player_Duration_format `Player.Duration(format)`\endlink
   - \link Player_FinishTime_format `Player.FinishTime(format)`\endlink
   - \link Player_StartTime_format `Player.StartTime(format)` \endlink
   - \link Player_SeekNumeric_format `Player.SeekNumeric(format)`\endlink
   - \link ListItem_Duration_format `ListItem.Duration(format)`\endlink
   - \link PVR_EpgEventDuration_format `PVR.EpgEventDuration(format)`\endlink
   - \link PVR_EpgEventElapsedTime_format `PVR.EpgEventElapsedTime(format)`\endlink
   - \link PVR_EpgEventRemainingTime_format `PVR.EpgEventRemainingTime(format)`\endlink
   - \link PVR_EpgEventSeekTime_format `PVR.EpgEventSeekTime(format)`\endlink
   - \link PVR_EpgEventFinishTime_format `PVR.EpgEventFinishTime(format)`\endlink
   - \link PVR_TimeShiftStart_format `PVR.TimeShiftStart(format)`\endlink
   - \link PVR_TimeShiftEnd_format `PVR.TimeShiftEnd(format)`\endlink
   - \link PVR_TimeShiftCur_format `PVR.TimeShiftCur(format)`\endlink
   - \link PVR_TimeShiftOffset_format `PVR.TimeShiftOffset(format)`\endlink
   - \link PVR_TimeshiftProgressDuration_format `PVR.TimeshiftProgressDuration(format)`\endlink
   - \link PVR_TimeshiftProgressEndTime `PVR.TimeshiftProgressEndTime`\endlink
   - \link PVR_TimeshiftProgressEndTime_format `PVR.TimeshiftProgressEndTime(format)`\endlink
   - \link ListItem_NextDuration_format `ListItem.NextDuration(format)` \endlink
  <p>
 */
enum TIME_FORMAT
{
  TIME_FORMAT_GUESS = 0, ///< usually used as the fallback value if the format value is empty
  TIME_FORMAT_SS = 1, ///< <b>ss</b> - seconds only
  TIME_FORMAT_MM = 2, ///< <b>mm</b> - minutes only (2-digit)
  TIME_FORMAT_MM_SS = 3, ///< <b>mm:ss</b> - minutes and seconds
  TIME_FORMAT_HH = 4, ///< <b>hh</b> - hours only (2-digit)
  TIME_FORMAT_HH_SS = 5, ///< <b>hh:ss</b> - hours and seconds (this is not particularly useful)
  TIME_FORMAT_HH_MM = 6, ///< <b>hh:mm</b> - hours and minutes
  TIME_FORMAT_HH_MM_SS = 7, ///< <b>hh:mm:ss</b> - hours, minutes and seconds
  TIME_FORMAT_XX = 8, ///<  <b>xx</b> - returns AM/PM for a 12-hour clock
  TIME_FORMAT_HH_MM_XX =
      14, ///< <b>hh:mm xx</b> - returns hours and minutes in a 12-hour clock format (AM/PM)
  TIME_FORMAT_HH_MM_SS_XX =
      15, ///< <b>hh:mm:ss xx</b> - returns hours (2-digit), minutes and seconds in a 12-hour clock format (AM/PM)
  TIME_FORMAT_H = 16, ///< <b>h</b> - hours only (1-digit)
  TIME_FORMAT_H_MM_SS = 19, ///< <b>hh:mm:ss</b> - hours, minutes and seconds
  TIME_FORMAT_H_MM_SS_XX =
      27, ///< <b>hh:mm:ss xx</b> - returns hours (1-digit), minutes and seconds in a 12-hour clock format (AM/PM)
  TIME_FORMAT_SECS = 32, ///<  <b>secs</b> - total time in seconds
  TIME_FORMAT_MINS = 64, ///<  <b>mins</b> - total time in minutes
  TIME_FORMAT_HOURS = 128, ///< <b>hours</b> - total time in hours
  TIME_FORMAT_M = 256 ///< <b>m</b> - minutes only (1-digit)
};
