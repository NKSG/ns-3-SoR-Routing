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
#ifndef SOR_RTABLE_H
#define SOR_RTABLE_H

#include <cassert>
#include <map>
#include <sys/types.h>
#include <string.h>
#include <sstream>
#include <cstring>

#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/tcp-socket.h"


namespace ns3 {
namespace sorrouting {

enum RouteFlags
{
	VALID = 0,     // !< VALID
	INVALID = 1,     // !< INVALID
};

class RoutingTableEntry
{
public:

        RoutingTableEntry (Ptr<NetDevice> dev = 0, 
                        Ipv4Address dst = Ipv4Address (), 
                        u_int16_t m_seqNo = 0,
                        Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (), 
                        u_int8_t hopCount = 0, 
                        std::string attachedNetwork = "", 
                        Ipv4Address gateway = Ipv4Address (),
                        Time lifetime = Simulator::Now (), 
                        Time SettlingTime = Simulator::Now (), 
                        bool changedEntries = false);

        ~RoutingTableEntry ();

        Ipv4Address GetDestination () const
        {
                return m_ipv4Route->GetDestination ();
        }

        Ptr<Ipv4Route> GetRoute () const
        {
                return m_ipv4Route;
        }
        void SetRoute (Ptr<Ipv4Route> route)
        {
                m_ipv4Route = route;
        }

        void SetGateway (Ipv4Address Gateway)
        {
                m_ipv4Route->SetGateway (Gateway);
        }
        Ipv4Address GetGateway () const
        {
                return m_ipv4Route->GetGateway ();
        }

        void SetOutputDevice (Ptr<NetDevice> device)
        {
        	m_ipv4Route->SetOutputDevice (device);
        }
        Ptr<NetDevice> GetOutputDevice () const
        {
                return m_ipv4Route->GetOutputDevice ();
        }

        Ipv4InterfaceAddress GetInterface () const
        {
                return m_iface;
        }
        void SetInterface (Ipv4InterfaceAddress iface)
        {
                m_iface = iface;
        }

        void SetSeqNo (uint16_t sequenceNumber)
        {
                m_seqNo = sequenceNumber;
        }
        uint16_t GetSeqNo () const
        {
                return m_seqNo;
        }

        void SetHopCount (uint8_t hopCount)
        {
                m_hopCount = hopCount;
        }
        uint8_t GetHopCount () const
        {
                return m_hopCount;
        }

        void SetLifeTime (Time lifeTime)
        {
        	m_lifeTime = lifeTime;
        }
        Time GetLifeTime () const
        {
                return (Simulator::Now () - m_lifeTime);
        }

        void SetSettlingTime (Time settlingTime)
        {
                m_settlingTime = settlingTime;
        }
        Time GetSettlingTime () const
        {
                return (m_settlingTime);
        }

        void SetFlag (RouteFlags flag)
        {
                m_flag = flag;
        }
        RouteFlags GetFlag () const
        {
                return m_flag;
        }

        void SetEntriesChanged (bool entriesChanged)
        {
                m_entriesChanged = entriesChanged;
        }
        bool GetEntriesChanged () const
        {
                return m_entriesChanged;
        }

        void SetAttachednetwork(std::string network)
        {
                m_attachedNetwork = network;
        }
        std::string GetAttachednetwork() const
        {
                return m_attachedNetwork;
        }

        void SetAttachedDevice (Ptr<NetDevice> device)
        {
                m_attachedDevice = device;
        }
        Ptr<NetDevice> GetAttachedDevice () const
        {
                return m_attachedDevice;
        }



        bool operator== (Ipv4Address const destination) const
        {
                return (m_ipv4Route->GetDestination () == destination);
        }

        void PrintRTableEntry () const;

private:
        uint16_t m_seqNo; /// Destination SeGetListOfDestinationWithGateWayquence Number
        uint8_t m_hopCount; /// Hop Count (number of hops needed to reach destination)
        std::string m_attachedNetwork; /// the attached network for the route
        Time m_lifeTime;
        Ptr<Ipv4Route> m_ipv4Route;
        /** Ip route, includes
        *   - destination address
        *   - source address
        *   - gateway address
        *   - output device
        */

        Ipv4InterfaceAddress m_iface; /// Output interface address
        RouteFlags m_flag; /// Routing flags: valid, invalid or in search
        Time m_settlingTime; /// Time for which the node retains an update with changed metric before broadcasting it. A node does that in hope of receiving a better update.
        uint32_t m_entriesChanged; /// Flag to show if any of the routing table entries were changed with the routing update.

