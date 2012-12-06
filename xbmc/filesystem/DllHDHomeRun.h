#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DynamicDll.h"
#include "lib/libhdhomerun/hdhomerun.h"

class DllHdHomeRunInterface
{
public:
  virtual ~DllHdHomeRunInterface() {}
  virtual int           discover_find_devices_custom(uint32_t target_ip, uint32_t device_type, uint32_t device_id, struct hdhomerun_discover_device_t result_list[], int max_count)=0;
  virtual struct hdhomerun_device_t*  device_create_from_str(const char *device_str, struct hdhomerun_debug_t *dbg)=0;
  virtual void          device_destroy(struct hdhomerun_device_t *hd)=0;
  virtual int           device_stream_start(struct hdhomerun_device_t *hd)=0;
  virtual uint8_t*      device_stream_recv(struct hdhomerun_device_t *hd, size_t max_size, size_t* pactual_size)=0;
  virtual void          device_stream_stop(struct hdhomerun_device_t *hd)=0;
  virtual int           device_set_tuner_channel(struct hdhomerun_device_t *hd, const char *channel)=0;
  virtual int           device_set_tuner_program(struct hdhomerun_device_t *hd, const char *program)=0;
  virtual int           device_set_tuner_from_str(struct hdhomerun_device_t *hd, const char *tuner_str)=0;
  virtual void          device_set_tuner(struct hdhomerun_device_t *hd, unsigned int tuner)=0;
  virtual int           device_get_tuner_status(struct hdhomerun_device_t *hd, char **pstatus_str, struct hdhomerun_tuner_status_t *status)=0;
};

class DllHdHomeRun : public DllDynamic, public DllHdHomeRunInterface
{
  DECLARE_DLL_WRAPPER(DllHdHomeRun, DLL_PATH_LIBHDHOMERUN)
  DEFINE_METHOD5(int, discover_find_devices_custom, (uint32_t p1, uint32_t p2, uint32_t p3, struct hdhomerun_discover_device_t p4[], int p5))
  DEFINE_METHOD2(struct hdhomerun_device_t*, device_create_from_str, (const char* p1, struct hdhomerun_debug_t *p2))
  DEFINE_METHOD1(void, device_destroy, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD1(int, device_stream_start, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD3(uint8_t*, device_stream_recv, (struct hdhomerun_device_t* p1, size_t p2, size_t* p3))
  DEFINE_METHOD1(void, device_stream_stop, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD2(int, device_set_tuner_channel, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(int, device_set_tuner_program, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(int, device_set_tuner_from_str, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(void, device_set_tuner, (struct hdhomerun_device_t *p1, unsigned int p2))
  DEFINE_METHOD3(int, device_get_tuner_status, (struct hdhomerun_device_t *p1, char **p2, struct hdhomerun_tuner_status_t *p3));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(hdhomerun_discover_find_devices_custom, discover_find_devices_custom)
    RESOLVE_METHOD_RENAME(hdhomerun_device_create_from_str, device_create_from_str)
    RESOLVE_METHOD_RENAME(hdhomerun_device_destroy, device_destroy)
    RESOLVE_METHOD_RENAME(hdhomerun_device_stream_start, device_stream_start)
    RESOLVE_METHOD_RENAME(hdhomerun_device_stream_recv, device_stream_recv)
    RESOLVE_METHOD_RENAME(hdhomerun_device_stream_stop, device_stream_stop)
    RESOLVE_METHOD_RENAME(hdhomerun_device_set_tuner_channel, device_set_tuner_channel)
    RESOLVE_METHOD_RENAME(hdhomerun_device_set_tuner_program, device_set_tuner_program)
    RESOLVE_METHOD_RENAME(hdhomerun_device_set_tuner_from_str, device_set_tuner_from_str)
    RESOLVE_METHOD_RENAME(hdhomerun_device_set_tuner, device_set_tuner)
    RESOLVE_METHOD_RENAME(hdhomerun_device_get_tuner_status, device_get_tuner_status)
  END_METHOD_RESOLVE()
};

