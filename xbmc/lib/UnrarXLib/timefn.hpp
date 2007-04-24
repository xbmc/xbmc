#ifndef _RAR_TIMEFN_
#define _RAR_TIMEFN_

struct RarLocalTime
{
  uint Year;
  uint Month;
  uint Day;
  uint Hour;
  uint Minute;
  uint Second;
  uint Reminder;
  uint wDay;
  uint yDay;
};


class RarTime
{
  private:
    Int64 GetRaw();
    void SetRaw(Int64 RawTime);

    RarLocalTime rlt;

    Int64 Time;
  public:
    RarTime();
#ifdef _WIN_32
    RarTime& operator =(FILETIME &ft);
    void GetWin32(FILETIME *ft);
#endif
#if defined(_UNIX) || defined(_EMX)
    RarTime& operator =(time_t ut);
    time_t GetUnix();
#endif
    bool operator == (RarTime &rt);
    bool operator < (RarTime &rt);
    bool operator <= (RarTime &rt);
    bool operator > (RarTime &rt);
    bool operator >= (RarTime &rt);
    void GetLocal(RarLocalTime *lt) {*lt=rlt;}
    void SetLocal(RarLocalTime *lt) {rlt=*lt;}
    uint GetDos();
    void SetDos(uint DosTime);
    void GetText(char *DateStr,bool FullYear);
    void SetIsoText(char *TimeText);
    void SetAgeText(char *TimeText);
    void SetCurrentTime();
    void Reset() {rlt.Year=0;}
    bool IsSet() {return(rlt.Year!=0);}
};

const char *GetMonthName(int Month);
bool IsLeapYear(int Year);

#endif
