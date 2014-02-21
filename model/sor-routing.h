/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hiroaki Nishi Laboratory, Keio University, Japan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Janaka Wijekoon <janaka@west.sd.ekio.ac.jp>, Rajitha Tennekoon <rajitha@west.sd.keio.ac.jp>
 */


#ifndef SOR_ROUTING_H
#define SOR_ROUTING_H

#include <string.h>
#include <sstream>

#include "sor-packet-queue.h"
#include "sor-routing.h"
#include "sor-packet.h"
#include "sor-ntable.h"
#include "sor-rtable.h"

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/tcp-socket.h"

namespace ns3 {
namespace sorrouting {

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
      static TypeId
      GetTypeId (void);
      static const uint32_t SORRT_PORT;

      RoutingProtocol ();
      virtual ~RoutingProtocol ();
      virtual void DoDispose ();

      Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
      bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
      MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb);
      virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;
      virtual void NotifyInterfaceUp (uint32_t interface);
      virtual void NotifyInterfaceDown (uint32_t interface);
      virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
      virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
      virtual void SetIpv4 (Ptr<Ipv4> ipv4);
      void SetEnableBufferFlag (bool f);
      bool GetEnableBufferFlag () const;
      void SetWSTFlag (bool f);
      bool GetWSTFlag () const;
      void SetEnableRAFlag (bool f);
      bool GetEnableRAFlag () const;
      //this is used to find the execution sequence
      static int count; 

      /**
      * Assign a fixed random variable stream number to the random variables
      * used by this model.  Return the number of streams (possibly zero) that
      * have been assigned.
      *
      * \param stream first stream index to use
      * \return the number of stream indices assigned by this model
      */
      int64_t AssignStreams (int64_t stream);

private:
        RoutingTableEntry rEntry;

      /* Holdtimes is the multiplicative factor of PeriodicUpdateInterval for 
       * which the node waits since the last update before flushing a route from 
       * the routing table. If PeriodicUpdateInterval is 8s and Holdtimes is 3, 
       * the node waits for 24s since the last update to flush this route from 
       * its routing table.*/
      uint32_t Holdtimes;

      /* PeriodicUpdateInterval specifies the periodic time interval between which 
       * the a node broadcasts its entire routing table. */
      Time m_periodicUpdateInterval;

      /* periodicUpdateInterval specifies the periodic time interval between which 
       * a node sends keep alive messages*/
      uint32_t m_neighborUpdateInterval;

      /* SettlingTime specifies the time for which a node waits before propagating 
       * an update. It waits for this time interval in hope of receiving an 
       * update with a better metric.*/
      Time m_settlingTime;

      /// Nodes IP address
      Ipv4Address m_mainAddress;

      /// IP protocol
      Ptr<Ipv4> m_ipv4;

      /// Raw socket per each IP interface, map socket -> iface address (IP + mask)
      std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;

      /// Loopback device used to defer route requests until a route is found
      Ptr<NetDevice> m_lo;

      /// Neighbor Table
      NeighborTable m_neighborTable;

      /// Main Routing table for the node
      RoutingTable m_routingTable;

      /// Advertised Routing table for the node
      RoutingTable m_advRoutingTable;

      /// The maximum number of packets that we allow a routing protocol to buffer.
      uint32_t m_maxQueueLen;

      /// The maximum number of packets that we allow per destination to buffer.
      //uint32_t m_maxQueuedPacketsPerDst;

      /// The maximum period of time that a routing protocol is allowed to buffer a packet for.
      Time m_maxQueueTime;

      /// The maximum period of time that a router waits for neighbor response.
      Time m_maxNeighborWaitTime;

      /// A "drop front on full" queue used by the routing layer to buffer packets to which it does not have a route.
      PacketQueue m_queue;

      /// Flag that is used to enable or disable buffering
      bool enableBuffering;

      /// Unicast callback for own packets
      //  This is for Protocol Purposes only
      UnicastForwardCallback m_scb;

      /// Error callback for own packets
      //  This is for Protool purposes only
      ErrorCallback m_ecb;

      /// interface list
      std::map<std::string, int> localIFace;

private:
      /// Start protocol operation
      void Start ();

      /// Queue packet untill we find a route
      void AddtoLocalQueue/*DeferredRouteOutput*/ (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);

      /// Look for any queued packets to send them out
      void LookForQueuedPackets (void);

      /**
      * Send packet from queue
      * \param dst - destination address to which we are sending the packet to
      * \param route - route identified for this packet
      */
      void SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route);

      /// Find socket with local interface address iface
      Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;   
      void sendHelloPackets ();      
      uint16_t GetNodeIDfromIP (Ipv4Address address);
      void RecvSorRouting (Ptr<Socket> socket);
      void ExtractPacket(Ipv4Address dst, uint32_t Key, int msgType); 
      Ipv4Address GetIPfromMAC (Address address);

      void Send (Ptr<Ipv4Route>, Ptr<const Packet>, const Ipv4Header &);

      /// Create loopback route for given header
      Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const;

      /**
      * Get settlingTime for a destination
      * \param dst - destination address
      * \return settlingTime for the destination if found
      */
      Time GetSettlingTime (Ipv4Address dst);

      /// Sends trigger update from a node
      void SendTriggeredUpdate ();

      /// Broadcasts the entire routing table for every PeriodicUpdateInterval
      void SendPeriodicUpdate ();

      void MergeTriggerPeriodicUpdates ();

      /// Notify that packet is dropped for some reason
      void Drop (Ptr<const Packet>, const Ipv4Header &, Socket::SocketErrno);

      /// Print the statistics; Number of packets passed, number of hello packets and Number of routing updates
      void PrintStats ();  

      /// Get the Network Address for the IP and netmask
      std::string GetNetworkAddress(std::string ip, std::string netMask);

      /// Timer to trigger periodic updates from a node
      Timer m_periodicUpdateTimer;

      /// Timer to trigger to send periodic hellow messages to neighbors
      Timer m_periodicHelloTimer;

      /// Timer to trigger print routing table
      Timer m_printRtTableTimer;

      /// Timer used by the trigger updates in case of Weighted Settling Time is used
      Timer m_triggeredExpireTimer;

      ///timer to print the statistics
      Timer m_printStat;

      /// Provides uniform random variables.
      Ptr<UniformRandomVariable> m_uniformRandomVariable;  
      
      //int interfaceID;
      //Time now;
      int cnt;

      uint64_t helloCounter, packetCounter, updateCounter, routeTime;
};
}//sorrouting
}//ns3

#endif /* SOR_ROUTING_H */

