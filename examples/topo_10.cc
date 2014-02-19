#include <iostream>
#include <cassert>
#include <string.h>
#include <sstream>
#include <cstring>
#include <cstdlib>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"

#include "ns3/sor-routing-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("SRRtsimple");

int main (int argc,char *argv[])
{
	CommandLine cmd;
	cmd.Parse(argc,argv);
	std::ostringstream oss;
   
	Ptr<Node> net1Node0 = CreateObject<Node> ();
	Ptr<Node> net1Node1 = CreateObject<Node> ();
	Ptr<Node> net1Node2 = CreateObject<Node> ();
	Ptr<Node> net1Node3 = CreateObject<Node> ();
	Ptr<Node> net1Node4 = CreateObject<Node> ();
	Ptr<Node> net1Node5 = CreateObject<Node> ();
	Ptr<Node> net1Node6 = CreateObject<Node> ();
	Ptr<Node> net1Node7 = CreateObject<Node> ();
	Ptr<Node> net1Node8 = CreateObject<Node> ();
	Ptr<Node> net1Node9 = CreateObject<Node> ();

	Ptr<Node> client = CreateObject<Node> ();
	Ptr<Node> server = CreateObject<Node> ();

	NodeContainer net1cont_temp1 = NodeContainer (net1Node0,net1Node1,net1Node2,net1Node3,net1Node4);
	NodeContainer net1cont_temp2 = NodeContainer (net1Node5,net1Node6,net1Node7,net1Node8,net1Node9);

	NodeContainer clientserver = NodeContainer (client,server);

	NodeContainer net1cont = NodeContainer (net1cont_temp1,net1cont_temp2,clientserver);
	//Enable Routing
	InternetStackHelper internet;
	SorRoutingHelper sorrouting;
	//Ipv4NixVectorHelper nixRouting;
  	Ipv4ListRoutingHelper list;
  	list.Add (sorrouting, 0);
	//list.Add (nixRouting, 0);
  	internet.SetRoutingHelper (list);
	internet.Install (net1cont);	
 	

	NodeContainer* net1nc = new NodeContainer[18];
	for (int j=0;j<9;j++)
	{
		int k = j;
		net1nc[j] = NodeContainer(net1cont.Get(j),net1cont.Get(++k));
	}
	net1nc[9] = NodeContainer(net1cont.Get(9),net1cont.Get(5));
	net1nc[10] = NodeContainer(net1cont.Get(6),net1cont.Get(9));
	net1nc[11] = NodeContainer(net1cont.Get(7),net1cont.Get(5));
	net1nc[12] = NodeContainer(net1cont.Get(7),net1cont.Get(9));
	net1nc[13] = NodeContainer(net1cont.Get(1),net1cont.Get(8));
	net1nc[14] = NodeContainer(net1cont.Get(2),net1cont.Get(8));
	net1nc[15] = NodeContainer(net1cont.Get(3),net1cont.Get(8));
	net1nc[16] = NodeContainer(net1cont.Get(3),net1cont.Get(9));
	net1nc[17] = NodeContainer(net1cont.Get(4),net1cont.Get(9));
	

	//create channels to add Point-to-Point links
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));


	NetDeviceContainer* net1ndc = new NetDeviceContainer[18];
	for (int l=0;l<18;l++)
	{
		net1ndc[l] = p2p.Install (net1nc[l]);
	}

	Ipv4AddressHelper net1address;
	Ipv4AddressHelper net1ipv4;

	Ipv4InterfaceContainer* net1ic =  new Ipv4InterfaceContainer[18];
	for (int k=0;k<18;k++)
	{
		oss.str ("");
          	oss << "10" << ".0." << 10 + k << ".0";
          	net1ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");
		net1ic[k] =  net1ipv4.Assign(net1ndc[k]);
	}

	NodeContainer clirtr = NodeContainer (client,net1cont.Get(0));
	NodeContainer srvrtr = NodeContainer (server,net1cont.Get(4));
   

	//create channels to to endhosts
	PointToPointHelper p2p_endhost;
	p2p_endhost.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	p2p_endhost.SetChannelAttribute ("Delay", StringValue ("1ms"));

	NetDeviceContainer Dclirtr = p2p_endhost.Install (clirtr);
        NetDeviceContainer Dsrvrtr = p2p_endhost.Install (srvrtr);

	net1address.SetBase("172.16.1.0","255.255.255.0");
        Ipv4InterfaceContainer iclirtr = net1address.Assign(Dclirtr);
	net1address.SetBase("192.168.16.0","255.255.255.0");
        Ipv4InterfaceContainer isrvrtr = net1address.Assign(Dsrvrtr);

	Ptr<Ipv4> ipv4cli = client->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv4svr = server->GetObject<Ipv4> ();

  	// Create router nodes, initialize routing database and set up the routing
  	// tables in the nodes.
  	//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

/*SoR application (UDP communication)*/
	//create server  	
	uint16_t port = 9; // well-known echo port number
  	SoREchoServerHelper serverApp (port);
  	ApplicationContainer apps = serverApp.Install (server);
  	apps.Start (Seconds (1.0));
  	apps.Stop (Seconds (1000));


	//create cleint
  	uint32_t packetSize = 512;
  	uint32_t maxPacketCount = 10000;
 	Time interPacketInterval = Seconds (0.25);
	SoREchoClientHelper clientApp (ipv4svr->GetAddress(1,0).GetLocal(), port);
	//SoREchoClientHelper client ("5.6.7.8", port);
  	clientApp.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  	clientApp.SetAttribute ("Interval", TimeValue (interPacketInterval));
  	clientApp.SetAttribute ("PacketSize", UintegerValue (packetSize));
  	apps = clientApp.Install (client);
	clientApp.SetFill (apps.Get (0), "The Service-oriented Router (SoR) is a router proposed to provide content based services for next-generation networks by shifting the current Internet infrastructure to an information-based, open innovation platform. Yet the SoR is still under research and development, a simulation platform is vital to simulate the functions of SoR.  In fact, the NS2-SoR implementation has several limitations, such as; it cannot be used with generic traffic generators, transport protocols and NetDevices.");
  	apps.Start (Seconds (10));
  	apps.Stop (Seconds (1000));
 
/*end of SoR application*/


	Simulator::Stop (Seconds (1050));
	//AnimationInterface anim ("animation.xml");
	Simulator::Run ();
      	Simulator::Destroy ();
      	return 0;
}

