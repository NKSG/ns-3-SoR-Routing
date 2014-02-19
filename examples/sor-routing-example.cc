/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// Network topology
//
//  n0 ----------------- n1
//        5 Mb/s, 2ms
//
// - all links are point-to-point links with indicated one-way BW/delay


#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/sor-routing-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SoRRoutingTestExample");

int 
main (int argc, char *argv[])
{
  
  CommandLine cmd;
  bool enableFlowMonitor = false;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);

  // Here, we will explicitly create four nodes.  In more sophisticated
  // topologies, we could configure a node factory.
  NS_LOG_INFO ("Creating nodes.");
  NodeContainer c;
  c.Create (2);
  NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get (1));

  InternetStackHelper internet;
  // -----------------
  SorRoutingHelper sorrouting;

  Ipv4ListRoutingHelper list;
  list.Add (sorrouting, 100);

  internet.SetRoutingHelper (list);
  // -----------------
  internet.Install (c);

  // ------------ We create the channels first without any IP addressing information ------------
  NS_LOG_INFO ("Creating channels.");
  PointToPointHelper p2p;
  
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);

  // ------------ Later, we add IP addresses. ------------
  NS_LOG_INFO ("Assigning IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (50));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");

  Simulator::Destroy ();
  return 0;
}
