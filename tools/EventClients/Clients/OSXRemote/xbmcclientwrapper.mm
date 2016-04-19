/*
 *  xbmcclient.cpp
 *  xbmclauncher
 *
 *  Created by Stephan Diederich on 17.09.08.
 *  Copyright 2008 University Heidelberg. All rights reserved.
 *
 */
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "xbmcclientwrapper.h"
#include "../../lib/c++/xbmcclient.h"
#include "XBMCDebugHelpers.h"
#include <vector>
#include <map>
#include <string>
#include <sstream>

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
typedef std::map<std::pair<int, eATVClientEvent>, CPacketBUTTON*> tMultiRemoteMap;

class  XBMCClientWrapperImpl{
	tEventMap m_event_map;
  tSequenceMap m_sequence_map;
  tMultiRemoteMap m_multiremote_map;
  eRemoteMode m_mode;
	int					m_socket;	
  std::string	m_address;
  int         m_port;
  XBMCClientEventSequence m_sequence;
  CFRunLoopTimerRef	m_timer;
  double m_sequence_timeout;
  int m_device_id;
  bool m_verbose_mode;
  void populateEventMap();
  void populateSequenceMap();
  void populateMultiRemoteModeMap();
  void sendButton(eATVClientEvent f_event);
  void sendSequence();
  void restartTimer();
  void resetTimer();
  bool isStartToken(eATVClientEvent f_event);
  static void timerCallBack (CFRunLoopTimerRef timer, void *info);
public:
  XBMCClientWrapperImpl(eRemoteMode f_mode, const std::string& fcr_address = "localhost", int f_port = 9777, bool f_verbose_mode=false);
  ~XBMCClientWrapperImpl();
  void setUniversalModeTimeout(double f_timeout){
    m_sequence_timeout = f_timeout;
  }
  void switchRemote(int f_device_id){
    m_device_id = f_device_id;
  }
  void handleEvent(eATVClientEvent f_event);   
  void enableVerboseMode(bool f_value){
    m_verbose_mode = f_value;
  }
};

void XBMCClientWrapperImpl::timerCallBack (CFRunLoopTimerRef timer, void *info)
{
	if (!info)
	{
		fprintf(stderr, "Error. invalid argument to timer callback\n");
		return;
	}
	
	XBMCClientWrapperImpl *p_impl = (XBMCClientWrapperImpl *)info;
	p_impl->sendSequence();
	p_impl->resetTimer();
}

void XBMCClientWrapperImpl::resetTimer(){
	if (m_timer)
	{
		CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);
		CFRunLoopTimerInvalidate(m_timer);
		CFRelease(m_timer);
    m_timer = NULL;
	}
}  

void XBMCClientWrapperImpl::restartTimer(){
	if (m_timer)
    resetTimer();	
  
	CFRunLoopTimerContext context = { 0, this, 0, 0, 0 };
	m_timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + m_sequence_timeout, 0, 0, 0, timerCallBack, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);
}

XBMCClientWrapperImpl::XBMCClientWrapperImpl(eRemoteMode f_mode, const std::string& fcr_address, int f_port, bool f_verbose_mode): 
m_mode(f_mode), m_address(fcr_address), m_port(f_port), m_timer(0), m_sequence_timeout(0.5), m_device_id(150), m_verbose_mode(f_verbose_mode){	
  if(m_mode == MULTIREMOTE_MODE){
    if(m_verbose_mode)
      NSLog(@"XBMCClientWrapperImpl started in multiremote mode sending to address %s, port %i", fcr_address.c_str(), f_port);
    populateMultiRemoteModeMap();
  } else {
    if(m_mode == UNIVERSAL_MODE){
      if(m_verbose_mode)
        NSLog(@"XBMCClientWrapperImpl started in universal mode sending to address %s, port %i", fcr_address.c_str(), f_port);
      populateSequenceMap();
    } else if(m_verbose_mode)
        NSLog(@"XBMCClientWrapperImpl started in normal mode sending to address %s, port %i", fcr_address.c_str(), f_port);
    populateEventMap();
  }
  
	//open udp port etc
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket < 0)
	{
		ELOG(@"Error opening UDP socket! error: ", errno);
		//TODO What to do?
	}
}

