/*
 *      Copyright (C) 2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/api2/AddonLib.hpp"
#include "kodi/api2/addon/General.hpp"
#include "kodi/api2/gui/DialogOK.hpp"
#include "kodi/api2/gui/DialogSelect.hpp"
#include "kodi/api2/gui/DialogProgress.hpp"
#include "kodi/api2/gui/DialogExtendedProgress.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <p8-platform/util/timeutils.h>
#include <p8-platform/threads/threads.h>

using namespace P8PLATFORM;
using namespace V2;


class CThreadTest : public P8PLATFORM::CThread
{
public:
  CThreadTest();
  ~CThreadTest();

protected:
  virtual void *Process(void);

};

CThreadTest::CThreadTest()
{
  CreateThread();
}

CThreadTest::~CThreadTest()
{
}

void* CThreadTest::Process(void)
{
  int ret = KodiAPI::InitThread();

  int cnt = 8;
  while (!IsStopped())
  {
    //sleep(1);
    cnt--;
    KodiAPI::Log(ADDON_LOG_ERROR, "Countdown               ---> %i", cnt);

    if (cnt == 0)
      break;
  }

  KodiAPI::Finalize();
  return NULL;
}






int frequency_of_primes (int n) {
  int i,j;
  int freq=n-1;
  uint64_t count = 0;
  for (i=2; i<=n; ++i)
  {
    for (j=sqrt(i);j>1;--j)
    {
      if (i%j==0)
      {
        --freq;
        break;
      }
        ++count;
    }
  }
  fprintf(stderr, "count = %lu\n", count);
  return freq;
}

/**
 * Currently for test purpose is library a executable.
 */
int main(int argc, char *argv[])
{
  addon_properties props;
  props.id              = "demo_binary_addon";
  props.type            = "xbmc.addon.executable";
  props.version         = "0.0.1";
  props.name            = "Demo binary add-on";
  props.license         = "GPL2";
  props.summary         = "";
  props.description     = "Hallo";
  props.path            = "";
  props.libname         = "";
  props.author          = "";
  props.source          = "";
  props.icon            = "";
  props.disclaimer      = "";
  props.changelog       = "";
  props.fanart          = "";
  props.is_independent  = false;
  props.use_net_only    = false;

  //sleep(5);

  if (KodiAPI::Init(argc, argv, &props, "127.0.0.1") != API_SUCCESS)
  {
    fprintf(stderr, "Binary AddOn: %i %s\n", KODI_API_lasterror, KodiAPI_ErrorCodeToString(KODI_API_lasterror));
    return 1;
  }

  KodiAPI::Log(ADDON_LOG_ERROR, "Hello, World from Log");

  sleep(4);
   //CThreadTest test;


    /// \ingroup CAddonLib_General
    /// @brief Returns the value of an addon property as a string
    /// @param[in] id id of the property that the module needs to access
    /// |              | Choices are  |              |
    /// |:------------:|:------------:|:------------:|
    /// |  author      | icon         | stars        |
    /// |  changelog   | id           | summary      |
    /// |  description | name         | type         |
    /// |  disclaimer  | path         | version      |
    /// |  fanart      | profile      |              |
    ///
    /// @return Returned information string
    ///
    /// @warning Throws exception on wrong string value!
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.c}
    /// #include <kodi/api2/addon/General.h>
    /// ...
    /// const char* addonName = CAddonLib_General::GetAddonInfo("name");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    fprintf(stderr, "- %s\n", KodiAPI::AddOn::General::GetAddonInfo("description").c_str());

   KodiAPI::AddOn::General::QueueFormattedNotification(QUEUE_INFO, "Hallo");
   KodiAPI::AddOn::General::QueueNotification(QUEUE_WARNING, "Hallo", "fhfgdhfdh");
  KodiAPI::AddOn::General::QueueNotification("/home/alwin/.kodi/userdata/Thumbnails/0/00a1133b.png", "Hallo", "fhfgdhfdh");

  std::string md5;
  KodiAPI::AddOn::General::GetMD5("Test", md5);
  fprintf(stderr, "----------- %s %s\n", md5.c_str(), KodiAPI::AddOn::General::GetLanguage().c_str());

  KodiAPI::GUI::DialogOK::ShowAndGetInput("Hallo", "hfghfghgfgf");

     const std::vector<std::string> entries
     {
       "Test 1",
       "Test 2",
       "Test 3",
       "Test 4",
       "Test 5"
     };

     int selected = KodiAPI::GUI::DialogSelect::Show("Test selection", entries, -1);
     if (selected < 0)
       fprintf(stderr, "Item selection canceled\n");
     else
       fprintf(stderr, "Selected item is: %i\n", selected);

   KodiAPI::GUI::CDialogExtendedProgress *ext_progress = new KodiAPI::GUI::CDialogExtendedProgress("Test Extended progress");
   ext_progress->SetText("Test progress");
   for (unsigned int i = 0; i < 50; i += 10)
   {
     ext_progress->SetProgress(i, 100);
     sleep(1);
   }

   ext_progress->SetTitle("Test Extended progress - Second round");
   ext_progress->SetText("Test progress - Step 2");

   for (unsigned int i = 50; i < 100; i += 10)
   {
     ext_progress->SetProgress(i, 100);
     sleep(1);
   }
   delete ext_progress;


   KodiAPI::GUI::CDialogProgress *progress = new KodiAPI::GUI::CDialogProgress;
   progress->SetHeading("Test progress");
   progress->SetLine(1, "line 1");
   progress->SetLine(2, "line 2");
   progress->SetLine(3, "line 3");
   progress->SetCanCancel(true);
   progress->ShowProgressBar(true);
   progress->Open();
   for (unsigned int i = 0; i < 100; i += 10)
   {
     progress->SetPercentage(i);
     sleep(1);
   }
   delete progress;

  // CThreadTest* test2 = new CThreadTest;
  // CThreadTest* test3 = new CThreadTest;
  // CThreadTest* test4 = new CThreadTest;
  // CThreadTest* test5 = new CThreadTest;
//   CThreadTest* test6 = new CThreadTest;
//   CThreadTest* test7 = new CThreadTest;
//   CThreadTest* test8 = new CThreadTest;
//   CThreadTest* test9 = new CThreadTest;
//   CThreadTest* test10 = new CThreadTest;
//   CThreadTest* test11 = new CThreadTest;
//   CThreadTest* test12 = new CThreadTest;
//   CThreadTest* test13 = new CThreadTest;
  // sleep(10);
  // delete test2;
  // delete test3;
  // delete test4;
  // delete test5;




/*
  clock_t t;
  int f;
  t = clock();
  printf ("Calculating...\n");
  f = frequency_of_primes (99999);
  printf ("The number of primes lower than 100,000 is: %d\n",f);
  t = clock() - t;
  printf ("It took me %ld clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);
  */

//sleep(10);



KodiAPI::Log(ADDON_LOG_ERROR, "He22llo, World from Log");

  KodiAPI::Finalize();

  return 0;
}
