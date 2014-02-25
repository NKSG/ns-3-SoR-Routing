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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <cstring>

#include "sor-routing.h"
#include "sor-packet.h"
#include "sor-ntable.h"
#include "sor-rtable.h"
#include "sor-packet-queue.h"
#include "../../network/model/realtimestamp.h" /* This is for experiment purposes, No need of this line for the compilation */

#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/node-list.h"
#include "ns3/tcp-socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/timer.h"


using namespace std;

NS_LOG_COMPONENT_DEFINE ("SoRRoutingProtocol");

namespace ns3 {
namespace sorrouting {
int RoutingProtocol::count=0; // this defined to trouble shoot the execution sequence of the protocol
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/* TCP UDP Unassigned Port SoRRouting protocol control traffic*/
const uint32_t RoutingProtocol::SORRT_PORT = 275; //269
/* list the interface ID and its IP address */
std::map<string, Ipv4Address> iface; 

struct DeferredRouteOutputTag : public Tag
{
       /// Positive if output device is fixed in RouteOutput
      int32_t oif;
      DeferredRouteOutputTag (int32_t o = -1) : Tag (), oif (o)
      {
      }
      static TypeId GetTypeId ()
      {
            static TypeId tid = TypeId("ns3::sorrouting::DeferredRouteOutputTag").SetParent<Tag> ();
            return tid;
      }
      TypeId GetInstanceTypeId () const
      {
            return GetTypeId ();
      }
      uint32_t GetSerializedSize () const
      {
            return sizeof(int32_t);
      }
      void Serialize (TagBuffer i) const
      {
            i.WriteU32 (oif);
      }
      void Deserialize (TagBuffer i)
      {
            oif = i.ReadU32 ();
      }
      void Print (std::ostream &os) const
      {
            os << "DeferredRouteOutputTag: output interface = " << oif;
      }
};

TypeId RoutingProtocol::GetTypeId (void)
{
      static TypeId tid = TypeId ("ns3::sorrouting::RoutingProtocol")
            .SetParent<Ipv4RoutingProtocol> ()
            .AddConstructor<RoutingProtocol> ()
            .AddAttribute ("PeriodicUpdateInterval","Periodic interval between exchange of full routing tables among nodes. ",
                  TimeValue (Seconds (15)),
                  MakeTimeAccessor (&RoutingProtocol::m_periodicUpdateInterval),
                  MakeTimeChecker ())
            .AddAttribute ("KeepAliveInterval","Periodic interval between two hello packets (keep alive pacekts)",
                  UintegerValue (10),
                  MakeUintegerAccessor (&RoutingProtocol::m_neighborUpdateInterval),
                  MakeUintegerChecker <uint32_t> ())
            .AddAttribute ("SettlingTime", "Minimum time an update is to be stored in adv table before sending out"
                   "in case of change in metric (in seconds)",
                  TimeValue (Seconds (15)),
                  MakeTimeAccessor (&RoutingProtocol::m_settlingTime),
                  MakeTimeChecker ())
            .AddAttribute ("MaxQueueLen", "Maximum number of packets that we allow a routing protocol to buffer.",
                  UintegerValue (1000),
                  MakeUintegerAccessor (&RoutingProtocol::m_maxQueueLen),
                  MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("MaxQueueTime","Maximum time packets can be queued (in seconds). Look for three routing updates.",
                  TimeValue (Seconds (3*15)),/*keep the packet for three updates*/
                  MakeTimeAccessor (&RoutingProtocol::m_maxQueueTime),
                  MakeTimeChecker ())
            .AddAttribute ("MaxNeighborTime","Maximum time that a neighbor table record can be waiting in the neighbor table without recieving a keep alive message",
                  TimeValue (Seconds (10)),
                  MakeTimeAccessor (&RoutingProtocol::m_maxNeighborWaitTime),
                  MakeTimeChecker ())
            .AddAttribute ("enableBuffering","Enables buffering of data packets if no route to destination is available",
                  BooleanValue (true),
                  MakeBooleanAccessor (&RoutingProtocol::SetEnableBufferFlag,&RoutingProtocol::GetEnableBufferFlag),
                  MakeBooleanChecker ());
      return tid;
}

void 
RoutingProtocol::SetEnableBufferFlag (bool f)
{  
      enableBuffering = f;
}
bool 
RoutingProtocol::GetEnableBufferFlag () const
{ 
      return enableBuffering;
}

int64_t 
RoutingProtocol::AssignStreams (int64_t stream)
{ 
      NS_LOG_FUNCTION (this << stream);
      m_uniformRandomVariable->SetStream (stream);
      return 1;
}

RoutingProtocol::RoutingProtocol ()
      : m_routingTable (),
        m_advRoutingTable (),
        m_queue (),
        m_periodicUpdateTimer (Timer::CANCEL_ON_DESTROY)
{  

      helloCounter = 0;
      packetCounter = 0;
      updateCounter = 0;
      routeTime = 0;  
      m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
      srand(time(NULL));
}

RoutingProtocol::~RoutingProtocol ()
{ 
}

void 
RoutingProtocol::DoDispose ()
{ 
      m_ipv4 = 0;
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter = m_socketAddresses.begin (); 
            iter != m_socketAddresses.end (); iter++)
      {
            iter->first->Close ();
      }

      m_socketAddresses.clear ();
      m_neighborTable.DoDispose();
      m_routingTable.Clear();
      Ipv4RoutingProtocol::DoDispose ();
}

void 
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{ 
      m_routingTable.PrintRTable();  
}

void 
RoutingProtocol::PrintStats ()
{  
      //cout<< m_ipv4->GetObject<Node> ()->GetId ()<<" "<<helloCounter<< " "<< packetCounter<< " "<< updateCounter<<endl; 
      cout<< m_ipv4->GetObject<Node> ()->GetId ()<<" "<< (double)routeTime/packetCounter << endl;
      helloCounter = 0;
      packetCounter = 0;
      updateCounter = 0;  
      routeTime = 0;  
      m_printStat.Schedule (Seconds (100));  
}

void 
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
        NS_ASSERT (ipv4 != 0);
        NS_ASSERT (m_ipv4 == 0);
        m_ipv4 = ipv4;
        // Create lo route. It is asserted that the only one interface up for now is loopback
        NS_ASSERT (m_ipv4->GetNInterfaces () == 1 && m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
        m_lo = m_ipv4->GetNetDevice (0);
        NS_ASSERT (m_lo != 0);

        // Remember lo route
        RoutingTableEntry rt (  /*device=*/ m_lo,
                                /*dst=*/Ipv4Address::GetLoopback (), 
                                /*seqno=*/0,
                                /*iface=*/ Ipv4InterfaceAddress (Ipv4Address::GetLoopback (),Ipv4Mask ("255.0.0.0")), 
                                /*hops=*/ 0, 
                                /*attachedNetwork=*/ GetNetworkAddress("127.0.0.1","255.0.0.0"),  
                                /*next hop=*/Ipv4Address::GetLoopback (), 
                                /*lifetime=*/ Simulator::GetMaximumSimulationTime ()
                                /*,socket,interface*/);

        rt.SetFlag (INVALID);
        rt.SetEntriesChanged (false);
        m_routingTable.AddRoute (rt);
        Simulator::ScheduleNow (&RoutingProtocol::Start,this);
         
}

void 
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{     
        NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
        Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
        Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
        if (!l3->IsUp (i))
        {
                return;
        }

        if (!socket)
        {
                if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
                {
                        return;
                }
                
                Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
                NS_ASSERT (socket != 0);
                socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSorRouting,this));
                socket->BindToNetDevice (l3->GetNetDevice (i));
                // Bind to any IP address so that broadcasts can be received
                socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), SORRT_PORT));
                socket->SetAllowBroadcast (true);
                m_socketAddresses.insert (std::make_pair (socket,iface));
                NS_LOG_FUNCTION (this << " added a new socket to the socket table " << socket);
		
		sendHelloPackets(); //Send immidiate notification to its neighbors about the newly added interface

		
                //Get the network attached 
        	std::ostringstream ip;
                ip << iface.GetLocal ();
                std::string ipaddr = ip.str();
                std::ostringstream mask;
                mask << iface.GetMask ();
                std::string maskaddr = mask.str();
                

                std::string netaddr = GetNetworkAddress(ipaddr, maskaddr); 
        
                Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
                RoutingTableEntry rt (  /*device=*/ dev, 
                                        /*dst=*/ iface.GetBroadcast (),
                                        /*seqno=*/ 0,
                                        /*iface=*/ iface,
                                        /*hops=*/ 0,
                                        /*attachedNetwork=*/ netaddr,
                                        /*gateway=*/ iface.GetBroadcast (),
                                        /*lifetime=*/ Simulator::GetMaximumSimulationTime ()
                                        /*, socket, i*/ );
                NS_LOG_FUNCTION (this << " added a new route to the routing table ");
                m_routingTable.AddRoute (rt);
        }
}

