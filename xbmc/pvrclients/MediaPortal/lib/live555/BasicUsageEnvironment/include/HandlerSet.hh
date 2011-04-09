/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Basic Usage Environment: for a simple, non-scripted, console application
// C++ header

#ifndef _HANDLER_SET_HH
#define _HANDLER_SET_HH

////////// HandlerSet (etc.) definition //////////

class HandlerDescriptor {
  HandlerDescriptor(HandlerDescriptor* nextHandler);
  virtual ~HandlerDescriptor();

public:
  int socketNum;
  TaskScheduler::BackgroundHandlerProc* handlerProc;
  void* clientData;

private:
  // Descriptors are linked together in a doubly-linked list:
  friend class HandlerSet;
  friend class HandlerIterator;
  HandlerDescriptor* fNextHandler;
  HandlerDescriptor* fPrevHandler;
};

class HandlerSet {
public:
  HandlerSet();
  virtual ~HandlerSet();

  void assignHandler(int socketNum,
		     TaskScheduler::BackgroundHandlerProc* handlerProc,
		     void* clientData);
  void removeHandler(int socketNum);
  void moveHandler(int oldSocketNum, int newSocketNum);

private:
  HandlerDescriptor* lookupHandler(int socketNum);

private:
  friend class HandlerIterator;
  HandlerDescriptor fHandlers;
};

class HandlerIterator {
public:
  HandlerIterator(HandlerSet& handlerSet);
  virtual ~HandlerIterator();

  HandlerDescriptor* next(); // returns NULL if none
  void reset();

private:
  HandlerSet& fOurSet;
  HandlerDescriptor* fNextPtr;
};

#endif
