#ifndef PLEXBUSYINDICATOR_H
#define PLEXBUSYINDICATOR_H

#include "threads/Event.h"
#include "threads/CriticalSection.h"
#include "Job.h"

#include <map>

class CPlexBusyIndicator : public IJobCallback
{
public:
  CPlexBusyIndicator();
  bool blockWaitingForJob(CJob* job, IJobCallback *callback);

  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total,
                     const CJob* job);

private:
  CCriticalSection m_section;
  std::map<int, IJobCallback*> m_callbackMap;
  CEvent m_blockEvent;
};

#endif // PLEXBUSYINDICATOR_H