void 
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{ 
        Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
        if (socket)
        {
                m_socketAddresses.erase (socket);
                Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();

                if (l3->GetNAddresses (interface))
                {
                        Ipv4InterfaceAddress iface = l3->GetAddress (interface,0);
                        // Create a socket to listen only on this interface
                        Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
                        NS_ASSERT (socket != 0);
                        socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSorRouting,this));
                        // Bind to any IP address so that broadcasts can be received
                        socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), SORRT_PORT));
                        socket->SetAllowBroadcast (true);
                        m_socketAddresses.insert (std::make_pair (socket,iface));
                }
        }
	/*in this function we do not need to consider about the neughbors.
	 * because the neighbor updation methods will automatically compansiate
	 * inactive neighbors from the topology
	 */ 
}

void 
RoutingProtocol::NotifyInterfaceDown (uint32_t interface)
{
	NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal () << " interface is down");
        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
        Ptr<NetDevice> dev = l3->GetNetDevice (interface);
        Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (interface,0));
        NS_ASSERT (socket);
        socket->Close ();
        if (m_socketAddresses.empty ())
        {
                NS_LOG_LOGIC ("No interfaces are found");
                m_routingTable.Clear ();
                return;
        }
        m_socketAddresses.erase (socket);

	/*delete routing records and send a immidiate triggerd update*/
	if (m_mainAddress == m_ipv4->GetAddress (interface,0).GetLocal())
	{
                NS_LOG_FUNCTION (this << " Interface 1 is deleted and sent an immediate Update. ");
                NS_LOG_FUNCTION (this << " Main interface is assigned by the default address ");
		m_routingTable.DeleteRouteForLocalAddress(m_ipv4->GetAddress (interface,0).GetBroadcast());
                m_advRoutingTable.DeleteRouteForLocalAddress(m_ipv4->GetAddress (interface,0).GetBroadcast());
		m_mainAddress = Ipv4Address ();
	}
	else
	{
                NS_LOG_FUNCTION (this << " Interface is deleted and sent an immediate Update. ");
		m_routingTable.DeleteAllRoutesForInterface (m_ipv4->GetAddress (interface,0));
        	m_advRoutingTable.DeleteAllRoutesForInterface (m_ipv4->GetAddress (interface,0));
	}
	/*Send Immidiat Triggered Update*/
	SendTriggeredUpdate();
}

void 
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
	NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal () << " interface is up");

	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
	Ipv4InterfaceAddress iface = l3->GetAddress (i,0);

	if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
	{
	  return;
	}
	
        /*This part is implemented to get the Network address from the IP address
        * Then we we identify the interface ID in the SoR to particular network
        */
        std::ostringstream ip;
        ip << iface.GetLocal ();
        std::string ipaddr = ip.str();
        std::ostringstream mask;
        mask << iface.GetMask ();
        std::string maskaddr = mask.str();
        std::string netaddr = GetNetworkAddress(ipaddr, maskaddr);

	// Create a socket to listen only on this interface
	Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
	NS_ASSERT (socket != 0);
	/*Bind the interface IP address to the socket created*/
	socket->Bind (InetSocketAddress(iface.GetLocal(),SORRT_PORT));
	socket->SetAllowBroadcast (true);
	socket->SetAttribute ("IpTtl",UintegerValue (1));
        socket->BindToNetDevice (l3->GetNetDevice (i));
	socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSorRouting,this));

	m_socketAddresses.insert (std::make_pair (socket,iface));

	////// Add local broadcast record to the routing table
	Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
	RoutingTableEntry rt (  /*device=*/ dev, 
                                /*dst=*/ iface.GetBroadcast (), 
                                /*seqno=*/ 0,
                                /*iface=*/ iface,
                                /*hops=*/ 0,
	                        /*attachedNetwork=*/ netaddr, 
                                /*next hop=*/ iface.GetBroadcast (), 
                                /*lifetime=*/ Simulator::GetMaximumSimulationTime ()
                                /*,socket,i*/);
	m_routingTable.AddRoute (rt);
	if (m_mainAddress == Ipv4Address ())
	{
		m_mainAddress = iface.GetLocal ();
	}

	NS_ASSERT (m_mainAddress != Ipv4Address ());
}

void 
RoutingProtocol::Start ()
{  
        m_queue.SetMaxQueueLen (m_maxQueueLen);
        m_queue.SetQueueTimeout (m_maxQueueTime);

        m_scb = MakeCallback (&RoutingProtocol::Send,this);
        m_ecb = MakeCallback (&RoutingProtocol::Drop,this);
        m_periodicUpdateTimer.SetFunction (&RoutingProtocol::SendPeriodicUpdate,this);
        m_periodicUpdateTimer.Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,1000)));

        m_printRtTableTimer = Timer(Timer::CANCEL_ON_DESTROY);
        m_printRtTableTimer.SetFunction (&RoutingProtocol::PrintRoutingTable,this);

        m_printStat = Timer(Timer::CANCEL_ON_DESTROY);
        m_printStat.SetFunction (&RoutingProtocol::PrintStats,this);
        m_printStat.Schedule (MilliSeconds (10));

        m_periodicHelloTimer = Timer(Timer::CANCEL_ON_DESTROY);
        m_periodicHelloTimer.SetFunction (&RoutingProtocol::sendHelloPackets,this);
	m_periodicHelloTimer.Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,500)));

}

