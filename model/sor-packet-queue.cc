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
#include <algorithm>
#include <functional>

#include "sor-packet-queue.h"
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("SorRoutingPacketQueue");

namespace ns3 {
namespace sorrouting {

uint32_t 
PacketQueue::GetSize ()
{
      Purge ();
      return m_queue.size ();
}

bool 
PacketQueue::Enqueue (QueueEntry & entry)
{
      NS_LOG_FUNCTION ("Enqueing packet destined for" << entry.GetIpv4Header ().GetDestination ());
      Purge ();

      for (std::vector<QueueEntry>::const_iterator i = m_queue.begin (); i!= m_queue.end (); ++i)
      {
            if ((i->GetPacket ()->GetUid () == entry.GetPacket ()->GetUid ()) && (i->GetIpv4Header ().GetDestination () == entry.GetIpv4Header ().GetDestination ()))
            {
                  return false;
            }
      }

      if (m_queue.size () >= m_maxLen)
      {
            NS_LOG_DEBUG ("Max packets reached. Not queuing any further packets");
            return false;
      }
      else
      {
            NS_LOG_DEBUG("Packet added to the Queue "<<entry.GetPacket()->GetSize());
            entry.SetExpireTime (m_queueTimeout);
            m_queue.push_back (entry);
            return true;
      }
}

bool 
PacketQueue::Dequeue (Ipv4Address dst, QueueEntry & entry)
{
      NS_LOG_FUNCTION ("Dequeueing packet destined for" << dst);
      Purge ();

      for (std::vector<QueueEntry>::iterator i = m_queue.begin (); i != m_queue.end (); ++i)
      {
            if (i->GetIpv4Header ().GetDestination () == dst)
            {
                  entry = *i;
                  m_queue.erase (i);
                  return true;
            }
      }
      return false;
}

bool 
PacketQueue::Find (Ipv4Address dst)
{
      for (std::vector<QueueEntry>::const_iterator i = m_queue.begin (); 
            i != m_queue.end (); ++i)
      {
            if (i->GetIpv4Header ().GetDestination () == dst)
            {
                  NS_LOG_DEBUG ("Find");
                  return true;
            }
      }
      return false;
}


struct 
IsExpired
{
      bool operator() (QueueEntry const & e) const
      {
            return (e.GetExpireTime () < Seconds (0));
      }
};

void 
PacketQueue::Purge ()
{
      NS_LOG_DEBUG("Purging Queue");
      IsExpired pred;
      for (std::vector<QueueEntry>::iterator i = m_queue.begin (); i != m_queue.end (); ++i)
      {
            if (pred (*i))
            {
                  NS_LOG_DEBUG ("Dropping outdated Packets");
                  Drop (*i, "Drop outdated packet ");
            }
      }
      m_queue.erase (std::remove_if (m_queue.begin (), m_queue.end (), pred), m_queue.end ());
}

void 
PacketQueue::Drop (QueueEntry en, std::string reason)
{
      NS_LOG_LOGIC (reason << en.GetPacket ()->GetUid () << " " << en.GetIpv4Header ().GetDestination ());
      return;
}

} // END OF sorrouting
} // END OF ns3
