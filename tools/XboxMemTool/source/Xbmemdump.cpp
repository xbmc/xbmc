
#include "..\stdafx.h"
#include "Xbmemdump.h"
#include <shellapi.h>
#include "Log.h"
#include "Util.h"

CXbmemdump::CXbmemdump()
{
  m_hOriginalStdout = INVALID_HANDLE_VALUE;
  m_hChildStdoutRd = INVALID_HANDLE_VALUE;
  m_hChildStdoutWr = INVALID_HANDLE_VALUE;
  m_hChildStdoutRdDup = INVALID_HANDLE_VALUE;
}

CXbmemdump::~CXbmemdump()
{
}

bool CXbmemdump::Setup()
{
  HANDLE hCurrentProcess = GetCurrentProcess();
  
  SECURITY_ATTRIBUTES saAttr; 
  BOOL bSuccess; 

  // Set the bInheritHandle flag so pipe handles are inherited. 
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
  saAttr.bInheritHandle = TRUE; 
  saAttr.lpSecurityDescriptor = NULL; 

  // Get the handle to the current STDOUT. 
  m_hOriginalStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

  // Create a pipe for the child process's STDOUT. 
  if (!CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &saAttr, 0)) 
  {
     g_log.Log(LOG_ERROR, "Stdout pipe creation failed\n");
     return false;
  }

  // Create noninheritable read handle and close the inheritable read handle. 
  bSuccess = DuplicateHandle(hCurrentProcess, m_hChildStdoutRd,
                             hCurrentProcess, &m_hChildStdoutRdDup,
                             0, FALSE, DUPLICATE_SAME_ACCESS);
  if(!bSuccess)
  {
    g_log.Log(LOG_ERROR, "DuplicateHandle failed");
    Finish();
    return false;
  }
  
  return true;
}

void CXbmemdump::Finish()
{
  // set stdout to original value if not already done
  if (m_hOriginalStdout != INVALID_HANDLE_VALUE)
  {
    SetStdHandle(STD_OUTPUT_HANDLE, m_hOriginalStdout);
    m_hOriginalStdout = INVALID_HANDLE_VALUE;
  }

  // close read pipe if still open
  if (m_hChildStdoutRd != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hChildStdoutRd);
    m_hChildStdoutRd = INVALID_HANDLE_VALUE;
  }

  // close write pipe if still open
  if (m_hChildStdoutWr != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hChildStdoutWr);
    m_hChildStdoutWr = INVALID_HANDLE_VALUE;
  }
  
  // close child read pipe (dup) if not already done
  if (m_hChildStdoutRdDup != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hChildStdoutRdDup);
    m_hChildStdoutRdDup = INVALID_HANDLE_VALUE;
  }
}

bool CXbmemdump::Execute(std::vector<std::string>& result)
{
  if (!Setup())
  {
    return false;
  }
  
  // Now create the child process. 
  if (!CreateChildProcess())
  {
     g_log.Log(LOG_ERROR, "Create process failed");
     Finish();
     return false;
  }
  
  // After process creation, restore the saved STDIN and STDOUT. 
  if (!SetStdHandle(STD_OUTPUT_HANDLE, m_hOriginalStdout))
  {
    g_log.Log(LOG_ERROR, "Re-redirecting Stdout failed\n");
    Finish();
    return false;
  }
  // no need to set it again in Finish..
  m_hOriginalStdout = INVALID_HANDLE_VALUE;
  
  BYTE chBuf[4096];
  DWORD dwRead;
  int bufferSize = 0;
  
  // Close the write end of the pipe before reading from the 
  // read end of the pipe. 
  if (!CloseHandle(m_hChildStdoutWr))
  {
    g_log.Log(LOG_ERROR, "Closing write end of channel failed\n");
    Finish();
    return false;
  }
  
  // read data from the stream
  for (;;) 
  { 
    if(!ReadFile( m_hChildStdoutRdDup, chBuf + bufferSize, sizeof(chBuf) - bufferSize, &dwRead, NULL) || dwRead == 0) break; 
    
    bufferSize += dwRead;
    
    // extract the line
    int i = 0;
    while (i < bufferSize)
    {
      if ((bufferSize - i) > 1 && chBuf[i] == '\r' && chBuf[i + 1] == '\n')
      {
        // found eol
        chBuf[i] = '\0';
        chBuf[i + 1] = '\0';
        
        std::string s = (char*)chBuf;
        result.push_back(s);
        
        bufferSize -= (i + 2);
        memmove(chBuf, chBuf + (i + 2), bufferSize);
        i = 0;
      }
      else i++;
    }
    
    //
  }
  return true;
}