void 
RoutingProtocol::sendHelloPackets ()
{ 
        NS_LOG_FUNCTION (m_mainAddress << " is sending out its Hello Packets");

        for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j!= m_socketAddresses.end (); ++j)
        {
                Ptr<Socket> socket = j->first;
                Ipv4InterfaceAddress iface = j->second; 
                SoRHello helloHeader;
                Ptr<Packet> packet = Create<Packet> ();
                helloHeader.SetPacketType (HELLO_MESSAGE);
                helloHeader.SetsrcAddress (iface.GetLocal ());

                packet->AddHeader (helloHeader);
                socket->SendTo (packet, 0, InetSocketAddress (iface.GetBroadcast (), SORRT_PORT)); 
        }
	m_periodicHelloTimer.Schedule (Seconds (m_uniformRandomVariable->GetInteger (0,m_neighborUpdateInterval)));
}

void 
RoutingProtocol::RecvSorRouting (Ptr<Socket> socket)
{
        Address sourceAddress;
        Ptr<Packet> advpacket = Create<Packet> ();
        Ptr<Socket> nbrSocket;

        Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
        InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
        Ipv4Address sender = inetSourceAddr.GetIpv4 ();
        Ipv4Address receiver = m_socketAddresses[socket].GetLocal (); 
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
        uint32_t packetSize = packet->GetSize ();

        NS_LOG_FUNCTION (m_mainAddress << " received sor packet of size: " << packetSize<< " and packet id: " << packet->GetUid ());


        /*This part is implemented to get the Network address from the IP address
        * Then we we identify the interface ID, Socket and the Node that SoR recieved the update to particular network
        */
        std::ostringstream ip;
        ip << sender;
        std::string ipaddr = ip.str();
        std::ostringstream mask;
        mask << m_socketAddresses[socket].GetMask();
        std::string ipmask = mask.str();
        std::string netaddr = GetNetworkAddress(ipaddr, ipmask); 
        
	if (packetSize == SOR_HELLO_HEADER_SIZE)// This is to filter out exactly hello packets
        {
                if(/*(m_ipv4->GetObject<Node> ()->GetId () == 0 ) && */(m_ipv4->GetInterfaceForDevice (dev) == 1))
                        helloCounter++;
                SoRHello helloHeader;
                packet->RemoveHeader (helloHeader);

                if (helloHeader.GetPacketType () == HELLO_MESSAGE)/*for Hello Packets*/
                {
                        NS_LOG_FUNCTION (m_mainAddress << " Received a Hello packet");
                        for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator k = m_socketAddresses.begin (); 
			k != m_socketAddresses.end (); ++k)
                        {
                                if (k->second == m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0))
                                {
                                        /*Get the actual Socket bound to the selected IPv4Interface*/
                                        nbrSocket  = k->first; 
                                }
                        }       
                        NeighborTableEntry nbrTableEntry;
			//Check the route in the Neighbor Table
                        bool inNbrTable = m_neighborTable.LookupNbr (helloHeader.GetSrcAddress(),nbrTableEntry);
                        if (inNbrTable == false)// if the neighbor is not in the neighborTable 
                        {
                                NS_LOG_DEBUG ("Received New Neighbor Route!");
                                NeighborTableEntry newEntry (   /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                                                /*device=*/ dev, 
                                                                /*socket*/ nbrSocket,
								/*LifeTime*/ Simulator::Now ());
                                m_neighborTable.AddNbr (helloHeader.GetSrcAddress(),newEntry);
                                NS_LOG_DEBUG ("Add the Neighbor to the NeighborTable");
                        }  
			else//if the neighbor is present in the neighbor table
			{
				NS_LOG_DEBUG (helloHeader.GetSrcAddress()<<" Neighbor is in my Neighbor Table!");
				NS_LOG_DEBUG (helloHeader.GetSrcAddress()<<" Update the life time to new time!");
				std::map<Ipv4Address, NeighborTableEntry> nbrTable;
				m_neighborTable.getNbrTable (nbrTable);
				/*update the life time of the neibhor table entries*/
        			for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator j = nbrTable.begin (); j != nbrTable.end (); ++j)
        			{
					if (j->first == helloHeader.GetSrcAddress())
					{
						NeighborTableEntry nte;
						nte.SetInterface(j->second.GetInterface());
						nte.SetOutputDevice(j->second.GetOutputDevice());
						nte.SetOutputSocket(j->second.GetOutputSocket());
						nte.SetLifeTime(Simulator::Now());
						m_neighborTable.UpdateNbr (j->first,nte);
						NS_LOG_DEBUG (helloHeader.GetSrcAddress()<<" Neighbor is Updated!");
					}
				}
			}
		 	/*if neighbors are not responding the maximum toleratable time*/
			std::map<Ipv4Address, NeighborTableEntry> nbrTable;
                        m_neighborTable.getNbrTable (nbrTable);
                        for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator j = nbrTable.begin (); j != nbrTable.end ();j++)
                        {
                                Ipv4Address ipAddressTemp = j->first;
                                NeighborTableEntry entryTemp = j->second;
                                if ((((Simulator::Now() - j->second.GetLifeTime()).GetSeconds())> m_maxNeighborWaitTime))
                                {
                                        m_neighborTable.DeleteNbr (j->first);
                                        NS_LOG_DEBUG (helloHeader.GetSrcAddress()<<" Neighbor is no longer responding and deleted the neighbor!");
                                        NS_LOG_DEBUG ("Now deleting the routes for the neighbor "<< helloHeader.GetSrcAddress());
                                        m_routingTable.DeleteAllRoutesForGateway (ipAddressTemp);
                                        m_advRoutingTable.DeleteAllRoutesForGateway (ipAddressTemp);
					//Send Immidiate Triggered Update
					SendTriggeredUpdate();

                                }
                        }
		
                }//end of Hello Packets
        }
        /*From this point onwards the routing update process is handled*/
        else
        {

                for (; packetSize > 0; packetSize = packetSize - 13)// to unpack the bulked update packets 
                {
                        Ptr<Ipv4L3Protocol> l3_test = m_ipv4->GetObject<Ipv4L3Protocol> ();
	                Ipv4InterfaceAddress iface_test = l3_test->GetAddress (1,0);
                        if(/*(m_ipv4->GetObject<Node> ()->GetId () == 0 ) && */(m_ipv4->GetInterfaceForDevice (dev) == 1))
                                updateCounter++;
                        SorRoutingPktHeader srRoutingHdr;
                        packet->RemoveHeader (srRoutingHdr);       
                        NS_LOG_DEBUG ("Processing new update for " << srRoutingHdr.GetDstAddress ());
                        std::map<Ipv4Address, NeighborTableEntry>  nbrTable;
                        m_neighborTable.getNbrTable (nbrTable);

                        /* Verifying for the piggybacking process. If pacekts are piggybacked, discarding them!
                         * this is beacause we use the broadcast address to tsend the route adverticements*/
                        if (sender == receiver)
                        {
                                if (srRoutingHdr.GetPacketSequenceNumber () % 2 == 1) 
				// if the sequence number is odd it means the interface is down, else the interface is up
                                {
                                        NS_LOG_DEBUG ("Sent Route Update back to the same Destination, with infinite metric."
					<<" Time left to send fwd update: "<< m_periodicUpdateTimer.GetDelayLeft ());
                                }                                 
                                continue;
                        }
                        
                        /*Verifying for the piggybacking process. If pacekts are piggybacked, discarding them!*/ 
                        for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); 
			j != m_socketAddresses.end (); ++j)
                        {
                                Ipv4InterfaceAddress interface = j->second;
                                if (srRoutingHdr.GetDstAddress () == interface.GetLocal ())
                                {
                                        if (srRoutingHdr.GetPacketSequenceNumber () % 2 == 1) 
					// if the sequence number is38 odd it means the interface is down, else the interface is up
                                        {
                                                count++;
                                                NS_LOG_DEBUG ("Sent Route Update back to the same Destination,"
						<<" with infinite metric. Time left to send fwd update: "<<
                                                m_periodicUpdateTimer.GetDelayLeft ());
                                        }
                                        else
                                        {
                                                count++;
                                                NS_LOG_DEBUG ("Received update for my address. Discarding this.");
                                        }
                                }
                        }
                        if (count > 0)
                        {
                                count=0;
                                continue;
                        }
                        NS_LOG_DEBUG ("Received a Route Update packet from "
				<< sender << " to " 
				<< receiver << ". Details are: Destination: " 
				<< srRoutingHdr.GetDstAddress () 
				<< ", Seq No: "<< srRoutingHdr.GetPacketSequenceNumber () 
				<< ", HopCount: " << srRoutingHdr.GetHopCount ());

                        RoutingTableEntry fwdTableEntry, advTableEntry;
                        EventId event;
                        bool permanentTableVerifier = m_routingTable.LookupRoute (srRoutingHdr.GetDstAddress (),fwdTableEntry); 
                        if (permanentTableVerifier == false) 
			// Check the route in the routing table 
                        {
                                if (srRoutingHdr.GetPacketSequenceNumber () % 2 != 1)
				// if an interface up
                                {
                                        NS_LOG_DEBUG ("Received New Route!");
                                        RoutingTableEntry newEntry (/*device=*/ dev, 
                                                                    /*dst=*/srRoutingHdr.GetDstAddress (), 
                                                                    /*seqno=*/srRoutingHdr.GetPacketSequenceNumber (),
                                                                    /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                                                    /*hops=*/ srRoutingHdr.GetHopCount (),
                                                                    /*attachedNetwork=*/ netaddr,
                                                                    /*next hop=*/sender, 
                                                                    /*lifetime=*/Simulator::Now (), 
                                                                    /*settlingTime*/m_settlingTime, 
                                                                    /*entries changed*/true);
                                        newEntry.SetFlag (VALID);
                                        m_routingTable.AddRoute (newEntry);
                                        NS_LOG_DEBUG ("New Route added to both tables");
                                        m_advRoutingTable.AddRoute (newEntry); // add the new entry to the routing table                       
                                }
                                else
                                {
                                        // received update not present in main routing table and also with infinite metric
                                        NS_LOG_DEBUG ("Discarding this update as this route is not"
					" present in main routing table and received with infinite metric");
                                }
                        } 
                        else // if the route is in the routing table
                        {
                                // if the record is not in the adverticement table
                                if (!m_advRoutingTable.LookupRoute (srRoutingHdr.GetDstAddress (),advTableEntry)) 
                                {
                                        RoutingTableEntry tr;
                                        std::map<Ipv4Address, RoutingTableEntry> allRoutes;
                                        m_advRoutingTable.GetListOfAllRoutes (allRoutes);
                                        for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
                                        {
                                                NS_LOG_DEBUG ("ADV table routes are:" << i->second.GetDestination ());
                                        }
                                        // present in fwd table and not in advtable
                                        //add the routing entry to the advertisement Table
                                        m_advRoutingTable.AddRoute (fwdTableEntry);
                                        m_advRoutingTable.LookupRoute (srRoutingHdr.GetDstAddress (),advTableEntry);
                                }
                                if (srRoutingHdr.GetPacketSequenceNumber () % 2 != 1) // if an interface up
                                {
                                        if (srRoutingHdr.GetPacketSequenceNumber () > advTableEntry.GetSeqNo ())
                                        {
                                                // Received update with better seq number. Clear any old events that are running
                                                if (m_advRoutingTable.ForceDeleteIpv4Event (srRoutingHdr.GetDstAddress ()))
                                                {
                                                        NS_LOG_DEBUG ("Canceling the timer to update route with better seq number");
                                                }
						//if the recieved update's metric is different waiting the setting tile to send the update
                                                if (srRoutingHdr.GetHopCount () != advTableEntry.GetHopCount ())
                                                {
                                                        advTableEntry.SetSeqNo (srRoutingHdr.GetPacketSequenceNumber ());
                                                        advTableEntry.SetLifeTime (Simulator::Now ());
                                                        advTableEntry.SetFlag (VALID);
                                                        advTableEntry.SetEntriesChanged (true);
                                                        advTableEntry.SetGateway (sender);
                                                        advTableEntry.SetHopCount (srRoutingHdr.GetHopCount ());
                                                        advTableEntry.SetAttachednetwork(netaddr);
                                                        NS_LOG_DEBUG ("Received update with better sequence number and changed metric."
								<<" Waiting for the settling time");
                                                        Time tempSettlingtime = GetSettlingTime (srRoutingHdr.GetDstAddress ());
                                                        advTableEntry.SetSettlingTime (tempSettlingtime);//set the setting time
                                                        NS_LOG_DEBUG ("Added Settling Time:" 
								<< tempSettlingtime.GetSeconds ()
								<< "s as there is no event running for this route");
                                                        
                                                        //Send the triggered update about the updated route
                                                        event = Simulator::Schedule (tempSettlingtime,&RoutingProtocol::SendTriggeredUpdate,this);
                                                        m_advRoutingTable.AddIpv4Event (srRoutingHdr.GetDstAddress (),event);
                                                        NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
                                                        // if received changed metric, use it but adv it only after Settling Time
                                                        m_routingTable.Update (advTableEntry);
                                                        m_advRoutingTable.Update (advTableEntry);
                                                }
                                                else
                                                {
                                                        // Received update with better seq number and same metric.
                                                        advTableEntry.SetSeqNo (srRoutingHdr.GetPacketSequenceNumber ());
                                                        advTableEntry.SetLifeTime (Simulator::Now ());
                                                        advTableEntry.SetFlag (VALID);
                                                        advTableEntry.SetEntriesChanged (true);
                                                        advTableEntry.SetGateway (sender);
                                                        advTableEntry.SetHopCount (srRoutingHdr.GetHopCount ());
                                                        advTableEntry.SetAttachednetwork(netaddr);
                                                        m_advRoutingTable.Update (advTableEntry);
                                                        
                                                        NS_LOG_DEBUG ("Route with better sequence number and same metric received."
								<<" Advertised without WST");
                                                }
                                        }
					//if the update's sequence number and the present sequence number is same
                                        else if (srRoutingHdr.GetPacketSequenceNumber () == advTableEntry.GetSeqNo ())
                                        {
                                                if (srRoutingHdr.GetHopCount () < advTableEntry.GetHopCount ())
                                                {
                                                        /*Received update with same seq number and better hop count.
                                                        * As the metric is changed, we will have to wait the settling time
							* before sending out this update.
                                                        */
                                                        NS_LOG_DEBUG ("Canceling any existing timer to update route with same sequence number "
                                                        	"and better hop count");
                                                        m_advRoutingTable.ForceDeleteIpv4Event (srRoutingHdr.GetDstAddress ());
                                                        advTableEntry.SetSeqNo (srRoutingHdr.GetPacketSequenceNumber ());
                                                        advTableEntry.SetLifeTime (Simulator::Now ());
                                                        advTableEntry.SetFlag (VALID);
                                                        advTableEntry.SetEntriesChanged (true);
                                                        advTableEntry.SetGateway (sender);
                                                        advTableEntry.SetHopCount (srRoutingHdr.GetHopCount ());
                                                        advTableEntry.SetAttachednetwork(netaddr);
                                                        Time tempSettlingtime = GetSettlingTime (srRoutingHdr.GetDstAddress ());
                                                        advTableEntry.SetSettlingTime (tempSettlingtime);
                                                        NS_LOG_DEBUG ("Added Settling Time," << tempSettlingtime.GetSeconds ()
								<< " as there is no current event running for this route");
                                                        
                                                        event = Simulator::Schedule (tempSettlingtime,&RoutingProtocol::SendTriggeredUpdate,this);
                                                        m_advRoutingTable.AddIpv4Event (srRoutingHdr.GetDstAddress (),event);
                                                        NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
                                                        // if received changed metric, use it but adv it only after wst
                                                        m_routingTable.Update (advTableEntry);
                                                        m_advRoutingTable.Update (advTableEntry);
                                                }
                                                else
                                                {
                                                /*Received update with same seq number but with same or greater hop count.
                                                * Discard that update.
                                                */
                                                        if (!m_advRoutingTable.AnyRunningEvent (srRoutingHdr.GetDstAddress ()))
                                                        {
                                                                /*update the timer only if nexthop address matches thus discarding
                                                                * updates to that destination from other nodes.
                                                                */
                                                                if (advTableEntry.GetGateway () == sender)
                                                                {
                                                                        advTableEntry.SetLifeTime (Simulator::Now ());
                                                                        m_routingTable.Update (advTableEntry);
                                                                }
                                                                m_advRoutingTable.DeleteRoute (srRoutingHdr.GetDstAddress ());
                                                        }
                                                        NS_LOG_DEBUG ("Received update with same seq number and same / worst metric for, " 
								<< srRoutingHdr.GetDstAddress () << ". Discarding the update.");
                                                }
                                        }
                                        else
                                        {
                                                // Received update with an old sequence number. Discard the update
                                                if (!m_advRoutingTable.AnyRunningEvent (srRoutingHdr.GetDstAddress ()))
                                                {
                                                        m_advRoutingTable.DeleteRoute (srRoutingHdr.GetDstAddress ());
                                                }
                                                NS_LOG_DEBUG (srRoutingHdr.GetDstAddress () 
							<< " : Received update with old seq number. Discarding the update.");
                                        }
                                }
                                else
                                {
                                        NS_LOG_DEBUG ("Route with infinite metric received for "
						<< srRoutingHdr.GetDstAddress () << " from " << sender);
                                        
                                        // Delete route only if update was received from my nexthop neighbor
                                        // The next phase of pigibacking process
                                        if (sender == advTableEntry.GetGateway ())
                                        {
                                                NS_LOG_DEBUG ("Triggering an update for this unreachable route:");
                                                std::map<Ipv4Address, RoutingTableEntry> dstsWithNextHopSrc;
                                                m_routingTable.GetListOfDestinationWithGateWay (srRoutingHdr.GetDstAddress (),dstsWithNextHopSrc);
                                                m_routingTable.DeleteRoute (srRoutingHdr.GetDstAddress ());
                                                advTableEntry.SetSeqNo (srRoutingHdr.GetPacketSequenceNumber ());
                                                advTableEntry.SetEntriesChanged (true);
                                                m_advRoutingTable.Update (advTableEntry);
                                                for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = dstsWithNextHopSrc.begin (); 
						i != dstsWithNextHopSrc.end (); ++i)
                                                {
                                                        i->second.SetSeqNo (i->second.GetSeqNo () + 1);//Say it is unreachable
                                                        i->second.SetEntriesChanged (true);
                                                        m_advRoutingTable.AddRoute (i->second);
                                                        m_routingTable.DeleteRoute (i->second.GetDestination ());
                                                }
                                        }
                                        else
                                        {
                                                if (!m_advRoutingTable.AnyRunningEvent (srRoutingHdr.GetDstAddress ()))
                                                {
                                                        m_advRoutingTable.DeleteRoute (srRoutingHdr.GetDstAddress ());
                                                }
                                                NS_LOG_DEBUG (srRoutingHdr.GetDstAddress () 
							<<" : Discard this link break update as it was received from a"
							" different neighbor and I can reach the destination");
                                        }
                                }
                        }              
                }//end of for
                std::map<Ipv4Address, RoutingTableEntry> allRoutes;
                m_advRoutingTable.GetListOfAllRoutes (allRoutes);
                Simulator::Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,1000)),&RoutingProtocol::SendTriggeredUpdate,this);
       }//end of else
}// end