namespace {
 struct delete_second{
   template <class T> 
   void operator ()(T& fr_pair){
     delete fr_pair.second;
   }
 };
}
XBMCClientWrapperImpl::~XBMCClientWrapperImpl(){
  resetTimer();
  shutdown(m_socket, SHUT_RDWR);
  std::for_each(m_event_map.begin(), m_event_map.end(), delete_second());
  std::for_each(m_sequence_map.begin(), m_sequence_map.end(), delete_second());
  std::for_each(m_multiremote_map.begin(), m_multiremote_map.end(), delete_second());
}

bool XBMCClientWrapperImpl::isStartToken(eATVClientEvent f_event){
  return f_event==ATV_BUTTON_MENU_H;
}

void XBMCClientWrapperImpl::sendButton(eATVClientEvent f_event){
  CPacketBUTTON* lp_packet = 0;
  if(m_mode == MULTIREMOTE_MODE){
    tMultiRemoteMap::iterator it = m_multiremote_map.find(std::make_pair(m_device_id, f_event));
    if(it == m_multiremote_map.end()){
      ELOG(@"XBMCClientWrapperImpl::sendButton: No mapping defined for remoteID: %i button %i", m_device_id, f_event);	
      return;
    }
    lp_packet = it->second;
  } else {
    tEventMap::iterator it = m_event_map.find(f_event);
    if(it == m_event_map.end()){
      ELOG(@"XBMCClientWrapperImpl::sendButton: No mapping defined for button %i", f_event);	
      return;
    }
    lp_packet = it->second;
  }
  assert(lp_packet);
  CAddress addr(m_address.c_str(), m_port);
  if(m_verbose_mode)
    NSLog(@"XBMCClientWrapperImpl::sendButton sending button %i down:%i up:%i", lp_packet->GetButtonCode(), lp_packet->GetFlags()&BTN_DOWN,lp_packet->GetFlags()&BTN_UP );
  lp_packet->Send(m_socket, addr);  
}

void XBMCClientWrapperImpl::sendSequence(){
  tSequenceMap::const_iterator it = m_sequence_map.find(m_sequence);
  if(it != m_sequence_map.end()){
    CPacketBUTTON& packet = *(it->second);
    CAddress addr(m_address.c_str());
    packet.Send(m_socket, addr);      
    if(m_verbose_mode)
      NSLog(@"XBMCClientWrapperImpl::sendSequence sent sequence %i down:%i up:%i", packet.GetButtonCode(), packet.GetFlags()&BTN_DOWN,packet.GetFlags()&BTN_UP );
  } else {
    ELOG(@"XBMCClientWrapperImpl::sendSequence: No mapping defined for sequence %s", m_sequence.str().c_str());
  }
  m_sequence.clear();
}

