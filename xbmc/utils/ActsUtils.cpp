/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>


#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "utils/StringUtils.h"
#include "utils/ActsUtils.h"


bool acts_present()
{
  static int has_acts = -1;
  std::string valstr;
  if(has_acts==-1){
  	if(SysfsUtils::GetString("/system/etc/omx_codec.xml", valstr) == 0){
  		if(valstr.find("libOMX.Action.Video.Decoder.so") != std::string::npos){
  			has_acts = 1;
  			CLog::Log(LOGNOTICE, "Acts device detected");
  		}else{
  			has_acts = 0;
  		}
  	}
  }
  return has_acts == 1;
}

bool acts_permissions()
{  
  if (!acts_present())
    return false;
  
  return true;
}
bool acts_support_hevc()
{
  if (SysfsUtils::Has("/system/lib/vd_hev.so") ||  SysfsUtils::Has("/system/lib/vd_h265.so"))
	{
		 return true;
  }
  return false;
}
bool acts_support_h264()
{
  if (SysfsUtils::Has("/system/lib/vd_h264.so") ||  SysfsUtils::Has("/system/lib/7059plus1.so"))
	{
		 return true;
  }
  return false;
}

bool acts_support_rv34()
{
	if (SysfsUtils::Has("/system/lib/vd_rv34.so") || SysfsUtils::Has("/system/lib/7059plus2.so"))
	{
		 return true;
  }
  return false;
}


bool acts_support_mpeg4()
{
	if (SysfsUtils::Has("/system/lib/vd_xvid.so") || SysfsUtils::Has("/system/lib/7059plus3.so"))
	{
		 return true;
  }
  return false;
}

bool acts_support_mpeg2()
{
	if (SysfsUtils::Has("/system/lib/vd_mpeg.so")|| SysfsUtils::Has("/system/lib/7059plus4.so"))
	{
		 return true;
  }
  return false;
}


bool acts_support_vc1()
{
	if (SysfsUtils::Has("/system/lib/vd_vc1.so") || SysfsUtils::Has("/system/lib/7059plus5.so"))
	{
		 return true;
  }
  return false;
}

bool acts_support_msm4v3()
{
	if (SysfsUtils::Has("/system/lib/vd_msm4.so"))
	{
		 return true;
  }
  std::string valstr;
  if(SysfsUtils::GetString("/system/etc/acts_codec.xml", valstr) == 0){
  	if(valstr.find("use_acts_msm4") != std::string::npos){
  		return true;
  	}
	}
  return false;
}

bool acts_support_h263()
{
	if (SysfsUtils::Has("/system/lib/vd_h263.so"))
	{
		 return true;
  }
}

bool acts_support_flv1()
{
	if (SysfsUtils::Has("/system/lib/vd_flv1.so"))
	{
		 return true;
  }
  return false;
}
bool acts_support_vp6()
{
  if (SysfsUtils::Has("/system/lib/vd_vp6.so"))
	{
		 return true;
  }
  return false;
}
bool acts_support_vp8()
{
  if (SysfsUtils::Has("/system/lib/vd_vp8.so"))
	{
		 return true;
  }
  return false;
}
bool acts_support_vp9()
{
  if (SysfsUtils::Has("/system/lib/vd_vp9.so") || SysfsUtils::Has("/system/lib/vd_vp9_hw.so"))
	{
		 return true;
  }
  return false;
}

bool acts_support_avs()
{
	if (SysfsUtils::Has("/system/lib/vd_avs.so") || SysfsUtils::Has("/system/lib/vd_avs_hw.so"))
	{
		 return true;
  }
  return false;
}
bool acts_support_wmv2(){
	std::string valstr;
  if(SysfsUtils::GetString("/system/etc/acts_codec.xml", valstr) == 0){
  	if(valstr.find("use_acts_wmv2") != std::string::npos){
  		if (SysfsUtils::Has("/system/lib/vd_wmv2.so") ){
		 		return true;
  		}
  	}
	}
  return false;
}


