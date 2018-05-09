/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "broadcast-strategy.hpp"
#include "algorithm.hpp"
#include <ndn-cxx/lp/tags.hpp>

#include "ns3/net-device-container.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"

// For energy consumption
#include "ns3/energy-source-container.h"
#include "ns3/energy-module.h"
#include "ns3/li-ion-energy-source.h"
#include "src/energy/helper/li-ion-energy-source-helper.h"

NFD_LOG_INIT("BroadcastStrategy");

namespace nfd {
namespace fw {

  //const Name BroadcastStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/broadcast/%FD%02");
NFD_REGISTER_STRATEGY(BroadcastStrategy);

BroadcastStrategy::BroadcastStrategy(Forwarder& forwarder,const Name& name): Strategy(forwarder)
{
  //string file("/energy_level");
/*
  shared_ptr<std::ofstream> os(new std::ofstream());
  m_os = os;
  m_os->open(m_file.c_str(), std::ios_base::out | std::ios_base::trunc);
  if (!m_os->is_open()) {
      NS_LOG_ERROR("File " << m_file << " cannot be opened for writing. Tracing disabled");
      return;
    }
  *m_os << "Time"
     << ","
     << "Node"
     << ","
     << "HopCount"//"SeqNo"
     << ","
     << "EnergyRemaining"
     << ","
     << "EnergyPercent"
     << "\n";
*/
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("BroadcastStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
                                                "BroadcastStrategy does not support version " + std::to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));

  //shared_ptr<std::fstream> os(new std::fstream());
//  shared_ptr<std::fstream> os(new std::fstream());
  std::shared_ptr<std::ofstream> os(new std::ofstream());
  os->open(m_file.c_str(), std::ios_base::out | std::ios_base::trunc);
  //os->open(m_file.c_str(), std::fstream::out | std::fstream::app);
  //os->open(m_file.c_str(), std::fstream::out | std::fstream::app);
  
  //m_os = os;
  if (!os->is_open()) {
      std::cerr << "NOT!!! Writing into energy file"<<m_file<< std::endl;
      NS_LOG_ERROR("File " << m_file << " cannot be opened for writing. Tracing disabled");
      return;
    }
   //m_os = os;
   //std::cerr << "Writing into energy file"<<m_file<< std::endl;
  *os << "Time"
     << ","
     << "Node"
     << ","
     << "HopCount"//"SeqNo"
     << ","
     << "EnergyRemaining"
     << ","
     << "EnergyPercent\n";
     //std::cerr << "STARTED into energy file"<<m_file<< std::endl;

}

const Name&
BroadcastStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/broadcast/%FD%02");
  return strategyName;
}
  
void BroadcastStrategy::energyTracer(int nodeId, int hopCount, double nodeCurrentPowerA)
{
    std::shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(m_file.c_str(), std::ios_base::out | std::ios_base::app);

                if (!os->is_open()) {
                std::cerr << "VALUES NOT!!! Writing into energy file"<<m_file<< std::endl;
                NS_LOG_ERROR("File " << m_file << " cannot be opened for writing. Tracing disabled");
                //return;
                }
             else{
                //std::cerr << "inner core\n";
                *os << ns3::Simulator::Now().ToDouble(ns3::Time::S) //"Time"
                  << ","
                  << nodeId//"Node"
                  << "," //
                  << hopCount //"Hopcount"//
                  << ","
                  << nodeCurrentPowerA//"EnergyRemaining"
                  << ","
                  << (int)((nodeCurrentPowerA/m_initPowerLevelA)*100)
                  << "\n";
                }

}
  