void XBMCClientWrapperImpl::handleEvent(eATVClientEvent f_event){	
  if(m_mode != UNIVERSAL_MODE){
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

void XBMCClientWrapperImpl::populateEventMap(){
	tEventMap& lr_map = m_event_map;
  
	lr_map.insert(std::make_pair(ATV_BUTTON_CENTER,          new CPacketBUTTON("obc5", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT,         new CPacketBUTTON("obc4", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT_RELEASE, new CPacketBUTTON("obc4", "R2", BTN_UP | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_LEFT,          new CPacketBUTTON("obc3", "R2", BTN_DOWN | BTN_NO_REPEAT| BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_LEFT_RELEASE,  new CPacketBUTTON("obc3", "R2", BTN_UP | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_MENU,          new CPacketBUTTON("obc6", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_MENU_H,        new CPacketBUTTON("obc8", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_UP,            new CPacketBUTTON("obc1", "R2", BTN_DOWN | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_UP_RELEASE,    new CPacketBUTTON("obc1", "R2", BTN_UP | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_DOWN,          new CPacketBUTTON("obc2", "R2", BTN_DOWN | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_DOWN_RELEASE,  new CPacketBUTTON("obc2", "R2", BTN_UP | BTN_QUEUE)));
	
	// only present on ATV <= 2.1 <--- check that; OSX seems to have the release parts
	lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT_H, new CPacketBUTTON("obc10", "R2", BTN_DOWN | BTN_QUEUE)));	
	lr_map.insert(std::make_pair(ATV_BUTTON_RIGHT_H_RELEASE, new CPacketBUTTON("obc10", "R2", BTN_UP | BTN_QUEUE)));	
	lr_map.insert(std::make_pair(ATV_BUTTON_LEFT_H,  new CPacketBUTTON("obc9", "R2", BTN_DOWN | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_LEFT_H_RELEASE, new CPacketBUTTON("obc9", "R2", BTN_UP | BTN_QUEUE)));	

	//new aluminium remote buttons
	lr_map.insert(std::make_pair(ATV_BUTTON_PLAY,  new CPacketBUTTON("obc12", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
	lr_map.insert(std::make_pair(ATV_BUTTON_PLAY_H, new CPacketBUTTON("obc13", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));

	// only present on atv >= 2.2
	lr_map.insert(std::make_pair(ATV_BUTTON_CENTER_H,  new CPacketBUTTON("obc7", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  //learned remote buttons (ATV >=2.3)
  lr_map.insert(std::make_pair(ATV_LEARNED_PLAY,  new CPacketBUTTON("obc70", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_PAUSE,  new CPacketBUTTON("obc71", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_STOP,  new CPacketBUTTON("obc72", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_PREVIOUS,  new CPacketBUTTON("obc73", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_NEXT,  new CPacketBUTTON("obc74", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_REWIND,  new CPacketBUTTON("obc75", "R2", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_REWIND_RELEASE,  new CPacketBUTTON("obc75", "R2", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_FORWARD,  new CPacketBUTTON("obc76", "R2", BTN_DOWN | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_FORWARD_RELEASE,  new CPacketBUTTON("obc76", "R2", BTN_UP | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_RETURN,  new CPacketBUTTON("obc77", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  lr_map.insert(std::make_pair(ATV_LEARNED_ENTER,  new CPacketBUTTON("obc78", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
}

void XBMCClientWrapperImpl::populateSequenceMap(){
  XBMCClientEventSequence sequence_prefix;
  sequence_prefix << ATV_BUTTON_MENU_H;
  m_sequence_map.insert(std::make_pair(sequence_prefix, new CPacketBUTTON("obc8", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc20", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc21", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc22", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc23", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc24", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc25", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_CENTER;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc26", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc27", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc28", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc29", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc30", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc31", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_UP;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc32", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc33", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc34", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc35", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc36", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc37", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_DOWN;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc38", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc39", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc40", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc41", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc42", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc43", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_RIGHT;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc44", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc45", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc46", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc47", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc48", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc49", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  
  sequence_prefix.clear();
  sequence_prefix << ATV_BUTTON_MENU_H << ATV_BUTTON_LEFT;
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_CENTER, new CPacketBUTTON("obc50", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_RIGHT, new CPacketBUTTON("obc51", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_LEFT, new CPacketBUTTON("obc52", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_UP, new CPacketBUTTON("obc53", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_DOWN, new CPacketBUTTON("obc54", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
  m_sequence_map.insert(std::make_pair( sequence_prefix + ATV_BUTTON_MENU, new CPacketBUTTON("obc55", "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
}

const char * GetObcString(int obcCode)
{
    std::string ret =   "obc" + std::to_string(obcCode);
    return ret.c_str();
}

void XBMCClientWrapperImpl::populateMultiRemoteModeMap(){
  //the example of harmony as a multi-remote uses a few key device-id's paired with the normal buttons
  static int device_ids[] = {150, 151, 152, 153, 154, 155, 157, 158, 159, 160};
  int offset = 100;// to distinguish them from the other ids as we only have one keyboard map for all inputs
  for(int* device_id = device_ids; device_id != device_ids + sizeof(device_ids)/sizeof(*device_ids); ++device_id, offset += 10)
  {
    // keymaps for mult-apple-remote, including the device-key sent after remote-switch
    // we just add them here with unique button numbers and do the real mapping in keymap.xml
    // as an offset for the buttons we use the device 
    
    // this loop should probably be replaced by a proper setting of the individual keys
    // cons: currently only button-codes (aka ints) are sent
    //       way too lazy ;)  
    // pro: custom tweaks. e.g. button 1 on the harmony may be  (153, ATV_BUTTON_LEFT) and this should not get a repeat
    //      maybe use the loop and tweak individual buttons later; plex maps here to strings, and later in keymap.xml to other strings,
    //      but this may need another kind of remote in XBMC source
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_UP),            new CPacketBUTTON(GetObcString(1 + offset), "R2", BTN_DOWN | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_UP_RELEASE),    new CPacketBUTTON(GetObcString(1 + offset), "R2", BTN_UP | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_DOWN),          new CPacketBUTTON(GetObcString(2 + offset), "R2", BTN_DOWN | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_DOWN_RELEASE),  new CPacketBUTTON(GetObcString(2 + offset), "R2", BTN_UP | BTN_QUEUE)));
    
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_LEFT),          new CPacketBUTTON(GetObcString(3 + offset), "R2", BTN_DOWN | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_LEFT_RELEASE),  new CPacketBUTTON(GetObcString(3 + offset), "R2", BTN_UP | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_RIGHT),         new CPacketBUTTON(GetObcString(4 + offset), "R2", BTN_DOWN | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_RIGHT_RELEASE), new CPacketBUTTON(GetObcString(4 + offset), "R2", BTN_UP | BTN_QUEUE)));
    
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_CENTER),          new CPacketBUTTON(GetObcString(5 + offset), "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE))); 
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_MENU),          new CPacketBUTTON(GetObcString(6 + offset), "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_CENTER_H),        new CPacketBUTTON(GetObcString(7 + offset), "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_MENU_H),        new CPacketBUTTON(GetObcString(8 + offset), "R2", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE)));
    
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_RIGHT_H),       new CPacketBUTTON(GetObcString(9 + offset), "R2", BTN_DOWN | BTN_QUEUE)));
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_RIGHT_H_RELEASE),new CPacketBUTTON(GetObcString(9 + offset), "R2", BTN_UP | BTN_QUEUE)));
        
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_LEFT_H),        new CPacketBUTTON(GetObcString(10 + offset), "R2", BTN_DOWN | BTN_QUEUE)));    
    m_multiremote_map.insert(std::make_pair(std::make_pair(*device_id,ATV_BUTTON_LEFT_H_RELEASE),new CPacketBUTTON(GetObcString(10 + offset), "R2", BTN_UP | BTN_QUEUE)));    
  }
}

@implementation XBMCClientWrapper
- (id) init {
  return [self initWithMode:DEFAULT_MODE serverAddress:@"localhost" port:9777 verbose: false];
}
- (id) initWithMode:(eRemoteMode) f_mode serverAddress:(NSString*) fp_server port:(int) f_port verbose:(bool) f_verbose{
	if( ![super init] )
		return nil; 
	mp_impl = new XBMCClientWrapperImpl(f_mode, [fp_server UTF8String], f_port, f_verbose);
	return self;
}

- (void) setUniversalModeTimeout:(double) f_timeout{
  mp_impl->setUniversalModeTimeout(f_timeout);
}

- (void)dealloc{
  delete mp_impl;
	[super dealloc];
}

-(void) handleEvent:(eATVClientEvent) f_event{
  mp_impl->handleEvent(f_event);
}

-(void) switchRemote:(int) f_device_id{
  mp_impl->switchRemote(f_device_id);
}

- (void) enableVerboseMode:(bool) f_really{
  mp_impl->enableVerboseMode(f_really);
}
@end
