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

#include <iomanip>
#include <stdio.h>

#include "sor-ntable.h"

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/timer.h"

NS_LOG_COMPONENT_DEFINE ("SorNeighborTable");

#define IPV4_ADDRESS_SIZE 4

namespace ns3 {
namespace sorrouting {

NeighborTableEntry::NeighborTableEntry (Ipv4InterfaceAddress iface, 
					Ptr<NetDevice> dev, 
					Ptr<Socket> socket, 
					Time lTime)
                              			: m_iface (iface),
                                		m_LocalDev (dev),
                                		m_LocalSocket (socket),
						m_lifeTime ( lTime)
{
	//Cstrctr
}

NeighborTableEntry::~NeighborTableEntry ()
{
	//dstrctr
}

NeighborTable::NeighborTable (){}
NeighborTable::~NeighborTable (){}

bool 
NeighborTable::IsEmpty()
{
	return (m_ipv4NbrTable.empty ())?true:false;
}

bool 
NeighborTable::AddNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry)
{
	NS_LOG_FUNCTION ("Added the new Neighbor " << nbrAdd);
	std::pair<std::map<Ipv4Address, NeighborTableEntry>::iterator,bool> result = m_ipv4NbrTable.insert (std::make_pair (nbrAdd,nbrEntry));
  	return result.second;	
}
bool 
NeighborTable::DeleteNbr (Ipv4Address nbrAdd)
{
	NS_LOG_FUNCTION ("Deleting the Neighbor " << nbrAdd);
	if (m_ipv4NbrTable.empty ())
        {
                return false;
        }

	else 
	{
		m_ipv4NbrTable.erase (nbrAdd);
		NS_LOG_DEBUG("Neighbor erased");
		return true;
	}
	return false;
}

bool 
NeighborTable::DeleteLocalRecord (Ipv4InterfaceAddress localInterface)
{
        if (m_ipv4NbrTable.empty ())
        {
                return false;
        }
        for (std::map<Ipv4Address, NeighborTableEntry>::iterator i = m_ipv4NbrTable.begin (); i!= m_ipv4NbrTable.end (); ++i)
        {
                if (i->second.GetInterface() ==  localInterface)
                {
                        if (m_ipv4NbrTable.erase (i->first) != 0)
                        {
                                NS_LOG_DEBUG("Deleting the neighbor record for the Local Interface"<< i->first);
                                return true;
                        }
                }
        }
        return false;
}


bool 
NeighborTable::LookupNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry)
{
	if (m_ipv4NbrTable.empty ())
	{
		return false;
	}

	std::map<Ipv4Address, NeighborTableEntry>::const_iterator i = m_ipv4NbrTable.find (nbrAdd);

	if (i == m_ipv4NbrTable.end ())
	{
		return false;
	}

	nbrEntry = i->second;  
	return true;
}

bool 
NeighborTable::UpdateNbr (Ipv4Address nbrAdd, NeighborTableEntry & nbrEntry)
{
        if (m_ipv4NbrTable.empty ())
        {
                return false;
        }

        std::map<Ipv4Address, NeighborTableEntry>::iterator i = m_ipv4NbrTable.find (nbrAdd);

        if (i == m_ipv4NbrTable.end ())
        {
                NS_LOG_DEBUG("No neighbor records for update to the given address"<< nbrAdd);
                return false;
        }

        i->second = nbrEntry;
	 NS_LOG_FUNCTION ("The  " << nbrAdd << " has been updated");
        return true;
}

bool 
NeighborTable::LookupNbrRoute (Ipv4Address nbrAdd, Ptr<Ipv4Route> nbrRouteEntry)
{
	if (m_ipv4NbrTable.empty ())
	{
		return false;
	}

	std::map<Ipv4Address, NeighborTableEntry>::const_iterator i = m_ipv4NbrTable.find (nbrAdd);

	if (i == m_ipv4NbrTable.end ())
	{
                NS_LOG_DEBUG("No neighbor records are in the table to return for the given address "<< nbrAdd);
		return false;
	}

	nbrRouteEntry->SetDestination (nbrAdd);
	nbrRouteEntry->SetGateway (nbrAdd);
	nbrRouteEntry->SetSource (i->second.GetInterface ().GetLocal ());
	nbrRouteEntry->SetOutputDevice (i->second.GetOutputDevice());
	return true;
}

void 
NeighborTable::PrintNbrTable () const
{

        std::map<Ipv4Address, NeighborTableEntry>::const_iterator j = m_ipv4NbrTable.begin ();
        std::cout <<"\n" << "Neghbor IP \t\t Local Interface \t\t Life Time\n";
        for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator i = m_ipv4NbrTable.begin (); i!= m_ipv4NbrTable.end (); ++i)
        {
                std::cout << std::setiosflags (std::ios::fixed) 
			<< i->first<<"\t\t"<<i->second.GetInterface().GetLocal()
			<<"\t\t"<<i->second.GetLifeTime().GetSeconds()<<"\n";
        }

        std::cout << "\n";
}

void 
NeighborTable::getNbrTable (std::map<Ipv4Address, NeighborTableEntry> & nbrTable)
{
        for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator i = m_ipv4NbrTable.begin (); i!= m_ipv4NbrTable.end (); ++i)
        {
               nbrTable.insert (std::make_pair (i->first,i->second));
        }   
}

}//NbrTble
}//ns3