        Ptr<NetDevice> m_attachedDevice;
        /*uint32_t m_attachedInterface;*/
};


/*
 * brief The Routing table used by SoR routing protocol
 */
class RoutingTable
{
public:

          RoutingTable ();

          /**
          * Add routing table entry if it doesn't yet exist in routing table
          * \param r routing table entry
          * \return true in success
          */
          bool AddRoute (RoutingTableEntry & r);

          /**
          * Delete routing table entry with destination address dst, if it exists.
          * \param dst destination address
          * \return true on success
          */
          bool DeleteRoute (Ipv4Address dst);

          /**
          * Lookup routing table entry with destination address dst
          * \param dst destination address
          * \param rt entry with destination address dst, if exists
          * \return true on success
          */
          bool LookupRoute (Ipv4Address dst, RoutingTableEntry & rt);
          bool LookupRoute (Ipv4Address id, RoutingTableEntry & rt, bool forRouteInput);

          /**
          * Updating the routing Table with routing table entry rt
          * \param rt routing table entry
          * \return true on success
          */
          bool Update (RoutingTableEntry & rt);

          /**
          * Lookup list of addresses for which nxtHp is the next Hop address
          * \param nxtHp nexthop's address for which we want the list of destinations
          * \param dstList is the list that will hold all these destination addresses
          */
          void GetListOfDestinationWithGateWay (Ipv4Address gateway, std::map<Ipv4Address, RoutingTableEntry> & dstList);

          /**
          * Lookup list of all addresses in the routing table
          * \param allRoutes is the list that will hold all these addresses present in the nodes routing table
          */
          void GetListOfAllRoutes (std::map<Ipv4Address, RoutingTableEntry> & allRoutes);

          /// Delete all route from interface with address iface
          void DeleteAllRoutesForInterface (Ipv4InterfaceAddress iface);

          //Delete all routes for the given destination address
          void DeleteAllRoutesForGateway (Ipv4Address gw);

	  ///delete the route for given IP address
	  void DeleteRouteForLocalAddress (Ipv4Address address);


          /// Delete all entries from routing table
          void Clear ()
          {
                m_ipv4AddressEntry.clear ();
          }

          /// Delete all outdated entries if Lifetime is expired
          void Purge (std::map<Ipv4Address, RoutingTableEntry> & removedAddresses);

          /// Print routing table
          void Print (Ptr<OutputStreamWrapper> stream) const;
          void PrintRTable () const;

          /// Provides the number of routes present in that nodes routing table.
          uint32_t RoutingTableSize ();

          /**
          * Add an event for a destination address so that the update to for that destination is sent
          * after the event is completed.
          * \param address destination address for which this event is running.
          * \param id unique eventid that was generated.
          */
          bool AddIpv4Event (Ipv4Address address, EventId id);

          /**
          * Clear up the entry from the map after the event is completed
          * \param address destination address for which this event is running.
          * \return true on success
          */
          bool DeleteIpv4Event (Ipv4Address address);

          /**
          * Force delete an update waiting for settling time to complete as a better update to
          * same destination was received.
          * \param address destination address for which this event is running.
          * \return true on success
          */
          bool AnyRunningEvent (Ipv4Address address);

          /**
          * Force delete an update waiting for settling time to complete as a better update to
          * same destination was received.
          * \param address destination address for which this event is running.
          * \return true on finding out that an event is already running for that destination address.
          */
          bool ForceDeleteIpv4Event (Ipv4Address address);

          /* \param address destination address for which this event is running.
          * \return EventId on finding out an event is associated else return NULL.
          */
          EventId GetEventId (Ipv4Address address);
          Time Getholddowntime () const
          {
                return m_holddownTime;
          }
          void Setholddowntime (Time t)
          {
                m_holddownTime = t;
          }

private:
      /// an entry in the routing table.
      std::map<Ipv4Address, RoutingTableEntry> m_ipv4AddressEntry;

      /// an entry in the event table.
      std::map<Ipv4Address, EventId> m_ipv4Events; // IP address of a node is mapped to the event ID

      Time m_holddownTime;
};

} // END OF sorrouting
} // END OF ns3
#endif /* SOR_RTABLE_H */