Ptr<Ipv4Route> 
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
        NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

	if (!p) //if packet is empty add it to the internal packet loop
	{
		return LoopbackRoute (header,oif);
	}

	if (m_socketAddresses.empty ()) // if there are no sockets listed in the socket table return empty route
	{
		sockerr = Socket::ERROR_NOROUTETOHOST;
		NS_LOG_LOGIC ("No Sockets are found for the requested route");
		Ptr<Ipv4Route> route;
		return route; // return Empty route
	}

	std::map<Ipv4Address, RoutingTableEntry> removedAddresses;
	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv4Route> route;
	Ipv4Address dst = header.GetDestination ();
	NS_LOG_DEBUG ("Packet Size: " << p->GetSize ()	<< ", Packet id: " << p->GetUid () << ", Destination address in Packet: " << dst);
	RoutingTableEntry rt;
	m_routingTable.Purge (removedAddresses); //remove the routing entries that exceed the maximum time limit

	for (std::map<Ipv4Address, RoutingTableEntry>::iterator rmItr = removedAddresses.begin (); rmItr != removedAddresses.end (); ++rmItr)
	{
                /*Add the removed addresses to the advertisement table to advertice to neighbors*/
		rmItr->second.SetEntriesChanged (true);
		rmItr->second.SetSeqNo (rmItr->second.GetSeqNo () + 1);
		m_advRoutingTable.AddRoute (rmItr->second); 
	}

	if (!removedAddresses.empty ())
	{
                /*send a triggered update about the deleted routes*/
		Simulator::Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger(0,1000)),&RoutingProtocol::SendTriggeredUpdate,this); 
	}

	if (m_routingTable.LookupRoute (dst,rt))
	{
		if (enableBuffering)
		{
			LookForQueuedPackets (); 
		}
		if (rt.GetHopCount () == 1) // check for the immediate nexthop
		{
			route = rt.GetRoute ();
			NS_ASSERT (route != 0);
			NS_LOG_DEBUG ("A route exists from " << route->GetSource ()<< " to neighboring destination "<< route->GetDestination ());

			if (oif != 0 && route->GetOutputDevice () != oif) //ckeck for the correct output interface
			{
		            NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
		            sockerr = Socket::ERROR_NOROUTETOHOST;
		            return Ptr<Ipv4Route> ();
			}
			return route;
		}
		else // if the destination is not the immediate next hop
		{
			RoutingTableEntry newrt;
			if (m_routingTable.LookupRoute (rt.GetGateway (),newrt)) //get the route entry maches the nexthop address
			{
		            route = newrt.GetRoute ();
		            NS_ASSERT (route != 0);
		            NS_LOG_DEBUG ("A route exists from " << route->GetSource () << " to destination " << dst << " via " << rt.GetGateway ());

		            if (oif != 0 && route->GetOutputDevice () != oif) //ckeck for the correct output interface
		            {
                                  NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
                                  sockerr = Socket::ERROR_NOROUTETOHOST;
                                  return Ptr<Ipv4Route> ();
		            }
		            return route;
			}
		}
	}

        //if a no route is find and the local buffer is enable the packets will be delivered to the Local buffer
	if (enableBuffering) // if the routing buffer is enable 
	{
		uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice (oif) : -1);
		DeferredRouteOutputTag tag (iif);

		if (!p->PeekPacketTag (tag))
		{
			p->AddPacketTag (tag);
		}
	}
        return LoopbackRoute (header,oif); // if no route is find in the routing table add it to the loopback buffer to route later
}

