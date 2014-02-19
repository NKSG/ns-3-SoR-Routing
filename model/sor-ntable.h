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


#ifndef SOR_NTABLE_H
#define SOR_NTABLE_H

#include <cassert>
#include <map>
#include <sys/types.h>

#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3 {
namespace sorrouting {
class NeighborTableEntry
{
public:

        NeighborTableEntry (Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (), 
				Ptr<NetDevice> LocalDev = 0, 
				Ptr<Socket> socket = 0, 
				Time lTime = Simulator::Now () );
        ~NeighborTableEntry ();

        Ipv4InterfaceAddress GetInterface () const
        {
                return m_iface;
        }
        void SetInterface (Ipv4InterfaceAddress iface)
        {
                m_iface = iface;
        }
        
        void SetOutputDevice (Ptr<NetDevice> LocalDev)
        {
                m_LocalDev = LocalDev;
        }
        Ptr<NetDevice> GetOutputDevice () const
        {
                return m_LocalDev;
        }

        void SetOutputSocket (Ptr<Socket> socket)
        {
                m_LocalSocket = socket;
        }
        Ptr<Socket> GetOutputSocket () const
        {
                return m_LocalSocket;
        }
	void SetLifeTime( Time time)
	{
		m_lifeTime =  time;
	}
	Time GetLifeTime () const
	{
		return m_lifeTime;
	}

private:
        Ipv4InterfaceAddress m_iface; /* Output interface address.*/
        Ptr<NetDevice> m_LocalDev; /* Local NetDevice*/
        Ptr<Socket> m_LocalSocket; /* binded socket for above interface and the net device */
	Time m_lifeTime;
};



class NeighborTable
{
public:
        NeighborTable ();
        ~NeighborTable ();
        bool AddNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry);
        bool DeleteNbr (Ipv4Address nbrAdd);
	bool DeleteLocalRecord (Ipv4InterfaceAddress localInterface);
        bool LookupNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry);
	bool UpdateNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry);
	void PrintNbrTable() const;
        bool LookupNbrRoute (Ipv4Address nbrAdd, Ptr<Ipv4Route> nbrRouteEntry); 
        void getNbrTable (std::map<Ipv4Address, NeighborTableEntry> & nbrTable);
	bool IsEmpty();
	/// Delete all entries from routing table
	void DoDispose ()
	{
		m_ipv4NbrTable.clear ();
	}
private:

        std::map<Ipv4Address, NeighborTableEntry> m_ipv4NbrTable;
};


}//sorrouting
}//ns3
#endif /* SOR_NTABLE_H */
