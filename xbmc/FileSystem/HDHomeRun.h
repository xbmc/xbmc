#pragma once
#include "IDirectory.h"
#include "../DynamicDll.h"
#include "../lib/libhdhomerun/hdhomerun.h"


class DllHdHomeRunInterface
{
public:
  virtual ~DllHdHomeRunInterface() {} 
  virtual int           discover_find_devices(uint32_t device_type, struct hdhomerun_discover_device_t result_list[], int max_count)=0;
  virtual struct hdhomerun_device_t*  device_create_from_str(const char *device_str)=0;
  virtual void          device_destroy(struct hdhomerun_device_t *hd)=0;
  virtual int           device_stream_start(struct hdhomerun_device_t *hd)=0;
  virtual uint8_t*      device_stream_recv(struct hdhomerun_device_t *hd, unsigned int max_size, unsigned int* pactual_size)=0;
  virtual void          device_stream_stop(struct hdhomerun_device_t *hd)=0;
  virtual int           device_set_tuner_channel(struct hdhomerun_device_t *hd, const char *channel)=0;
  virtual int           device_set_tuner_program(struct hdhomerun_device_t *hd, const char *program)=0;
  virtual int           device_set_tuner_from_str(struct hdhomerun_device_t *hd, const char *tuner_str)=0;
  virtual void          device_set_tuner(struct hdhomerun_device_t *hd, unsigned int tuner)=0;
  virtual int           device_get_tuner_status(struct hdhomerun_device_t *hd, struct hdhomerun_tuner_status_t *status)=0;
};

class DllHdHomeRun : public DllDynamic, public DllHdHomeRunInterface
{
  DECLARE_DLL_WRAPPER(DllHdHomeRun, Q:\\system\\hdhomerun.dll)
  DEFINE_METHOD3(int, discover_find_devices, (uint32_t p1, struct hdhomerun_discover_device_t p2[], int p3))
  DEFINE_METHOD1(struct hdhomerun_device_t*, device_create_from_str, (const char* p1))
  DEFINE_METHOD1(void, device_destroy, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD1(int, device_stream_start, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD3(uint8_t*, device_stream_recv, (struct hdhomerun_device_t* p1, unsigned int p2, unsigned int* p3))
  DEFINE_METHOD1(void, device_stream_stop, (struct hdhomerun_device_t* p1))
  DEFINE_METHOD2(int, device_set_tuner_channel, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(int, device_set_tuner_program, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(int, device_set_tuner_from_str, (struct hdhomerun_device_t *p1, const char *p2))
  DEFINE_METHOD2(void, device_set_tuner, (struct hdhomerun_device_t *p1, unsigned int p2))
  DEFINE_METHOD2(int, device_get_tuner_status, (struct hdhomerun_device_t *p1, struct hdhomerun_tuner_status_t *p2));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(hdhomerun_discover_find_devices, discover_find_devices)
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

namespace DIRECTORY
{
  class CDirectoryHomeRun : public IDirectory
  {
    public:
      CDirectoryHomeRun(void);
      virtual ~CDirectoryHomeRun(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    private:
      DllHdHomeRun m_dll;
  };
}

namespace XFILE
{
	class CFileHomeRun : public IFile  
	{
    public:
	    CFileHomeRun();
	    ~CFileHomeRun();

      virtual bool          Exists(const CURL& url)                           { return false; }
      virtual __int64	      Seek(__int64 iFilePosition, int iWhence)          { return -1; }
      virtual int	          Stat(const CURL& url, struct __stat64* buffer)    { return 0; }
      virtual __int64       GetPosition()                                     { return 0; }
      virtual __int64       GetLength()                                       { return 0; }
      
      virtual bool          Open(const CURL& url, bool bBinary);
	    virtual void          Close();
      virtual unsigned int  Read(void* lpBuf, __int64 uiBufSize);
    private:
      struct hdhomerun_device_t* m_device;
      DllHdHomeRun m_dll;
  };
}
