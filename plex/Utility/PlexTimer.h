//
//  PlexTimer.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-20.
//
//

#ifndef __Plex_Home_Theater__PlexTimer__
#define __Plex_Home_Theater__PlexTimer__

#include <boost/date_time/posix_time/posix_time_types.hpp>

class CPlexTimer
{
  public:
    CPlexTimer();
    void restart();
    int64_t elapsedMs() const;
    int64_t elapsed() const;
  
  private:
    boost::posix_time::ptime m_started;
};

#endif /* defined(__Plex_Home_Theater__PlexTimer__) */
