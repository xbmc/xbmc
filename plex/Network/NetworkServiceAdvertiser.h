/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Dec 16, 2010
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include "NetworkService.h"
#include "NetworkServiceBase.h"

#define CRLF "\r\n"

class NetworkServiceAdvertiser;
typedef boost::shared_ptr<NetworkServiceAdvertiser> NetworkServiceAdvertiserPtr;

/////////////////////////////////////////////////////////////////////////////
class NetworkServiceAdvertiser : public NetworkServiceBase
{
 public:
  
  /// Constructor.
  NetworkServiceAdvertiser(boost::asio::io_service& ioService, const boost::asio::ip::address& groupAddr, unsigned short port)
   : NetworkServiceBase(ioService)
   , m_port(port)
  {
    // This is where we'll send notifications.
    m_notifyEndpoint = boost::asio::ip::udp::endpoint(groupAddr, m_port+1);
  }
  
  /// Destructor.
  virtual ~NetworkServiceAdvertiser() {}
 
  /// Start advertising the service.
  void start()
  {
    doStart();
  }
  
  /// Stop advertising the service.
  void stop()
  {
    doStop();
    
    // Send out the BYE message synchronously and close the socket.
    if (m_socket)
    {
      dprintf("NetworkService: Stopping advertisement.");
      string hello = "BYE * HTTP/1.0\r\n" + createReplyMessage();
      try { m_socket->send_to(boost::asio::buffer(hello), m_notifyEndpoint); } catch (...) { eprintf("Couldn't send BYE packet."); }
      m_socket->close();
    }
  }
  
  /// Advertise an update to the service.
  void update(const string& parameter="")
  {
    broadcastMessage(m_socket, "UPDATE", parameter);
  }
  
  /// For subclasses to fill in.
  virtual void createReply(map<string, string>& headers) {}
  
  /// For subclasses to fill in.
  virtual string getType() = 0;
  virtual string getResourceIdentifier() = 0;
  virtual string getBody() = 0;
  
 protected:
  
  virtual void doStart() {}
  virtual void doStop() {}
  
 private:
  
  /// Handle network change.
  virtual void handleNetworkChange(const vector<NetworkInterface>& interfaces)
  {
    dprintf("Network change for advertiser.");

    // Create a socket if we need to.
    if (!m_socket)
    {
      // Start up new socket.
      m_socket = udp_socket_ptr(new boost::asio::ip::udp::socket(m_ioService));

      // Listen, send out HELLO, and start reading data.
      setupMulticastListener(m_socket, "0.0.0.0", m_notifyEndpoint.address(), m_port);

      // Listen for the first packet.
      m_socket->async_receive_from(boost::asio::buffer(m_data, NS_MAX_PACKET_SIZE), m_endpoint, boost::bind(&NetworkServiceAdvertiser::handleRead, this, m_socket, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    
    // Always send out the HELLO packet, just in case.
    broadcastMessage(m_socket, "HELLO");
  }

  void broadcastMessage(const udp_socket_ptr& socket, const string& action, const string& parameter="")
  {
    // Send out the message.
    if (socket)
    {
      string hello = action + " * HTTP/1.0\r\n" + createReplyMessage(parameter);
      try { socket->send_to(boost::asio::buffer(hello), m_notifyEndpoint); } catch (...) {}
    }
  }
  
  void handleRead(const udp_socket_ptr& socket, const boost::system::error_code& error, size_t bytes)
  {
    if (!error)
    {
      // Create the reply, we don't actually look at the request for now.
      string reply = "HTTP/1.0 200 OK\r\n" + createReplyMessage();
      
      // Write the reply back to the client and wait for the next packet.
      try { socket->send_to(boost::asio::buffer(reply), m_endpoint); } 
      catch (...) { wprintf("Error replying to broadcast packet."); }
    }
    
    if (error)
    {
      eprintf("Network Service: Error in advertiser handle read: %d (%s) socket=%d", error.value(), error.message().c_str(), m_socket->native());
      usleep(1000 * 100);
    }
    
    // If the socket is open, keep receiving (On XP we need to abandon a socket for 10022 - An invalid argument was supplied - as well).
    if (socket->is_open() && error.value() != 10022)
      socket->async_receive_from(boost::asio::buffer(m_data, NS_MAX_PACKET_SIZE), m_endpoint, boost::bind(&NetworkServiceAdvertiser::handleRead, this, socket, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));      
    else
      iprintf("Network Service: Abandoning advertise socket, it was closed.");
  }
  
  // Turn the parameter map into HTTP headers.
  string createReplyMessage(const string& parameter="")
  {
    string reply;

    map<string, string> params;
    createReply(params);

    reply = "Content-Type: " + getType() + CRLF;
    reply += "Resource-Identifier: " + getResourceIdentifier() + CRLF;
    BOOST_FOREACH(string_pair param, params)
      reply += param.first + ": " + param.second + CRLF;

    // See if there's a body.
    string body = getBody();
    if (body.empty() == false)
      reply += "Content-Length: " + boost::lexical_cast<string>(body.size()) + CRLF;
    
    // See if there's a parameter.
    if (parameter.empty() == false)
      reply += "Parameters: " + parameter;
    
    reply += CRLF;
    reply += body;
    
    return reply;
  }
  
  udp_socket_ptr    m_socket;
  unsigned short    m_port;
  boost::asio::ip::udp::endpoint m_endpoint;
  boost::asio::ip::udp::endpoint m_notifyEndpoint;
  char              m_data[NS_MAX_PACKET_SIZE];
};
