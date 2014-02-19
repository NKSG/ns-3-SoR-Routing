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
#include "ns3/netanim-module.h"


using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("SRRtsimple");

int main (int argc,char *argv[])
{
	CommandLine cmd;
	cmd.Parse(argc,argv);

//Creating nodes
	Ptr<Node> rtr1 = CreateObject<Node> ();
	Ptr<Node> rtr2 = CreateObject<Node> ();
	Ptr<Node> rtr3 = CreateObject<Node> ();
	Ptr<Node> rtr4 = CreateObject<Node> ();
	
	Ptr<Node> cli = CreateObject<Node> ();
	Ptr<Node> srv = CreateObject<Node> ();

// Enable SoR
	rtr1->SoREnable(1);
	rtr2->SoREnable(1);
	rtr3->SoREnable(1);
	rtr4->SoREnable(1);

	//cli->SoREnable(1);

	NodeContainer a = NodeContainer (rtr1,rtr2,rtr3,rtr4);
	NodeContainer b = NodeContainer (cli,srv);
	NodeContainer c = NodeContainer (a,b);	

//Enable Routing
	InternetStackHelper internet;

	SorRoutingHelper sorrouting;

  	Ipv4ListRoutingHelper list;
  	list.Add (sorrouting, 0);

  	internet.SetRoutingHelper (list);
 	internet.Install (c);

// Creating Point-to-Point Links
	NodeContainer rtr1rtr2 = NodeContainer (rtr1,rtr2);
	NodeContainer rtr1rtr3 = NodeContainer (rtr1,rtr3);
	NodeContainer rtr2rtr3 = NodeContainer (rtr2,rtr3);
	NodeContainer rtr3rtr4 = NodeContainer (rtr3,rtr4);

	NodeContainer clirtr1 = NodeContainer (cli,rtr1);
	NodeContainer srvrtr4 = NodeContainer (srv,rtr4);

	//NodeContainer rtr1rtr4 = NodeContainer (rtr1,rtr4);//******

//create channels to add Point-to-Point links
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer Drtr1rtr2 = p2p.Install (rtr1rtr2);
        NetDeviceContainer Drtr1rtr3 = p2p.Install (rtr1rtr3);
        NetDeviceContainer Drtr2rtr3 = p2p.Install (rtr2rtr3);
        NetDeviceContainer Drtr3rtr4 = p2p.Install (rtr3rtr4);

	//NetDeviceContainer Drtr1rtr4 = p2p.Install (rtr1rtr4);//******

	PointToPointHelper p2p_endhost;
	p2p_endhost.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	p2p_endhost.SetChannelAttribute ("Delay", StringValue ("4ms"));

	NetDeviceContainer Dclirtr1 = p2p_endhost.Install (clirtr1);
        NetDeviceContainer Dsrvrtr4 = p2p_endhost.Install (srvrtr4);
	
//Assign IP address to the links and the node interfaces
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.1.0","255.255.255.0");
	Ipv4InterfaceContainer irtr1rtr2 = ipv4.Assign(Drtr1rtr2);
	ipv4.SetBase("10.0.2.0","255.255.255.0");
	Ipv4InterfaceContainer irtr1rtr3 = ipv4.Assign(Drtr1rtr3);
	ipv4.SetBase("10.0.3.0","255.255.255.0");
        Ipv4InterfaceContainer irtr2rtr3 = ipv4.Assign(Drtr2rtr3);
	ipv4.SetBase("10.0.4.0","255.255.255.0");
        Ipv4InterfaceContainer irtr3rtr4 = ipv4.Assign(Drtr3rtr4);

	//ipv4.SetBase("124.12.15.0","255.255.255.0");//******
        //Ipv4InterfaceContainer irtr1rtr4 = ipv4.Assign(Drtr1rtr4);//******

	ipv4.SetBase("172.16.1.0","255.255.255.0");
        Ipv4InterfaceContainer iclirtr1 = ipv4.Assign(Dclirtr1);
	ipv4.SetBase("192.168.16.0","255.255.255.0");
        Ipv4InterfaceContainer isrvrtr4 = ipv4.Assign(Dsrvrtr4);

	Ptr<Ipv4> ipv4rtr1 = rtr1->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4rtr2 = rtr2->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4rtr3 = rtr3->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4rtr4 = rtr4->GetObject<Ipv4> ();

	Ptr<Ipv4> ipv4cli = cli->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4svr = srv->GetObject<Ipv4> ();


/*SoR application (UDP communication)*/
	//create server  	
	uint16_t port = 9; // well-known echo port number
  	SoREchoServerHelper server (port);
  	ApplicationContainer apps = server.Install (srv);
  	apps.Start (Seconds (1.0));
  	apps.Stop (Seconds (150));


	//create cleint
  	uint32_t packetSize = 1024;
  	uint32_t maxPacketCount = 5;
 	Time interPacketInterval = Seconds (1);
	SoREchoClientHelper client (ipv4svr->GetAddress(1,0).GetLocal(), port);
	//SoREchoClientHelper client ("5.6.7.8", port);
  	client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  	client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  	client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  	apps = client.Install (cli);
	client.SetFill (apps.Get (0), "SoR Routing Testing Simulation");
  	apps.Start (Seconds (100));
  	apps.Stop (Seconds (150));
 
/*end of SoR application*/


/*TCP application*/
	/*uint16_t port = 50000;
	Address sinkLocalAddress (InetSocketAddress (ipv4svr->GetAddress(1,0).GetLocal(), port));
	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
	ApplicationContainer sinkApp = sinkHelper.Install (srv);
	sinkApp.Start (Seconds (1.0));
	sinkApp.Stop (Seconds (120.0));

	// Create the OnOff applications to send TCP to the server
	OnOffHelper clientHelper ("ns3::TcpSocketFactory", InetSocketAddress (ipv4cli->GetAddress(1,0).GetLocal(), port));
	clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
	clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));

	ApplicationContainer clientApps;
	AddressValue remoteAddress(InetSocketAddress (ipv4svr->GetAddress(1,0).GetLocal(), port));
	//AddressValue remoteAddress(InetSocketAddress ("5.6.7.8", port));
	clientHelper.SetAttribute ("Remote", remoteAddress);
	clientApps.Add (clientHelper.Install (cli));
	clientApps.Start (Seconds (10.0));
	clientApps.Stop (Seconds (100.0));
*/
/*end of TCP allication*/



//Run and simulate the simulation script
	//p2p.EnablePcapAll("sortesting");
	Simulator::Schedule (Seconds (25),&Ipv4::SetDown, ipv4rtr1,0);
	Simulator::Schedule (Seconds (50),&Ipv4::SetUp, ipv4rtr1,0);
        //Simulator::Schedule (Seconds (51),&Ipv4::SetDown, ipv4rtr3,1);
        //Simulator::Schedule (Seconds (65),&Ipv4::SetUp, ipv4rtr3,1);
        //Simulator::Schedule (Seconds (30),&Ipv4::SetDown, ipv4rtr1,4);
        //Simulator::Schedule (Seconds (80),&Ipv4::SetUp, ipv4rtr1,4);

      	Simulator::Stop (Seconds (250));
	//AnimationInterface anim ("animation.xml");
	Simulator::Run ();
      	Simulator::Destroy ();
      	return 0;


	
}
