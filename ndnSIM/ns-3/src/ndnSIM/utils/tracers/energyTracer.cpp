/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "energyTracer.hpp"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/callback.h"

#include "apps/ndn-app.hpp"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "ns3/mobility-module.h"
#include "ns3/constant-velocity-mobility-model.h"

#include <math.h> 
#include <fstream>

//#define CANTOR 5000

// For energy consumption
#include "ns3/energy-source-container.h"
#include "ns3/energy-module.h"
#include "ns3/li-ion-energy-source.h"
#include "src/energy/helper/li-ion-energy-source-helper.h"

NS_LOG_COMPONENT_DEFINE("ndn.energyTracer");

namespace ns3 {
namespace ndn {

static std::list<std::tuple<shared_ptr<std::ostream>, std::list<Ptr<energyTracer>>>>
  g_tracers;

void
energyTracer::Destroy()
{
  g_tracers.clear();
}

void
energyTracer::InstallAll(const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<energyTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<energyTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
energyTracer::Install(const NodeContainer& nodes, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<energyTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
    Ptr<energyTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
energyTracer::Install(Ptr<Node> node, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<energyTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  Ptr<energyTracer> trace = Install(node, outputStream);
  tracers.push_back(trace);

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

Ptr<energyTracer>
energyTracer::Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream)
{
  NS_LOG_DEBUG("Node: " << node->GetId());

  Ptr<energyTracer> trace = Create<energyTracer>(outputStream, node);

  return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

energyTracer::energyTracer(shared_ptr<std::ostream> os, Ptr<Node> node)
  : m_nodePtr(node)
  , m_os(os)
{
  m_node = boost::lexical_cast<std::string>(m_nodePtr->GetId());
  ns3::Ptr<ns3::EnergySourceContainer> EnergySourceContainerOnNode = m_nodePtr->GetObject<ns3::EnergySourceContainer> ();
  
  if (EnergySourceContainerOnNode == NULL){
    // Do nothing (probably a wired node)
  }
  else{
    ns3::Ptr<ns3::LiIonEnergySource> es = ns3::DynamicCast<ns3::LiIonEnergySource> (EnergySourceContainerOnNode->Get(0));
    double nodeCurrentPowerA = es->GetRemainingEnergy () / (3.6 * 3600);
    m_energyLevel=((nodeCurrentPowerA/m_initPowerLevelA)*100);
    //m_velocity=mob->GetVelocity().x+mob->GetVelocity().y; 
  Connect();

  std::string name = Names::FindName(node);
  if (!name.empty()) {
    m_node = name;
        }
    }
}

energyTracer::energyTracer(shared_ptr<std::ostream> os, const std::string& node)
  : m_node(node)
  , m_os(os)
{
  Connect();
}

energyTracer::~energyTracer(){};

void
energyTracer::Connect()
{
  Config::ConnectWithoutContext("/NodeList/" + m_node
                                  + "/ApplicationList/*/LastRetransmittedInterestDataDelay",
                                MakeCallback(&energyTracer::LastRetransmittedInterestDataDelay,
                                             this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/FirstInterestDataDelay",
                                MakeCallback(&energyTracer::FirstInterestDataDelay, this));
}

void
energyTracer::PrintHeader(std::ostream& os) const
{
  os << "Time"
     << ","
     << "Node"
     << ","
     << "AppId"
     << ","
     << "SeqNo"
     << ","
     << "Type"
     << ","
     //<<"dist"
     //<<","
     << "DelayS"
     << ","
     << "DelayUS"
     << ","
     << "RetxCount"
     << ","
     << "HopCount"
     << ","
     << "PowerLevel"
     << "";
}

void
energyTracer::LastRetransmittedInterestDataDelay(Ptr<App> app, uint32_t seqno, Time delay,
                                                   int32_t hopCount)
{
  *m_os << Simulator::Now().ToDouble(Time::S) << "," << m_node << "," << app->GetId() << ","
        << seqno << ","
        << "LastDelay"//<< "," << m_dist 
        <<"," <<delay.ToDouble(Time::S) << "," << delay.ToDouble(Time::US) << "," << 1 << ","
        << hopCount << ","<< m_energyLevel<< "\n";//m_velocity<<"\n";
}

void
energyTracer::FirstInterestDataDelay(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount,
                                       int32_t hopCount)
{
  *m_os << Simulator::Now().ToDouble(Time::S) << "," << m_node << "," << app->GetId() << ","
        << seqno << ","
        << "FullDelay" //<< "," << m_dist
        <<"," << delay.ToDouble(Time::S) << "," << delay.ToDouble(Time::US) << "," << retxCount
        << "," << hopCount << ","<< m_energyLevel<< "\n";//m_velocity<< "\n";
}

} // namespace ndn
} // namespace ns3

