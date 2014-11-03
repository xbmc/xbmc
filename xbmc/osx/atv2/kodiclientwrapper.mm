/*
 *  xbmcclient.cpp
 *  xbmclauncher
 *
 *      Created by Stephan Diederich on 17.09.08.
 *      Copyright 2008 Stephan Diederich. All rights reserved.
 *      Copyright (C) 2008-2013 Team XBMC
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
#include <map>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/sysctl.h>
#import <Foundation/Foundation.h>

#include "kodiclient.h"
#include "XBMCDebugHelpers.h"
#include "KodiClientWrapper.h"

//helper class for easy EventSequence handling
class XBMCClientEventSequence{
public:
  XBMCClientEventSequence(){}

  //implicit conversion
  XBMCClientEventSequence(eATVClientEvent f_event){
    m_stream.push_back(f_event);
  }

  std::string str() const{
    std::stringstream ss;
    for(std::vector<eATVClientEvent>::const_iterator it = m_stream.begin();
      it != m_stream.end();
        ++it){
      ss << *it;
    }
    return ss.str();
  }
  void clear(){
    m_stream.clear();
  }
  
  //
  // operators
  //
  friend XBMCClientEventSequence operator+ (XBMCClientEventSequence f_seq, eATVClientEvent f_event){
    f_seq.m_stream.push_back(f_event);
    return f_seq;
  }
  XBMCClientEventSequence& operator << (eATVClientEvent f_event){
    m_stream.push_back(f_event);
    return *this;
  }
  friend bool operator <(XBMCClientEventSequence const& fcr_lhs,XBMCClientEventSequence const& fcr_rhs){
    return fcr_lhs.m_stream < fcr_rhs.m_stream;
  }  
  friend bool operator ==(XBMCClientEventSequence const& fcr_lhs,XBMCClientEventSequence const& fcr_rhs){
    return fcr_lhs.m_stream == fcr_rhs.m_stream;
  }
private:
  std::vector<eATVClientEvent> m_stream;
};


//typedef is here, as is seems that I can't put it into iterface declaration
//CPacketBUTTON is a pointer, as I'm not sure how well it's copy constructor is implemented
typedef std::map<eATVClientEvent, CPacketBUTTON*> tEventMap;
typedef std::map<XBMCClientEventSequence, CPacketBUTTON*> tSequenceMap;

class  KodiClientWrapperImpl{
	tEventMap m_event_map;
  tSequenceMap m_sequence_map;
  
	int					m_socket;	
  std::string	m_address;
  bool m_universal_mode;
  XBMCClientEventSequence m_sequence;
  CFRunLoopTimerRef	m_timer;
  double m_sequence_timeout;
  
  void populateEventMap();
  void populateSequenceMap();
  void sendButton(eATVClientEvent f_event);
  void sendSequence();
  void restartTimer();
  void resetTimer();
  bool isStartToken(eATVClientEvent f_event);
  static void timerCallBack (CFRunLoopTimerRef timer, void *info);
public:
  KodiClientWrapperImpl(bool f_universal_mode, const std::string& fcr_address = "localhost");
  ~KodiClientWrapperImpl();
  void setUniversalModeTimeout(double f_timeout){
    m_sequence_timeout = f_timeout;
  }
  void handleEvent(eATVClientEvent f_event);   
};

void KodiClientWrapperImpl::timerCallBack (CFRunLoopTimerRef timer, void *info)
{
	if (!info)
	{
		fprintf(stderr, "Error. invalid argument to timer callback\n");
		return;
	}
	
	KodiClientWrapperImpl *p_impl = (KodiClientWrapperImpl *)info;
	p_impl->sendSequence();
	p_impl->resetTimer();
}

void KodiClientWrapperImpl::resetTimer(){
	if (m_timer)
	{
		CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);
		CFRunLoopTimerInvalidate(m_timer);
		CFRelease(m_timer);
    m_timer = NULL;
	}
}  

void KodiClientWrapperImpl::restartTimer(){
	if (m_timer)
    resetTimer();	

	CFRunLoopTimerContext context = { 0, this, 0, 0, 0 };
	m_timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + m_sequence_timeout, 0, 0, 0, timerCallBack, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);
}

KodiClientWrapperImpl::KodiClientWrapperImpl(bool f_universal_mode, const std::string& fcr_address): m_address(fcr_address), m_universal_mode(f_universal_mode), m_timer(0), m_sequence_timeout(0.5){
	//PRINT_SIGNATURE();
	
  populateEventMap();
  if (m_universal_mode)
  {
    //DLOG(@"KodiClientWrapperImpl started in universal mode sending to address %s", fcr_address.c_str());
    populateSequenceMap();
  }
  /*
  else
  {
    DLOG(@"KodiClientWrapperImpl started in normal mode sending to address %s", fcr_address.c_str());
  }
  */

	//open udp port etc
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket < 0)
	{
		ELOG(@"Error opening UDP socket! error: ", errno);
		//TODO What to do?
	}
}

