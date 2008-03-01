#pragma once

typedef struct cmyth_ringbuf   *cmyth_ringbuf_t;
typedef struct cmyth_conn      *cmyth_conn_t;
typedef struct cmyth_recorder  *cmyth_recorder_t;
typedef struct cmyth_proginfo  *cmyth_proginfo_t;
typedef struct cmyth_proglist  *cmyth_proglist_t;
typedef struct cmyth_file      *cmyth_file_t;
typedef struct cmyth_database  *cmyth_database_t;

class DllLibCMyth;

namespace XFILE
{

class CCMythSession
  : private CThread
{
public:
  static CCMythSession* AquireSession(const CURL& url);
  static void           ReleaseSession(CCMythSession*);
  static void           CheckIdle();

  class IEventListener
  {
  public:
    virtual ~IEventListener() {};
    virtual void OnEvent(int event, const std::string& data)=0;
  };

  bool             SetListener(IEventListener *listener);
  cmyth_conn_t     GetControl();
  cmyth_database_t GetDatabase();
  DllLibCMyth*     GetLibrary() { return m_dll; }

  bool             ProgramToTag(cmyth_proginfo_t info, CVideoInfoTag* tag);
private:
  CCMythSession(const CURL& url);
  ~CCMythSession();

  virtual void Process();

  bool             CanSupport(const CURL& url);
  void             Disconnect();

  IEventListener*  m_listener;
  cmyth_conn_t     m_control;
  cmyth_conn_t     m_event;
  cmyth_database_t m_database;
  CStdString       m_hostname;
  CStdString       m_username;
  CStdString       m_password;
  int              m_port;
  DllLibCMyth*     m_dll;
  CCriticalSection m_section;
  DWORD            m_timestamp;

  static CCriticalSection            m_section_session;
  static std::vector<CCMythSession*> m_sessions;
};

}
