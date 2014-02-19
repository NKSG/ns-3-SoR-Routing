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
#ifndef SORROUTING_PACKET_H
#define SORROUTING_PACKET_H

#include <iostream>

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#define SOR_PKT_HEADER_SIZE 13
#define SOR_HELLO_HEADER_SIZE 5

namespace ns3 {
namespace sorrouting {

enum packetType {
  HELLO_MESSAGE = 1, // Neighbor Hello Packet
  RTUPDT_MESSAGE = 2, // Routing Table Update
  KEY_MESSAGE = 3, // Key Exchange Messag
  TUPDT_MESSAGE = 4, // Triggered Update
  PUPDT_MESSAGE = 5, // Periodic Update
};

//   ------------------------- SoRRouting Header -------------------------
//
//       |      0       |      1        |      2        |       3       |
//       0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7

//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |        Packet Type            |     Metric    |  HopCount     |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                      Packet Sequence Number                   |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                          Source Address                       |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                       Destination Address                     |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class SorRoutingPktHeader : public Header
{
private:
	uint8_t m_packetType; ///< Type of the Packet
	uint8_t m_metric; ///< Metric Value
	uint8_t m_hopCount; ///< Number of Hops
	uint16_t m_packetSequenceNumber; ///< Destination Sequence Number
	Ipv4Address m_srcAddress; ///< Packet Originated Source IP Address
	Ipv4Address m_dstAddress; ///< Destination IP Address

public:	
	SorRoutingPktHeader (uint8_t packetType = 0, 
				uint8_t metric = 0, 
				uint8_t hopCount = 0, 
				uint16_t packetSequenceNumber = 0, 
				Ipv4Address srcAddress = Ipv4Address (), 
				Ipv4Address dstAddress = Ipv4Address ());
	virtual ~SorRoutingPktHeader ();

	static TypeId GetTypeId (void);
	virtual TypeId GetInstanceTypeId (void) const;

	virtual void Serialize (Buffer::Iterator start) const;
	virtual uint32_t GetSerializedSize () const;
	virtual uint32_t Deserialize (Buffer::Iterator start);

	virtual void Print (std::ostream &os) const;

	void SetPacketType (uint8_t packetType)
	{
		m_packetType = packetType;
	}
	uint8_t GetPacketType () const
	{
		return m_packetType;
	}
	
	void SetMetric (uint8_t metric)
        {
          m_metric = metric;
        }
        uint8_t GetMetric () const
        {
                return m_metric;
        }
	
	void SetHopCount (uint8_t hopCount)
	{
                m_hopCount = hopCount;
	}
	uint8_t GetHopCount () const
	{
                return m_hopCount;
	}

	void SetPacketSequenceNumber (uint16_t packetSequenceNumber)
	{
                m_packetSequenceNumber = packetSequenceNumber;
	}
	uint16_t GetPacketSequenceNumber () const
	{
                return m_packetSequenceNumber;
	}

	void SetSrcAddress (Ipv4Address srcAddress)
	{
                m_srcAddress = srcAddress;
	}
	Ipv4Address GetSrcAddress () const
	{
                return m_srcAddress;
	}
		
	void SetDstAddress (Ipv4Address dstAddress)
	{
                m_dstAddress = dstAddress;
	}
	Ipv4Address GetDstAddress () const
	{
                return m_dstAddress;
	}
};



/*

//   ------------------------- SoR Hello Header -------------------------
//
//       |      0       |      1        |      2        |       3       |
//       0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7

//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |        Packet Type            |     Not Use                   |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                         Sourse Address                        |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

class SoRHello : public Header
{
private:
        uint8_t m_packetType; ///< Type of the Packet
	Ipv4Address m_srcAddress; ///< Packet Originated Source IP Address

public:
        SoRHello (uint8_t packetType = 0, Ipv4Address srcAddress = Ipv4Address ());
	virtual ~SoRHello ();

	static TypeId GetTypeId (void);
	virtual TypeId GetInstanceTypeId (void) const;

	virtual void Serialize (Buffer::Iterator start) const;
	virtual uint32_t GetSerializedSize () const;
	virtual uint32_t Deserialize (Buffer::Iterator start);

	virtual void Print (std::ostream &os) const;

        void SetPacketType (uint8_t packetType)
	{
		m_packetType = packetType;
	}
	uint8_t GetPacketType () const
	{
		return m_packetType;
	}
  
        void SetsrcAddress (Ipv4Address neighborInterfaceAddress)
        {
          m_srcAddress = neighborInterfaceAddress;
        }
        Ipv4Address GetSrcAddress () const
        {
          return m_srcAddress;
        }
  
};

} // END OF sorrouting
} // END OF ns3

#endif /* SORROUTING_PACKET_H */