bool CXbmemdump::CreateChildProcess()
{
  PROCESS_INFORMATION piProcInfo; 
  STARTUPINFO siStartInfo;

  // Set up members of the PROCESS_INFORMATION structure. 
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure. 
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO); 
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
  siStartInfo.hStdError = m_hChildStdoutWr;
  siStartInfo.hStdOutput = m_hChildStdoutWr;
  siStartInfo.wShowWindow = SW_SHOW;

  // Create the child process. 
  BOOL bFuncRetn = CreateProcess(NULL, 
     "xbmemdump -m",       // command line 
     NULL,          // process security attributes 
     NULL,          // primary thread security attributes 
     TRUE,          // handles are inherited 
     0,             // creation flags 
     NULL,          // use parent's environment 
     NULL,          // use parent's current directory 
     &siStartInfo,  // STARTUPINFO pointer 
     &piProcInfo);  // receives PROCESS_INFORMATION 
  
  if (bFuncRetn == FALSE)
  {
    g_log.Log(LOG_ERROR, "CreateProcess failed");
    Finish();
    return false;
  }
  else 
  {
     CloseHandle(piProcInfo.hProcess);
     CloseHandle(piProcInfo.hThread);
     return bFuncRetn == TRUE;
  }

  return false;
}


/*
  Example of data to parse
  
      Xbox Name:	10.0.0.201 (10.0.0.201)
      Running XBE:	E:\xbmc\xbmcd.xbe

      #1:  80 bytes of heap at 0x01449FC0
	      0x4A7AE8
	      0x4E9A7
	      0x4E8AB
	      0x54DE4C

      #2:  1328 bytes of heap at 0x012D0D60
	      0x4A7AE8
	      0x4AACB5
*/

CSnapShot* CXbmemdump::CreateSnapShotFromVectorData(std::vector<std::string>& result)
{
  CSnapShot* pSnapShot = new CSnapShot();
  int totalAllocationSize = 0;
  int totalAllocations = 0;
  int size = result.size();
  for (int i = 0; i < size; i++)
  {
    const char* line = result[i].c_str();
    if (line[0] != '\0')
    {
      // if the line starts with # we have a new block
      // otherwise do nothing
      if (line[0] == '#')
      {
        CStackTrace* pStackTrace = new CStackTrace();
        
        int res = ParseTraceElementFromVectorData(result, i, pStackTrace);
        if (res < 0)
        {
          // error
          delete pStackTrace;
          delete pSnapShot;
          return NULL;
        }
        totalAllocationSize += pStackTrace->allocatedSize;
        totalAllocations++;
        pSnapShot->AppendStackTrace(pStackTrace);
        i += res;
      }
    }
  }
  
  pSnapShot->m_totalSize = totalAllocationSize;
  pSnapShot->m_totalAllocations = totalAllocations;
  return pSnapShot;
}

int CXbmemdump::ParseTraceElementFromVectorData(std::vector<std::string>& result, int offset, CStackTrace* pStackTrace)
{
  const char* line = result[offset].c_str();
  if (line[0] != '\0')//len > 0)
  {
    // get pos of ':' + 2 and remove that data
    // no use for us atm, it is just a number
    char* start = strchr(line, ':');
    if (start != NULL)
    {
      line += (start - line) + 3;
      // the first is the number of bytes used
      // atoi takes care of not number stuff for us
      pStackTrace->allocatedSize = atoi(line);
      
      // step two is the get the address from where to allocation is made
      // it starts with 0x, so look for the first 0 in the same line
      char* addr = strstr(line, "0x");
      if (addr != NULL)
      {
        addr += 2; // remove the 0x
        pStackTrace->address = _httoi(addr);
        
        // now we have to take care of the stack trace
        // just parse ever address untill we encounter a new line
        int size = result.size();
        int i = offset + 1;
        for (; i < size; i++)
        {
          line = result[i].c_str();
          if (line[0] == '\0')
          {
            // done, empty line
            break;
          }
          // in front is only a tab char, we remove it
          line++;
          DWORD addr = _httoi(line);
          pStackTrace->AppendAddress(addr);
        }
        
        // return nr of lines used
        return (i - offset);
      }
    }
  }
  return -1;
}
