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

#ifndef NFD_DAEMON_FW_BROADCAST_STRATEGY_HPP
#define NFD_DAEMON_FW_BROADCAST_STRATEGY_HPP

#include "strategy.hpp"
#include "ns3/ptr.h"
#include <ns3/nstime.h>

namespace nfd {
namespace fw {

/** \brief a forwarding strategy that forwards Interest to all FIB nexthops
 */
class BroadcastStrategy : public Strategy
{
public:
  explicit
  BroadcastStrategy(Forwarder& forwarder,const Name& name= getStrategyName());

  static const Name& getStrategyName();
  
  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void energyTracer(int nodeId, int hopCount, double nodeCurrentPowerA);

public:
  static const Name STRATEGY_NAME;

private:
  const double m_initPowerLevelA = 2.45;//4.2; // Initial power level in nodes
  const double m_energyTh1 = m_initPowerLevelA * 0.35; // Energy threshold 1 (35%)
  const double m_energyTh2 = m_initPowerLevelA * 0.2; // Energy threshold 2 (20%)
  double m_nodeCurrentPowerA = 0.0;
  std::string m_file={"/VM2_D2D_ndnSIM/ndnSIM/ns-3/scratch/energy_level.txt"};//{"./../../../../../scratch/energy_level"};
  shared_ptr<std::fstream> m_os ;//= new std::ofstream();
  //m_os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);
  //shared_ptr<std::ostream> m_os;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_BROADCAST_STRATEGY_HPP