bool 
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, 
			Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
			MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
        RealTimeStampCounter rtsc(3200);
	rtsc.StartCounter();
                        
        if( /*(m_ipv4->GetObject<Node> ()->GetId () == 0) && */(m_ipv4->GetInterfaceForDevice (idev) == 1)) // get the attached interface for the recieved Netdevice
        packetCounter++;
	NS_LOG_FUNCTION (m_mainAddress << " received packet " 
                                        << p->GetUid ()<< " from " << header.GetSource ()
                                        << " on interface " << idev->GetAddress ()<< " to destination " << header.GetDestination ());

        if (m_socketAddresses.empty ())
        {
                NS_LOG_DEBUG ("No socket address frond for the requested route");
                return false; // Return no route
        }

        NS_ASSERT (m_ipv4 != 0);
        // Check if input device supports IP
        NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
        int32_t iif = m_ipv4->GetInterfaceForDevice (idev); // get the attached interface for the recieved Netdevice

        Ipv4Address dstAddress = header.GetDestination ();
        Ipv4Address sourceAddress = header.GetSource ();
        // The routig protocol still not supports for Multicasting
        if (dstAddress.IsMulticast ())
        {
                return false;
        }

        // Deferred route request  
        // add the packet to the local Queue  
        if (enableBuffering == true && idev == m_lo)
        {
                DeferredRouteOutputTag tag;
                if (p->PeekPacketTag (tag))
                {
                        AddtoLocalQueue(p,header,ucb,ecb);
                        return true;
                }
        }

        for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
                Ipv4InterfaceAddress iface = j->second;

                if (sourceAddress == iface.GetLocal ()) // Just Do nothing
                {
                        return true;
                }
        } 
        // LOCAL DELIVARY TO INTERFACES
        for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
            Ipv4InterfaceAddress iface = j->second;
                
                if (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif) 
		// Check for the packet delivery interface is listed in the local socket list 
                {
                        if (dstAddress == iface.GetBroadcast () || dstAddress.IsBroadcast ()) 
			// of the recieved packet is a broadcast packet it ckecks for the local router is the destination
                        {
                                Ptr<Packet> packet = p->Copy ();
                                if (lcb.IsNull () == false) 
				// if there is a Local Delivery Callback function 
                                {
                                        NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetLocal ());
                                        lcb (p, header, iif); // call the Local Delivery
                                        // Fall through to additional processing
                                }
                        else
                        {
                                NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " 
					<< p->GetUid () << " from " << sourceAddress);
                                ecb (p, header, Socket::ERROR_NOROUTETOHOST); //Else hand the packet over to errer callback
                        }

                        if (header.GetTtl () > 1) // if the packet is not destinged to the local node forward it using unicast call back
                        {
                                NS_LOG_LOGIC ("Forward broadcast. TTL " << (uint16_t) header.GetTtl ());
                                RoutingTableEntry toBroadcast;

                                if (m_routingTable.LookupRoute (dstAddress,toBroadcast,true))
                                {
                                        Ptr<Ipv4Route> route = toBroadcast.GetRoute ();
                                        rtsc.StopCounter();
                                        routeTime+=rtsc.GetElapsedSeconds();
                                        ucb (route,packet,header);
                                }
                                else
                                {
                                        Drop (p, header, Socket::ERROR_NOROUTETOHOST);
                                        NS_LOG_DEBUG ("No route to forward. Drop packet " << p->GetUid ());
                                }
                        }
                        return true;
                        }
                }
        }

        if (m_ipv4->IsDestinationAddress (dstAddress, iif)) // if the recieve packet is for the local node handle it
        {
                if (lcb.IsNull () == false)
                {
                        NS_LOG_LOGIC ("Unicast local delivery to " << dstAddress);
                        lcb (p, header, iif); // call the local callback
                }
                else
                {
                        NS_LOG_ERROR ("Unable to delilover packet locally due to null callback " << p->GetUid () << " from " << sourceAddress);
                        ecb (p, header, Socket::ERROR_NOROUTETOHOST); //call error callback
                }
                return true;
        }

        RoutingTableEntry toDst;

        if (m_routingTable.LookupRoute (dstAddress,toDst)) //Check the routing table for the next hop
        {
                RoutingTableEntry nexthop;

                if (m_routingTable.LookupRoute (toDst.GetGateway (),nexthop)) // get the next hop
                {
                        Ptr<Ipv4Route> route = nexthop.GetRoute ();
                        NS_LOG_LOGIC (m_mainAddress << " is forwarding packet " 
                                                << p->GetUid ()<< " to " << dstAddress << " from " 
                                                << header.GetSource ()<< " via nexthop neighbor " << toDst.GetGateway ());
                        rtsc.StopCounter();
                        routeTime+=rtsc.GetElapsedSeconds();
                        ucb (route,p,header); //call the unicast callback for the next hop
                        return true;
                }
        }
        /*if nothing is returned, It means no Route input. Therefore, drop the packet and return false*/
        NS_LOG_LOGIC ("Drop packet " << p->GetUid () << " as there is no route to forward it.");
        return false;
}