KodiClientWrapperImpl::~KodiClientWrapperImpl(){
	//PRINT_SIGNATURE();
  resetTimer();
  shutdown(m_socket, SHUT_RDWR);
}

bool KodiClientWrapperImpl::isStartToken(eATVClientEvent f_event){
  return f_event==ATV_BUTTON_MENU_H;
}

void KodiClientWrapperImpl::sendButton(eATVClientEvent f_event){
  tEventMap::iterator it = m_event_map.find(f_event);
  if(it == m_event_map.end()){
    ELOG(@"KodiClientWrapperImpl::sendButton: No mapping defined for button %i", f_event);	
    return;
  }
  CPacketBUTTON& packet = *(it->second);
  CAddress addr(m_address.c_str());
  packet.Send(m_socket, addr);  
}

void KodiClientWrapperImpl::sendSequence(){
  tSequenceMap::const_iterator it = m_sequence_map.find(m_sequence);
  if(it != m_sequence_map.end()){
    CPacketBUTTON& packet = *(it->second);
    CAddress addr(m_address.c_str());
    packet.Send(m_socket, addr);      
    DLOG(@"KodiClientWrapperImpl::sendSequence: sent sequence %s as button %i", m_sequence.str().c_str(), it->second->GetButtonCode());
  } else {
    ELOG(@"KodiClientWrapperImpl::sendSequence: No mapping defined for sequence %s", m_sequence.str().c_str());
  }
  m_sequence.clear();
}
    
void KodiClientWrapperImpl::handleEvent(eATVClientEvent f_event){	
  if(!m_universal_mode){
    sendButton(f_event);
  }	else {
    //in universal mode no keys are directly send. instead a key-sequence is assembled and a timer started
    //when the timer expires, that key sequence is checked against predefined sequences and if it is a valid one, 
    //a button press is generated
    if(m_sequence.str().empty()){
      if(isStartToken(f_event)){
        m_sequence << f_event;
        DLOG(@"Starting sequence with token %s", m_sequence.str().c_str());
        restartTimer();
      } else {
        sendButton(f_event);
      }
    } else {
        //dont queue release-events but restart timer
        if(f_event == ATV_BUTTON_LEFT_RELEASE || f_event == ATV_BUTTON_RIGHT_RELEASE || f_event == ATV_BUTTON_UP_RELEASE || f_event == ATV_BUTTON_DOWN_RELEASE)
            DLOG(@"Discarded button up event for sequence");
        else{
            m_sequence << f_event;
            DLOG(@"Extended sequence to %s", m_sequence.str().c_str());
        }
        restartTimer();
    }
  }
}

