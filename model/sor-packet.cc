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

#include "sor-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace sorrouting {

NS_OBJECT_ENSURE_REGISTERED (SorRoutingPktHeader);
NS_OBJECT_ENSURE_REGISTERED (SoRHello);

// --------------- SorRoutingPktHeader ---------------
SorRoutingPktHeader::SorRoutingPktHeader (	uint8_t packetType, 
						uint8_t metric, 
						uint8_t hopCount, 
						uint16_t packetSequenceNumber, 
						Ipv4Address srcAddress, 
						Ipv4Address dstAddress)
							: m_packetType (packetType),
							m_metric (metric),
							m_hopCount (hopCount),
							m_packetSequenceNumber (packetSequenceNumber),
							m_srcAddress (srcAddress),
							m_dstAddress (dstAddress)
{
}

SorRoutingPktHeader::~SorRoutingPktHeader ()
{
}

TypeId SorRoutingPktHeader::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::sorrouting::SorRoutingPktHeader")
		.SetParent<Header> ()
		.AddConstructor<SorRoutingPktHeader> ();
	return tid;
}

TypeId SorRoutingPktHeader::GetInstanceTypeId () const
{
	return GetTypeId ();
}

uint32_t 
SorRoutingPktHeader::GetSerializedSize () const
{
	return SOR_PKT_HEADER_SIZE;
}

void 
SorRoutingPktHeader::Serialize (Buffer::Iterator start) const
{
	Buffer::Iterator i = start;

	i.WriteU8 (m_packetType);
	i.WriteU8 (m_metric);
	i.WriteU8 (m_hopCount);
	i.WriteHtonU16 (m_packetSequenceNumber);
	i.WriteHtonU32 (m_srcAddress.Get ());
	i.WriteHtonU32 (m_dstAddress.Get ());
}

uint32_t 
SorRoutingPktHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;

	m_packetType = i.ReadU8 ();
	m_metric = i.ReadU8 ();
	m_hopCount = i.ReadU8 ();
	m_packetSequenceNumber = i.ReadNtohU16 ();
	m_srcAddress = Ipv4Address (i.ReadNtohU32 ());
	m_dstAddress = Ipv4Address (i.ReadNtohU32 ());
	
	uint32_t dist = i.GetDistanceFrom (start);
	NS_ASSERT (dist == GetSerializedSize ());
	return dist;
}

void 
SorRoutingPktHeader::Print (std::ostream &os) const
{            
	os << "Packet Type : " << m_packetType
		<< " Metric : " << m_metric
		<< " Hop Count : " << m_hopCount
		<< " Packet Sequence No. : " << m_packetSequenceNumber
		<< " Source Address : " << m_srcAddress
		<< " Destination Address : " << m_dstAddress;
}

SoRHello::SoRHello (uint8_t packetType, Ipv4Address srcAddress): m_packetType (packetType),m_srcAddress (srcAddress)
{}

SoRHello::~SoRHello ()
{}
TypeId SoRHello::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::sorrouting::SoRHello")
		.SetParent<Header> ()
		.AddConstructor<SorRoutingPktHeader> ();
	return tid;
}

TypeId SoRHello::GetInstanceTypeId () const
{
	return GetTypeId ();
}

void SoRHello::Print (std::ostream &os) const{/*this method is not implemented*/}

void SoRHello::Serialize (Buffer::Iterator start) const
{
        Buffer::Iterator i = start;
        i.WriteU8 (m_packetType);
        i.WriteHtonU32 (m_srcAddress.Get ());
         
}
uint32_t SoRHello::GetSerializedSize () const
{
        return (sizeof(m_packetType)+sizeof(m_srcAddress));
}
uint32_t SoRHello::Deserialize (Buffer::Iterator start)
{
        Buffer::Iterator i = start;

        m_packetType = i.ReadU8 ();
        m_srcAddress = Ipv4Address (i.ReadNtohU32 ());       

	uint32_t dist = i.GetDistanceFrom (start);
	NS_ASSERT (dist == GetSerializedSize ());
	return dist;
}

} // END OF sorrouting
} // END OF ns3