void 
RoutingProtocol::Send (Ptr<Ipv4Route> route, Ptr<const Packet> packet, const Ipv4Header &header)
{
        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT (l3 != 0);
        Ptr<Packet> p = packet->Copy ();
        l3->Send (p,route->GetSource (),header.GetDestination (),header.GetProtocol (),route); 
}

void 
RoutingProtocol::Drop (Ptr<const Packet> packet, const Ipv4Header &header, Socket::SocketErrno err)
{ 
        //write a procedure to drop tha packets 
        NS_LOG_DEBUG (m_mainAddress << " drop packet " << packet->GetUid () << " to "<< header.GetDestination () << " from queue. Error " << err);
}

void
RoutingProtocol::LookForQueuedPackets ()
{
        NS_LOG_FUNCTION (this);
        Ptr<Ipv4Route> route;
        std::map<Ipv4Address, RoutingTableEntry> allRoutes;
        m_routingTable.GetListOfAllRoutes (allRoutes);
        for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
        {
                RoutingTableEntry rt;
                rt = i->second;
                if (m_queue.Find (rt.GetDestination ()))
                {
                        if (rt.GetHopCount () == 1)
                        {
                                route = rt.GetRoute ();
                                NS_LOG_LOGIC ("A route exists from " 
					<< route->GetSource ()
					<< " to neighboring destination "<< route->GetDestination ());
                                NS_ASSERT (route != 0);
                        }
                        else
                        {
                                RoutingTableEntry newrt;
                                m_routingTable.LookupRoute (rt.GetGateway (),newrt);
                                route = newrt.GetRoute ();
                                NS_LOG_LOGIC ("A route exists from " 
					<< route->GetSource ()<< " to destination " 
					<< route->GetDestination () << " via "<< rt.GetGateway ());
                                NS_ASSERT (route != 0);
                        }
                        SendPacketFromQueue (rt.GetDestination (),route);
                }
        }
}

