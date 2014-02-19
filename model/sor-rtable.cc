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
#include <map>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <cstring>

#include "ns3/node.h"
#include "sor-rtable.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/tcp-socket.h"

NS_LOG_COMPONENT_DEFINE ("SoRRoutingTable");

#define IPV4_ADDRESS_SIZE 4

namespace ns3 {
namespace sorrouting {

RoutingTableEntry::RoutingTableEntry (Ptr<NetDevice> dev,
                                        Ipv4Address dst,
                                        u_int16_t seqNo,
                                        Ipv4InterfaceAddress iface,
                                        u_int8_t hopCount,
                                        std::string attachedNetwork,
                                        Ipv4Address gateway,
                                        Time lifetime,
                                        Time SettlingTime,
                                        bool areChanged)
                              : m_seqNo (seqNo),
                                m_hopCount (hopCount),
                                m_attachedNetwork(attachedNetwork),
                                m_lifeTime (lifetime),
                                m_iface (iface),
                                m_flag (VALID),
                                m_settlingTime (SettlingTime),
                                m_entriesChanged (areChanged)
{
	m_ipv4Route = Create<Ipv4Route> ();
	m_ipv4Route->SetDestination (dst);
	m_ipv4Route->SetGateway (gateway);
	m_ipv4Route->SetSource (m_iface.GetLocal ());
	m_ipv4Route->SetOutputDevice (dev);
        SetAttachedDevice (dev);
}

RoutingTableEntry::~RoutingTableEntry ()
{
}

RoutingTable::RoutingTable ()
{
}

bool 
RoutingTable::LookupRoute (Ipv4Address id, RoutingTableEntry & rt) // Search for a Given Routing IP (id) in the routing table and returns true if found. At the same time if found, the 'rt' ponter will be set to pointed to the found Routing tableEntry
{
        if (m_ipv4AddressEntry.empty ())
        {
                return false;
        }

        std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = m_ipv4AddressEntry.find (id);

        if (i == m_ipv4AddressEntry.end ())
        {
                return false;
        }

        rt = i->second;  
        return true;
}

bool 
RoutingTable::LookupRoute (Ipv4Address id, RoutingTableEntry & rt, bool forRouteInput)
{
        if (m_ipv4AddressEntry.empty ())
        {
                return false;
        }

        std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = m_ipv4AddressEntry.find (id);

        if (i == m_ipv4AddressEntry.end ())
        {
                return false;
        }

        if (forRouteInput == true && id == i->second.GetInterface ().GetBroadcast ())
        {
                return false;
        }

        rt = i->second;
        return true;
}

bool 
RoutingTable::DeleteRoute (Ipv4Address dst)
{
        if (m_ipv4AddressEntry.erase (dst) != 0)
        {
                NS_LOG_DEBUG("Route erased");
                return true;
        }
        return false;
}

uint32_t 
RoutingTable::RoutingTableSize ()
{
        return m_ipv4AddressEntry.size ();
}

bool 
RoutingTable::AddRoute (RoutingTableEntry & rt)
{
        std::pair<std::map<Ipv4Address, RoutingTableEntry>::iterator, bool> result = m_ipv4AddressEntry.insert (std::make_pair (rt.GetDestination (),rt));
        return result.second;
}

bool 
RoutingTable::Update (RoutingTableEntry & rt)
{
        std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.find (rt.GetDestination ());

        if (i == m_ipv4AddressEntry.end ())
        {
                return false;
        }

        i->second = rt;
        return true;
}

void 
RoutingTable::DeleteAllRoutesForInterface (Ipv4InterfaceAddress iface)
{
        if (m_ipv4AddressEntry.empty ())
        {
                return;
        }

        for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
        {
                if (i->second.GetInterface () == iface)
                {
                        std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
                        ++i;
                        m_ipv4AddressEntry.erase (tmp);
                }
                else
                {
                        ++i;
                }
        }
}

void 
RoutingTable::DeleteAllRoutesForGateway (Ipv4Address gw)
{
        if (m_ipv4AddressEntry.empty ())
        {
                return;
        }

        for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
        {
                if (i->second.GetGateway () == gw)
                {
			if(i->second.GetInterface().GetBroadcast() == gw)
                        {
                                ++i;
                                continue;
                        }

                        std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
                        ++i;
                        m_ipv4AddressEntry.erase (tmp);
                }
                else
                {
                        ++i;
                }
        }
}

void 
RoutingTable::DeleteRouteForLocalAddress (Ipv4Address address)
{
        if (m_ipv4AddressEntry.empty ())
        {
                return;
        }

        for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
        {
                if  (i->second.GetDestination() == address)
                {
                        std::map<Ipv4Address, RoutingTableEntry>::iterator tmp = i;
                        m_ipv4AddressEntry.erase (tmp);
			++i;
                }
                else
                {
                        ++i;
                }
        }
}

void 
RoutingTable::GetListOfAllRoutes (std::map<Ipv4Address, RoutingTableEntry> & allRoutes)
{
        for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.begin (); 
        i != m_ipv4AddressEntry.end (); ++i)
        {
                if (i->second.GetDestination () != Ipv4Address ("127.0.0.1") && i->second.GetFlag () == VALID)
                {
                        allRoutes.insert (std::make_pair (i->first,i->second));
                }
        }
}

void 
RoutingTable::GetListOfDestinationWithGateWay (Ipv4Address gateway, std::map<Ipv4Address, RoutingTableEntry> & unreachable)
{
        unreachable.clear ();
        for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); ++i)
        {
                if (i->second.GetGateway () == gateway)
                {
                        unreachable.insert (std::make_pair (i->first,i->second));
                }
        }
}

