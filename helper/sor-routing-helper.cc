/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "sor-routing-helper.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/sor-routing.h"

namespace ns3 {

SorRoutingHelper::SorRoutingHelper () : Ipv4RoutingHelper ()
{
      m_agentFactory.SetTypeId ("ns3::sorrouting::RoutingProtocol"); 
}

SorRoutingHelper::~SorRoutingHelper ()
{
}

SorRoutingHelper* SorRoutingHelper::Copy (void) const
{
      return new SorRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol> SorRoutingHelper::Create (Ptr<Node> node) const
{
      Ptr<sorrouting::RoutingProtocol> agent = m_agentFactory.Create<sorrouting::RoutingProtocol> ();
      node->AggregateObject (agent);
      return agent;
}

void SorRoutingHelper::Set (std::string name, const AttributeValue &value)
{
      m_agentFactory.Set (name, value);
}

} // END OF ns3