void
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst,Ptr<Ipv4Route> route)
{
        NS_LOG_DEBUG (m_mainAddress << " is sending a queued packet to destination " << dst);
        QueueEntry queueEntry;
        if (m_queue.Dequeue (dst,queueEntry))
        {
                DeferredRouteOutputTag tag;
                Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
                if (p->RemovePacketTag (tag))
                {
                        if ((tag.oif != -1 && tag.oif != m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ())))
                        {
				Drop (p, queueEntry.GetIpv4Header (), Socket::ERROR_NOROUTETOHOST);
                                NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
                                return;
                        }
                }
                UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
                Ipv4Header header = queueEntry.GetIpv4Header ();
                header.SetSource (route->GetSource ());
                header.SetTtl (header.GetTtl () + 1); // compensate extra TTL decrement by fake loopback routing
                ucb (route,p,header);
                if (m_queue.GetSize () != 0 && m_queue.Find (dst))
                {
                        Simulator::Schedule (MilliSeconds (m_uniformRandomVariable->GetInteger (0,100)),&RoutingProtocol::SendPacketFromQueue,this,dst,route);
                }
        }
}


Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif) const
{
      NS_ASSERT (m_lo != 0);
      Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
      rt->SetDestination (hdr.GetDestination ());

      //if a packet doesno't have a route, the packet willbe moved to the local queue either until
      //a route find or the packet queue time expires.
      // For single interface, single address nodes, this is not a problem.
      // When there are possibly multiple outgoing interfaces, the policy
      // implemented here is to pick the first available router interface.
      // If RouteOutput() caller specified an outgoing interface, that
      // further constrains the selection of source address
      //
      std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
      if (oif)
      {
            // Iterate to find an address on the oif device
            for (; j != m_socketAddresses.end (); ++j)
            {
                  Ipv4Address addr = j->second.GetLocal ();
                  int32_t interface = m_ipv4->GetInterfaceForAddress (addr);

                  if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
                  {
                        rt->SetSource (addr);
                        break;
                  }
            }
      }
      else
      {
            rt->SetSource (j->second.GetLocal ());
      }
      NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid forwarding address is not found");
      rt->SetGateway (Ipv4Address ("127.0.0.1"));
      rt->SetOutputDevice (m_lo);
      return rt;
}

void 
RoutingProtocol::SendTriggeredUpdate ()
{  
        NS_LOG_FUNCTION (m_mainAddress << " is sending a triggered update");
        std::map<Ipv4Address, RoutingTableEntry> allRoutes;
        m_advRoutingTable.GetListOfAllRoutes (allRoutes);
        std::map<Ipv4Address, NeighborTableEntry>  nbrTable;
        m_neighborTable.getNbrTable (nbrTable);
        for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator j = nbrTable.begin (); j != nbrTable.end (); ++j) 
	// Get every neighbor in the neighbor table
        {
                SorRoutingPktHeader srRoutingHdr;
                Ptr<Socket> socket = j->second.GetOutputSocket ();
                Ipv4InterfaceAddress iface = j->second.GetInterface ();
                Ptr<Packet> packet = Create<Packet> ();
                for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
                {
                        NS_LOG_LOGIC ("Destination: " << i->second.GetDestination ()
				<< " SeqNo:" << i->second.GetSeqNo () 
				<< " HopCount:"<< i->second.GetHopCount () + 1);
                        RoutingTableEntry temp = i->second;
                        if ((i->second.GetEntriesChanged () == true) && (!m_advRoutingTable.AnyRunningEvent (temp.GetDestination ()))) 
			// if there is no running event to the route
                        {
                                srRoutingHdr.SetPacketType (TUPDT_MESSAGE);
                                srRoutingHdr.SetDstAddress (i->second.GetDestination ());
                                srRoutingHdr.SetPacketSequenceNumber (i->second.GetSeqNo ());
                                srRoutingHdr.SetHopCount (i->second.GetHopCount () + 1);
                                temp.SetFlag (VALID);
                                temp.SetEntriesChanged (false);
                                m_advRoutingTable.DeleteIpv4Event (temp.GetDestination ());
                                if (!(temp.GetSeqNo () % 2))
                                {
                                        m_routingTable.Update (temp);
                                }
                                packet->AddHeader (srRoutingHdr);//Aggrigate the routing updates
                                m_advRoutingTable.DeleteRoute (temp.GetDestination ());
                                NS_LOG_DEBUG ("Deleted this route from the advertised table");
                        }
                        else
                        {
                                EventId event = m_advRoutingTable.GetEventId (temp.GetDestination ());
                                NS_ASSERT (event.GetUid () != 0);
                                NS_LOG_DEBUG ("EventID " << event.GetUid () 
					<< " associated with " << temp.GetDestination () 
					<< " has not expired, waiting in adv table");
                        }
                }
                if (packet->GetSize () >= 13)//if there are routing updates to be sent
                {
                        
                        RoutingTableEntry temp2;
                        m_routingTable.LookupRoute (iface.GetBroadcast (), temp2);//get the relavent broadcast route in the routing table
                        srRoutingHdr.SetDstAddress (iface.GetLocal ());// add my interface
                        srRoutingHdr.SetPacketSequenceNumber (temp2.GetSeqNo ());
                        srRoutingHdr.SetHopCount (temp2.GetHopCount () + 1);
                        NS_LOG_DEBUG ("Adding my update as well to the packet"); 
                        packet->AddHeader (srRoutingHdr);//aggrigate the route packet with my information
                        

                        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
                        Ipv4Address destination;
                        destination = iface.GetBroadcast ();

                        socket->SendTo (packet, 0, InetSocketAddress (destination, SORRT_PORT));
                        NS_LOG_FUNCTION ("Sent Triggered Update from "
				<< srRoutingHdr.GetDstAddress () 
				<< " with packet id : " << packet->GetUid () 
				<< " and packet Size: " << packet->GetSize ());
                }
                else
                {
                        NS_LOG_FUNCTION ("Update not sent as there are no updates to be triggered");
                }
        }
}