void
BroadcastStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,const shared_ptr<pit::Entry>& pitEntry)
{
const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  
  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());  
  ns3::Ptr<ns3::NetDevice> netDevice;
  ns3::Ptr<ns3::ndn::L3Protocol> ndnl3;
  ndnl3 = node->GetObject<ns3::ndn::L3Protocol>();
  
  // Get HopCount
  int hopCount = 0;
  auto hopCountTag = interest.getTag<lp::HopCountTag>();
  if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
    hopCount = *hopCountTag;
  }
  
  ns3::Ptr<ns3::EnergySourceContainer> EnergySourceContainerOnNode = node->GetObject<ns3::EnergySourceContainer> ();
  
  if (EnergySourceContainerOnNode == NULL){
	  // Do nothing (probably a wired node)
  }
  else{
	  ns3::Ptr<ns3::LiIonEnergySource> es = ns3::DynamicCast<ns3::LiIonEnergySource> (EnergySourceContainerOnNode->Get(0));
	  m_nodeCurrentPowerA = es->GetRemainingEnergy () / (3.6 * 3600);
	  
	  NFD_LOG_DEBUG("At " << ns3::Simulator::Now ().GetSeconds () << 
					" Node Id: " << node->GetId () << 
					" Number of NetDevices: " << node->GetNDevices() <<
					" Remaining Capacity: " << es->GetRemainingEnergy () / (3.6 * 3600) << " Ah");

      /* log Energy Level*/
//          std::cerr << "current Time:"<<ns3::Simulator::Now().ToDouble(ns3::Time::MS)<<"\n";
          //std::shared_ptr<std::ofstream> os(new std::ofstream());
          //os->open(m_file.c_str(), std::ios_base::out | std::ios_base::app);
          //std::cerr << "current energy:"<<m_nodeCurrentPowerA<<" | B4 energy if into energy file:"<<(int)((m_nodeCurrentPowerA/m_initPowerLevelA)*100)<< std::endl;
          if((int)(ns3::Simulator::Now().ToDouble(ns3::Time::US)) % (int)60e6 == 0){
          ns3::Simulator:: Schedule (ns3::Seconds(0), &BroadcastStrategy::energyTracer, this, (int)node->GetId (), hopCount, m_nodeCurrentPowerA);
          }
          /*if((int)((m_nodeCurrentPowerA/m_initPowerLevelA)*100)%10 == 0)
          {
             if (!os->is_open()) {
                std::cerr << "VALUES NOT!!! Writing into energy file"<<m_file<< std::endl;
                NS_LOG_ERROR("File " << m_file << " cannot be opened for writing. Tracing disabled");
                //return;
                }
             else{
                std::cerr << "inner core\n";
                *os << ns3::Simulator::Now().ToDouble(ns3::Time::S) //"Time"
                  << ","
                  << node->GetId ()//"Node"
                  << "," //
                  << hopCount //"Hopcount"//
                  << ","
                  << m_nodeCurrentPowerA//"EnergyRemaining"
                  << ","
                  << (int)((m_nodeCurrentPowerA/m_initPowerLevelA)*100)
                  << "\n";
                }
          }*/
      /**
	   *  Start the algorithm here  
	   */
	   
	  if (m_nodeCurrentPowerA <= m_energyTh2 && m_nodeCurrentPowerA > 0){
		  // forward to LTE
		  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
			Face& outFace = std::next(it)->getFace();
			
			NFD_LOG_DEBUG("OutFace is: " << outFace);
		
			if (!wouldViolateScope(inFace, interest, outFace)) {
			  this->sendInterest(pitEntry, outFace, interest);
			}
			break;
		  }
	  }
	  else if (hopCount == 0){
		  auto retxTag = interest.getTag<lp::RetxTag>();
		  NFD_LOG_DEBUG( "Node Id: " << node->GetId () << " Retx Tag is: " << *retxTag);
		  if (*retxTag == 0){
			  // forward to WiFi
			  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
				Face& outFace = it->getFace();
				NFD_LOG_DEBUG("OutFace is: " << outFace);
			
				if (!wouldViolateScope(inFace, interest, outFace)) {
				  this->sendInterest(pitEntry, outFace, interest);
				}
				break;
			  }
		  } else{
			  // forward to LTE
			  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
				Face& outFace = std::next(it)->getFace();
				
				NFD_LOG_DEBUG("OutFace is: " << outFace);
			
				if (!wouldViolateScope(inFace, interest, outFace)) {
				  this->sendInterest(pitEntry, outFace, interest);
				}
				break;
			  }
		  }
	  }
	  else if (m_nodeCurrentPowerA <= m_energyTh1){	  // node is a router
		if (hopCount > 3)
			return;
		else{
			  // only forward if hopCount <= 3
			  // forward to WiFi
			  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
				Face& outFace = it->getFace();
				
				NFD_LOG_DEBUG("OutFace is: " << outFace);
			
				if (!wouldViolateScope(inFace, interest, outFace)) {
				  this->sendInterest(pitEntry, outFace, interest);
				}
				break;
			  }
		}
	  }
	  else{
		if(hopCount > 5)
			return;
		else{
		  // only forward if hopCount <= 5
		  // forward to WiFi
		  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
			Face& outFace = it->getFace();
			
			NFD_LOG_DEBUG("OutFace is: " << outFace);
		
			if (!wouldViolateScope(inFace, interest, outFace)) {
			  this->sendInterest(pitEntry, outFace, interest);
			}
			break;
		  }
		}
	  }
  
  }
}

} // namespace fw
} // namespace nfd
 

