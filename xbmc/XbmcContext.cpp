/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef ROOT_XBMCCONTEXT_H_INCLUDED
#define ROOT_XBMCCONTEXT_H_INCLUDED
#include "XbmcContext.h"
#endif


#ifndef ROOT_THREADS_THREAD_H_INCLUDED
#define ROOT_THREADS_THREAD_H_INCLUDED
#include "threads/Thread.h"
#endif

#ifndef ROOT_COMMONS_EXCEPTION_H_INCLUDED
#define ROOT_COMMONS_EXCEPTION_H_INCLUDED
#include "commons/Exception.h"
#endif

#ifndef ROOT_UTILS_LOG_H_INCLUDED
#define ROOT_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif


namespace XBMC
{

  class ContextOpaque
  {
  public:
    XbmcCommons::ILogger* loggerImpl;

    ContextOpaque() : loggerImpl(NULL) {}
  };

  Context::Context()
  {
    impl = new ContextOpaque;

    // instantiate
    impl->loggerImpl = new XbmcUtils::LogImplementation;

    // set
    XbmcCommons::Exception::SetLogger(impl->loggerImpl);
    CThread::SetLogger(impl->loggerImpl);
  }

  Context::~Context()
  {
    // cleanup
    XbmcCommons::Exception::SetLogger(NULL);
    CThread::SetLogger(NULL);
    delete impl->loggerImpl;

    delete impl;
  }
}