void 
RoutingProtocol::SendPeriodicUpdate ()
{     
        std::map<Ipv4Address, RoutingTableEntry> removedAddresses, allRoutes;
        m_routingTable.Purge (removedAddresses);
        MergeTriggerPeriodicUpdates ();
        m_routingTable.GetListOfAllRoutes (allRoutes);
        if (allRoutes.empty ())
        {
                return;
        }
        NS_LOG_FUNCTION (m_mainAddress << " is sending out its periodic update");
        
        std::map<Ipv4Address, NeighborTableEntry>  nbrTable;
        m_neighborTable.getNbrTable (nbrTable);
        for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator j = nbrTable.begin (); j != nbrTable.end (); ++j) 
	// Get every neighbor in the neighbor table
        {
                SorRoutingPktHeader srRoutingHdr;
                Ptr<Socket> socket = j->second.GetOutputSocket ();
                Ipv4InterfaceAddress iface = j->second.GetInterface ();
                Ptr<Packet> packet = Create<Packet> ();
                for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
                {
                        if (i->second.GetHopCount () == 0)
                        {
                                RoutingTableEntry ownEntry;
                                srRoutingHdr.SetPacketType (PUPDT_MESSAGE);
                                srRoutingHdr.SetDstAddress (i->second.GetInterface().GetLocal ());
                                srRoutingHdr.SetPacketSequenceNumber (i->second.GetSeqNo () + 2);
                                srRoutingHdr.SetHopCount (i->second.GetHopCount () + 1);
                                m_routingTable.LookupRoute (i->second.GetInterface().GetBroadcast (),ownEntry);
                                ownEntry.SetSeqNo (srRoutingHdr.GetPacketSequenceNumber ());
                                m_routingTable.Update (ownEntry);
                                packet->AddHeader (srRoutingHdr);//Aggrigate All routes to a one packet
                        }
                        else
                        {                              
                                srRoutingHdr.SetDstAddress (i->second.GetDestination ());
                                srRoutingHdr.SetPacketSequenceNumber ( i->second.GetSeqNo ());
                                srRoutingHdr.SetHopCount (i->second.GetHopCount () + 1);
                                packet->AddHeader (srRoutingHdr); //Aggrigate All routes in to a one packet
                        }
                        NS_LOG_DEBUG ("Forwarding the update for " << i->first);
                        NS_LOG_DEBUG ("Forwarding details are, Destination: " << srRoutingHdr.GetDstAddress () 
				<< ", SeqNo:" << srRoutingHdr.GetPacketSequenceNumber () 
				<< ", HopCount:" << srRoutingHdr.GetHopCount ()
				<< ", LifeTime: " << i->second.GetLifeTime ().GetSeconds ());
                }
                for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator rmItr = removedAddresses.begin (); 
		rmItr != removedAddresses.end (); ++rmItr)
                {
                        SorRoutingPktHeader removedHeader;
                        removedHeader.SetDstAddress (rmItr->second.GetDestination ());
                        removedHeader.SetPacketSequenceNumber (rmItr->second.GetSeqNo () + 1); // determination point of removind addresses
                        removedHeader.SetHopCount (rmItr->second.GetHopCount () + 1);
                        packet->AddHeader (removedHeader);
                        NS_LOG_DEBUG ("Update for removed record is: Destination: " << removedHeader.GetDstAddress () 
				<< " SeqNo:" << removedHeader.GetPacketSequenceNumber () 
				<< " HopCount:" <<  removedHeader.GetHopCount ());
                }
                // Send to all-hosts broadcast
                Ipv4Address destination;
                destination = iface.GetBroadcast ();

                socket->SendTo (packet, 0, InetSocketAddress (destination, SORRT_PORT));
                NS_LOG_FUNCTION ("PeriodicUpdate Packet UID is : " << packet->GetUid ());
        }
      m_periodicUpdateTimer.Schedule (m_periodicUpdateInterval + MicroSeconds (25 * m_uniformRandomVariable->GetInteger (0,1000)));  
}

void 
RoutingProtocol::MergeTriggerPeriodicUpdates ()
{
        NS_LOG_FUNCTION ("Merging advertised table changes with main table before sending out periodic update");
        std::map<Ipv4Address, RoutingTableEntry> allRoutes;
        m_advRoutingTable.GetListOfAllRoutes (allRoutes);
        if (allRoutes.size () > 0)
        {
                for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
                {
                        RoutingTableEntry advEntry = i->second;
                        if ((advEntry.GetEntriesChanged () == true) && (!m_advRoutingTable.AnyRunningEvent (advEntry.GetDestination ())))
                        {
                                if (!(advEntry.GetSeqNo () % 2))
                                {
                                        advEntry.SetFlag (VALID);
                                        advEntry.SetEntriesChanged (false);
                                        m_routingTable.Update (advEntry);
                                        NS_LOG_DEBUG ("Merged update for " << advEntry.GetDestination () << " with main routing Table");
                                }
                                m_advRoutingTable.DeleteRoute (advEntry.GetDestination ());
                        }
                        else
                        {
                                NS_LOG_DEBUG ("Event currently running. Cannot Merge Routing Tables");
                        }
                }
        }
}

Ptr<Socket> 
RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr) const
{               
      	for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
        	!= m_socketAddresses.end (); ++j)
      	{
            	Ptr<Socket> socket = j->first;
            	Ipv4InterfaceAddress iface = j->second;

            	if (iface == addr)
            	{
                  	return socket;
            	}
      	}

      	Ptr<Socket> socket;
      	return socket;
}

string 
RoutingProtocol::GetNetworkAddress(string ip, string netMask)
{
	int a, b, c, d;
	int e, f, g, h;
	std::stringstream ipAddress(ip);
	std::stringstream net(netMask);	
	char ch; //to temporarily store the '.'
	ipAddress >> a >> ch >> b >> ch >> c >> ch >> d;
	net >> e >> ch >> f >> ch >> g >> ch >> h;
	string result = "";

	a = a&e;
	b = b&f;
	c = c&g;
	d = d&h;

	ostringstream j,k,l,m;
     	j << a;	
	result  = j.str() + ".";
	k << b;	
	result  = result + k.str() + ".";
	l << c;	
	result  = result + l.str() + ".";
	m << d;	
	result  = result + m.str();
	return result;
}

Time
RoutingProtocol::GetSettlingTime (Ipv4Address address)
{
        NS_LOG_FUNCTION ("Calculating the settling time for " << address);
        RoutingTableEntry mainrt;
        Time weightedTime;
        m_routingTable.LookupRoute (address,mainrt);
        if (mainrt.GetSettlingTime () == Seconds (0))
        {
	        return Seconds (0);
        }
        return mainrt.GetSettlingTime ();
}

void
RoutingProtocol::AddtoLocalQueue(Ptr<const Packet> p,const Ipv4Header & header,UnicastForwardCallback ucb,ErrorCallback ecb)
{
        NS_LOG_FUNCTION (this << p << header);
        NS_ASSERT (p != 0 && p != Ptr<Packet> ());
        QueueEntry newEntry (p,header,ucb,ecb);
        bool result = m_queue.Enqueue (newEntry);
        if (result)
        {
                NS_LOG_DEBUG ("Added packet " << p->GetUid () << " to queue.");
        }
}


}//sorrouting
}//ns3

