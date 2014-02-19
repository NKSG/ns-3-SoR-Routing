/* this is a testing program writtne to test the SoR implementation from the begening
 * this program creates the followint static routing topology. Add and delete nodes 
 * should done at your own risk. 
 * 			initial topology as follow
 *			  src (172.16.1.1)	
 *				|
 *				| 10.0.1.0/24				
 *				| 			        
 *				|
 *		    src2-------rtr1 
 * 		172.16.2.1	|
 * 				|
 * 				| 10.20.2.0/24 
 * 				|
 * 				|
 * 			       rtr2 
 * 			        /\
 * 			      /    \
 * 	       10.10.3.0/24 /        \ 10.10.4.0/24
 * 			  /            \
 * 		        dst1           dst2
 *                 (192.168.1.1)    (192.168.2.1)
 *			 
*/

#include <iostream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"

#include "ns3/sor-routing-module.h"


using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("SoRRouteTestingTopology");

int main (int argc,char *argv[])
{
	CommandLine cmd;
	cmd.Parse(argc,argv);
	
	// create the routers and the aplication nodes
	Ptr<Node> src = CreateObject<Node> ();
	Ptr<Node> rtr1 = CreateObject<Node> ();
	Ptr<Node> rtr2 = CreateObject<Node> ();
	Ptr<Node> dst1 = CreateObject<Node> ();
	Ptr<Node> dst2 = CreateObject<Node> ();
	Ptr<Node> src2 = CreateObject<Node> ();
	//src->SoREnable(1);
	//src2->SoREnable(1);
	rtr1->SoREnable(1);
	rtr2->SoREnable(1);
	//dst1->SoREnable(1);
	//dst2->SoREnable(1);
	
	//add created nodes to the node container list
	NodeContainer a = NodeContainer (src,rtr1,rtr2,dst1,dst2);
	NodeContainer b = NodeContainer (src2);
	NodeContainer c = NodeContainer (a,b);
	
	//attach the TCP/IP protocol stack to the nodes and routers
	InternetStackHelper internet;

	SorRoutingHelper sorrouting;

  	Ipv4ListRoutingHelper list;
  	list.Add (sorrouting, 0);

  	internet.SetRoutingHelper (list);
 	internet.Install (c);


	//point-to-point links
	NodeContainer srcrtr1 = NodeContainer (src,rtr1);
	NodeContainer rtr1rtr2 = NodeContainer (rtr1,rtr2);
	NodeContainer dst1rtr2 = NodeContainer (dst1,rtr2);
	NodeContainer dst2rtr2 = NodeContainer (dst2,rtr2);
	NodeContainer src2rtr1 = NodeContainer (src2,rtr1);
	
	//create channels to add creatd links
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer Dsrcrtr1 = p2p.Install (srcrtr1);
        NetDeviceContainer Drtr1rtr2 = p2p.Install (rtr1rtr2);
        NetDeviceContainer Ddst1rtr2 = p2p.Install (dst1rtr2);
        NetDeviceContainer Ddst2rtr2 = p2p.Install (dst2rtr2);
	NetDeviceContainer Dsrc2rtr1 = p2p.Install (src2rtr1);
	
	//Ptr<NetDevice> srctortr1 = Dsrcrtr1.Get (0);
	//Ptr<NetDevice> dst1tortr2 = Ddst1rtr2.Get (0);
	//Ptr<NetDevice> dst2tortr2 = Ddst2rtr2.Get (0);

	Ptr<CsmaNetDevice> Sdevice = CreateObject<CsmaNetDevice> ();
  	Sdevice->SetAddress (Mac48Address::Allocate ());
  	src->AddDevice (Sdevice);

  	Ptr<CsmaNetDevice> Ddevice1 = CreateObject<CsmaNetDevice> ();
  	Ddevice1->SetAddress (Mac48Address::Allocate ());
  	dst1->AddDevice (Ddevice1);

	Ptr<CsmaNetDevice> Ddevice2 = CreateObject<CsmaNetDevice> ();
  	Ddevice2->SetAddress (Mac48Address::Allocate ());
  	dst2->AddDevice (Ddevice2);
	
        Ptr<CsmaNetDevice> Sdevice2 = CreateObject<CsmaNetDevice> ();
        Sdevice2->SetAddress (Mac48Address::Allocate ());
        src2->AddDevice (Sdevice2);

	//Assign IP address to the links and the node interfaces
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.1.0","255.255.255.0");
	Ipv4InterfaceContainer isrcrtr1 = ipv4.Assign(Dsrcrtr1);
	ipv4.SetBase("10.0.2.0","255.255.255.0");
	Ipv4InterfaceContainer irtr1rtr2 = ipv4.Assign(Drtr1rtr2);
	ipv4.SetBase("10.0.3.0","255.255.255.0");
        Ipv4InterfaceContainer idst1rtr2 = ipv4.Assign(Ddst1rtr2);
	ipv4.SetBase("10.0.4.0","255.255.255.0");
        Ipv4InterfaceContainer Idst2rtr2 = ipv4.Assign(Ddst2rtr2);
	ipv4.SetBase("10.0.5.0","255.255.255.0");
        Ipv4InterfaceContainer Isrc2rtr1 = ipv4.Assign(Dsrc2rtr1);


	Ptr<Ipv4> ipv4src = src->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4rtr1 = rtr1->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4rtr2 = rtr2->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4dst1 = dst1->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4dst2 = dst2->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4src2 = src2->GetObject<Ipv4> ();

	//Assign Static IP addresses to the source and the destination
  	int32_t ifsrc = ipv4src->AddInterface (Sdevice);
  	int32_t ifdst1 = ipv4dst1->AddInterface (Ddevice1);
	int32_t ifdst2 = ipv4dst2->AddInterface (Ddevice2);
	int32_t ifsrc2 = ipv4src2->AddInterface (Sdevice2);


  	Ipv4InterfaceAddress ifInAddrsrc = Ipv4InterfaceAddress (Ipv4Address ("172.16.1.1"), Ipv4Mask ("/32"));
  	ipv4src->AddAddress (ifsrc, ifInAddrsrc);
  	ipv4src->SetMetric (ifsrc, 1);
  	ipv4src->SetUp (ifsrc);

  	Ipv4InterfaceAddress ifInAddrdst1 = Ipv4InterfaceAddress (Ipv4Address ("192.168.1.1"), Ipv4Mask ("/32"));
  	ipv4dst1->AddAddress (ifdst1, ifInAddrdst1);
  	ipv4dst1->SetMetric (ifdst1, 1);
  	ipv4dst1->SetUp (ifdst1);

  	Ipv4InterfaceAddress ifInAddrdst2 = Ipv4InterfaceAddress (Ipv4Address ("192.168.2.1"), Ipv4Mask ("/32"));
  	ipv4dst2->AddAddress (ifdst2, ifInAddrdst2);
  	ipv4dst2->SetMetric (ifdst2, 1);
  	ipv4dst2->SetUp (ifdst2);
	
        Ipv4InterfaceAddress ifInAddrsrc2 = Ipv4InterfaceAddress (Ipv4Address ("172.16.2.1"), Ipv4Mask ("/32"));
        ipv4src2->AddAddress (ifsrc2, ifInAddrsrc2);
        ipv4src2->SetMetric (ifsrc2, 1);
        ipv4src2->SetUp (ifsrc2);


	//Enable SoR Advertisement
	//rtr1->SoRAdvertisement();	
	//rtr2->SoRAdvertisement();
/*
	//Create static routing 
	Ipv4StaticRoutingHelper ipv4routinghelper;
	Ptr<Ipv4StaticRouting> st_src = ipv4routinghelper.GetStaticRouting (ipv4src);
	Ptr<Ipv4StaticRouting> st_rtr1 = ipv4routinghelper.GetStaticRouting (ipv4rtr1);
	Ptr<Ipv4StaticRouting> st_rtr2 = ipv4routinghelper.GetStaticRouting (ipv4rtr2);
	Ptr<Ipv4StaticRouting> st_dst1 = ipv4routinghelper.GetStaticRouting (ipv4dst1);
	Ptr<Ipv4StaticRouting> st_dst2 = ipv4routinghelper.GetStaticRouting (ipv4dst2);
	Ptr<Ipv4StaticRouting> st_src2 = ipv4routinghelper.GetStaticRouting (ipv4src2);

	//create static routing table from source to the destination
	//router 1
	st_rtr1->AddHostRouteTo (Ipv4Address ("192.168.1.1"),Ipv4Address ("10.0.2.2"),2);
	st_rtr1->AddHostRouteTo (Ipv4Address ("192.168.2.1"),Ipv4Address ("10.0.2.2"),2);
 	st_rtr1->AddHostRouteTo (Ipv4Address ("10.0.1.1"),Ipv4Address ("10.0.1.1"),1);
	st_rtr1->AddHostRouteTo (Ipv4Address ("10.0.5.1"),Ipv4Address ("10.0.5.1"),3);


	//router 2
	st_rtr2->AddHostRouteTo (Ipv4Address ("192.168.1.1"),Ipv4Address ("10.0.4.2"),2);
	st_rtr2->AddHostRouteTo (Ipv4Address ("192.168.2.1"),Ipv4Address ("10.0.3.2"),3);
	st_rtr2->AddHostRouteTo (Ipv4Address ("10.0.1.1"),Ipv4Address ("10.0.2.1"),1);
	st_rtr2->AddHostRouteTo (Ipv4Address ("10.0.5.1"),Ipv4Address ("10.0.2.1"),1);
	
	//source
	st_src->AddHostRouteTo (Ipv4Address ("192.168.1.1"),Ipv4Address ("10.0.1.2"),1);
	st_src->AddHostRouteTo (Ipv4Address ("192.168.2.1"),Ipv4Address ("10.0.1.2"),1);

 	//source
        st_src2->AddHostRouteTo (Ipv4Address ("192.168.1.1"),Ipv4Address ("10.0.5.2"),1);
        st_src2->AddHostRouteTo (Ipv4Address ("192.168.2.1"),Ipv4Address ("10.0.5.2"),1);

	//Destination 1
	st_dst1->AddHostRouteTo (Ipv4Address ("10.0.1.1"),Ipv4Address ("10.0.4.1"),1);
	st_dst1->AddHostRouteTo (Ipv4Address ("10.0.5.1"),Ipv4Address ("10.0.4.1"),1);

	//Destination 1
	st_dst2->AddHostRouteTo (Ipv4Address ("10.0.1.1"),Ipv4Address ("10.0.3.1"),1);
	st_dst2->AddHostRouteTo (Ipv4Address ("10.0.5.1"),Ipv4Address ("10.0.3.1"),1);
*/	
	//
 	// Create a UdpEchoServer application on node one.
 	//
  	uint16_t port = 9; // well-known echo port number
  	SoREchoServerHelper server (port);
  	ApplicationContainer apps = server.Install (dst1);
  	apps.Start (Seconds (1.0));
  	apps.Stop (Seconds (200));
 	//
 	// Create a UdpEchoClient application to send UDP datagrams from node zero to
 	// node one.
 	//
  	uint32_t packetSize = 1024;
  	uint32_t maxPacketCount = 1;
 	Time interPacketInterval = Seconds (1);
  	//UdpEchoClientHelper client (ifInAddrdst2.GetLocal(), port);
	SoREchoClientHelper client (ifInAddrdst2.GetLocal(), port);
  	client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  	client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  	client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  	apps = client.Install (src);
	client.SetFill (apps.Get (0), "Hello World testing");
  	apps.Start (Seconds (1.2));
  	apps.Stop (Seconds (200));

	//SoREchoServerHelper server2 (port);
        //ApplicationContainer apps2 = server2.Install (dst1);
        //apps2.Start (Seconds (1.0));
        //apps2.Stop (Seconds (10.0));


	/*SoREchoClientHelper client2 (ifInAddrdst1.GetLocal(), port);
        client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
        client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
        client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
        apps = client2.Install (src2);
        client2.SetFill (apps.Get (0), "Hello World");
        apps.Start (Seconds (2.0));
        apps.Stop (Seconds (2.5));
	


        SoREchoClientHelper client3 (ifInAddrdst1.GetLocal(), port);
        client3.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
        client3.SetAttribute ("Interval", TimeValue (interPacketInterval));
        client3.SetAttribute ("PacketSize", UintegerValue (packetSize));
        apps = client3.Install (src2);
        client3.SetFill (apps.Get (0), "Hello World");
        apps.Start (Seconds (2.0));
        apps.Stop (Seconds (2.5));


        SoREchoClientHelper client4 (ifInAddrdst1.GetLocal(), port);
        client4.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
        client4.SetAttribute ("Interval", TimeValue (interPacketInterval));
        client4.SetAttribute ("PacketSize", UintegerValue (packetSize));
        apps = client4.Install (src2);
        client4.SetFill (apps.Get (0), "Hello World");
        apps.Start (Seconds (2.0));
        apps.Stop (Seconds (2.5));*/



	/*uint16_t port = 52000;
	OnOffHelper cbr ("ns3::UdpSocketFactory",Address(InetSocketAddress(ifInAddrdst2.GetLocal(),port)));
	cbr.SetConstantRate (DataRate(6000));
	ApplicationContainer apps = cbr.Install(src);
	apps.Start (Seconds(1.0));
	apps.Stop (Seconds(10000.8));

	PacketSinkHelper sink1 ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (),port)));
	apps = sink1.Install (dst1);
	apps.Start (Seconds(1.0));
        apps.Stop (Seconds(10000.8));

	PacketSinkHelper sink2 ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (),port)));
	apps = sink2.Install (dst2);
	apps.Start (Seconds(1.0));
        apps.Stop (Seconds(10000.8));
	*/
	//p2p.EnablePcapAll("sortesting");
      	Simulator::Stop (Seconds (10));
	Simulator::Run ();
      	Simulator::Destroy ();
      	return 0;
} 