void KodiClientWrapperImpl::populateEventMap(){
	tEventMap& lr_map = m_event_map;
  
	lr_map.insert(std::make_pair(ATV_BUTTON_PLAY,          new CPacketBUTTON(5, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
	lr_map.insert(std::make_pair(ATV_BUTTON_MENU,          new CPacketBUTTON(6, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_MENU_H,        new CPacketBUTTON(8, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_UP,            new CPacketBUTTON(1, "JS0:AppleRemote", BTN_DOWN  | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_UP_RELEASE,    new CPacketBUTTON(1, "JS0:AppleRemote", BTN_UP  | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_DOWN,          new CPacketBUTTON(2, "JS0:AppleRemote", BTN_DOWN | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_DOWN_RELEASE,  new CPacketBUTTON(2, "JS0:AppleRemote", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT,         new CPacketBUTTON(4, "JS0:AppleRemote", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT_RELEASE, new CPacketBUTTON(4, "JS0:AppleRemote", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_BUTTON_LEFT,          new CPacketBUTTON(3, "JS0:AppleRemote", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_BUTTON_LEFT_RELEASE,  new CPacketBUTTON(3, "JS0:AppleRemote", BTN_UP | BTN_QUEUE)));      
	lr_map.insert(std::make_pair(ATV_BUTTON_PLAY_H,  new CPacketBUTTON(7, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  //new aluminium remote buttons
	lr_map.insert(std::make_pair(ATV_ALUMINIUM_PLAY,  new CPacketBUTTON(12, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_ALUMINIUM_PLAY_H, new CPacketBUTTON(13, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  //learned remote buttons
  lr_map.insert(std::make_pair(ATV_LEARNED_PLAY,  new CPacketBUTTON(70, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_PAUSE,  new CPacketBUTTON(71, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_STOP,  new CPacketBUTTON(72, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_PREVIOUS,  new CPacketBUTTON(73, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_NEXT,  new CPacketBUTTON(74, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_REWIND,  new CPacketBUTTON(75, "JS0:AppleRemote", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_REWIND_RELEASE,  new CPacketBUTTON(75, "JS0:AppleRemote", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_FORWARD,  new CPacketBUTTON(76, "JS0:AppleRemote", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_FORWARD_RELEASE,  new CPacketBUTTON(76, "JS0:AppleRemote", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_RETURN,  new CPacketBUTTON(77, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_ENTER,  new CPacketBUTTON(78, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  //gestures
  lr_map.insert(std::make_pair(ATV_GESTURE_SWIPE_LEFT,  new CPacketBUTTON(80, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_GESTURE_SWIPE_RIGHT,  new CPacketBUTTON(81, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_GESTURE_SWIPE_UP,  new CPacketBUTTON(82, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_GESTURE_SWIPE_DOWN,  new CPacketBUTTON(83, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
}

void KodiClientWrapperImpl::populateSequenceMap(){
  XBMCClientEventSequence sequence_prefix;
  sequence_prefix << ATV_BUTTON_MENU_H;
  m_sequence_map.insert(std::make_pair(sequence_prefix, new CPacketBUTTON(8, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(20, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(21, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(22, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(23, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(24, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(25, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_PLAY;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(26, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(27, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(28, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(29, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(30, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(31, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_UP;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(32, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(33, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(34, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(35, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(36, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(37, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_DOWN;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(38, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(39, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(40, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(41, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(42, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(43, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_RIGHT;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(44, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(45, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(46, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(47, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(48, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(49, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_LEFT;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_PLAY, new CPacketBUTTON(50, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON(51, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON(52, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON(53, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON(54, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON(55, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
}

@implementation KodiClientWrapper
- (id) init {
  return [self initWithUniversalMode:false serverAddress:@"localhost"];
}
- (id) initWithUniversalMode:(bool) f_yes_no serverAddress:(NSString*) fp_server{
	//PRINT_SIGNATURE();
	if( ![super init] )
		return nil; 
	mp_impl = new KodiClientWrapperImpl(f_yes_no, [fp_server UTF8String]);
	return self;
}

- (void) setUniversalModeTimeout:(double) f_timeout{
  mp_impl->setUniversalModeTimeout(f_timeout);
}

- (void)dealloc{
	//PRINT_SIGNATURE();
  delete mp_impl;
	[super dealloc];
}

-(void) handleEvent:(eATVClientEvent) f_event{
  mp_impl->handleEvent(f_event);
}
@end