void 
RoutingTableEntry::PrintRTableEntry () const
{
        std::cout << std::setiosflags (std::ios::fixed) << m_ipv4Route->GetDestination () << "\t\t" << m_ipv4Route->GetGateway () << "\t"
                << m_iface.GetLocal () << "\t\t" << std::setiosflags (std::ios::left)
                << std::setw (10) << m_attachedNetwork << "\t" << std::setw (10) << (int)m_hopCount << "\t" << std::setw (10) << m_seqNo << "\t"
                << std::setprecision (3) << (Simulator::Now () - m_lifeTime).GetSeconds ()
                << "s\t" << m_settlingTime.GetSeconds () << "s\n";
}


void 
RoutingTable::Purge (std::map<Ipv4Address, RoutingTableEntry> & removedAddresses)
{
        if (m_ipv4AddressEntry.empty ())
        {
                return;
        }
        for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end (); )
        {
                std::map<Ipv4Address, RoutingTableEntry>::iterator i_tmp = i;
                if (i->second.GetLifeTime () > m_holddownTime && (i->second.GetHopCount () > 0))
                {
                        removedAddresses.insert (std::make_pair (i->first,i->second));
                        ++i;
                        m_ipv4AddressEntry.erase (i_tmp);
                }
                else
                {
                        ++i;
                }
        }
        return;
}

void 
RoutingTable::Print (Ptr<OutputStreamWrapper> stream) const
{
        std::map<Ipv4Address, RoutingTableEntry>::const_iterator j = m_ipv4AddressEntry.begin ();
        *stream->GetStream () << "\nSoR Routing table of Node"
                << j->second.GetAttachedDevice()->GetNode()->GetId()
                <<"\n" << "Destination\t\tGateway\t\tInterface\t\tNetwork\t\tHopCount\tSeqNum\t\tLifeTime\t\tSettlingTime\n";

        for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = m_ipv4AddressEntry.begin (); i!= m_ipv4AddressEntry.end (); ++i)
        {
                i->second.PrintRTableEntry();
        }

        *stream->GetStream () << "\n";
}

void 
RoutingTable::PrintRTable () const
{
        std::map<Ipv4Address, RoutingTableEntry>::const_iterator j = m_ipv4AddressEntry.begin ();
        std::cout << "\nSoR Routing table of Node"
                << j->second.GetAttachedDevice()->GetNode()->GetId() 
                <<"\n" << "Destination\t\tGateway\t\tInterface\t\tNetwork\t\tHopCount\tSeqNum\t\tLifeTime\t\tSettlingTime\n";

        for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = m_ipv4AddressEntry.begin (); i!= m_ipv4AddressEntry.end (); ++i)
        {
                i->second.PrintRTableEntry();
        }

        std::cout << "\n";
}

bool 
RoutingTable::AddIpv4Event (Ipv4Address address,EventId id)
{
        std::pair<std::map<Ipv4Address, EventId>::iterator, bool> result = m_ipv4Events.insert (std::make_pair (address,id));
        return result.second;
}

bool 
RoutingTable::AnyRunningEvent (Ipv4Address address)
{
        EventId event;
        std::map<Ipv4Address, EventId>::const_iterator i = m_ipv4Events.find (address);

        if (m_ipv4Events.empty () || i == m_ipv4Events.end ())
        {
                return false;
        }

        event = i->second;
        return event.IsRunning () ? true : false;

}

bool 
RoutingTable::ForceDeleteIpv4Event (Ipv4Address address)
{
        EventId event;
        std::map<Ipv4Address, EventId>::const_iterator i = m_ipv4Events.find (address);

        if (m_ipv4Events.empty () || i == m_ipv4Events.end ())
        {
                return false;
        }

        event = i->second;
        Simulator::Cancel (event);
        m_ipv4Events.erase (address);
        return true;
}

bool 
RoutingTable::DeleteIpv4Event (Ipv4Address address)
{
        EventId event;
        std::map<Ipv4Address, EventId>::const_iterator i = m_ipv4Events.find (address);

        if (m_ipv4Events.empty () || i == m_ipv4Events.end ())
        {
                return false;
        }

        event = i->second;

        if (event.IsRunning ())
        {
                return false;
        }

        if (event.IsExpired ())
        {
                event.Cancel ();
                m_ipv4Events.erase (address);
                return true;
        }
        else
        {
                m_ipv4Events.erase (address);
                return true;
        }
}

EventId RoutingTable::GetEventId (Ipv4Address address)
{
        std::map <Ipv4Address, EventId>::const_iterator i = m_ipv4Events.find (address);
        return (m_ipv4Events.empty () || i == m_ipv4Events.end ()) ? EventId () : i->second; 
}

} // END OF SoRRouting
} // END OF ns3
