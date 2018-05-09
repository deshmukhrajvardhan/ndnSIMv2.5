/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 *  This scenario deploys 25 pedestrian nodes and 25 vehicular nodes
 *  (50 total) in a 500 x 500 m2 Manhattan Grid. Each node has two WiFi 
 *  interfaces, one with a transmission range of approximately 100 m 
 *  and another interface that simulates an LTE link (higher 
 *  transmission range and lower latency). The primary interface is 
 *  the WiFi, switching to the LTE interface upon a retransmission 
 *  timeout event.
 * 
 *  All Pedestrian and Vehicular nodes in the simulation act as 
 *  Consumers, as well as content custodians (via caching). Data
 *  repositories disseminate content to the MANET in the first 100 s, 
 *  thus, this period is excluded from the analysis.
 * 
 *  Consumer Behavior:
 *  Consumer application request data following the Zipf distribution,
 *  with rank (q) 0.7 and power (s) 0.7 (default values). The content
 *  space is 1,000,000 chunks of 1,024 kB each (1 GB).
 *  Each consumer requests data on a uniformly distributed interpacket
 *  interval, with a maximum number of content of 50,000 (10 MB)
 *  Additionally, each consumer can hold up to 10,000 contents
 *  in its cache (10 MB).
 *  
 *  Producer behavior:
 *  - Data Repositories
 *  Data repositories disseminate data during simulation runtime.
 *  
 *  - Radio Access Network (RAN)
 *  The RAN is composed of a base station (eNB) and two nodes, with one
 *  being the Producer.
 * 
 *  Bonnmotion tool parameters for mobility files:
 *  ./bm -f manhattan-25-vehicle-0 ManhattanGrid -n 25 -x 500 -y 500 -u 5 -v 5 -m 13 -s 2 -d 4100 -i 3600 -b 0
 *  ./bm -f manhattan-25-pedestrian-0 ManhattanGrid -n 25 -x 500 -y 500 -u 5 -v 5 -m 1 -s 1 -d 4100 -i 3600 -b 0
 * 
 *  where,
 *   -n: number of nodes,
 *   -x, y: width and height of simulation area,
 *   -u, v: number of blocks along x and y axis,
 *   -m: mean speed in m/s,
 *   -s: speed standard deviation,
 *   -d: duration of the simulation,
 *   -i: cut off seconds at the beginning (all nodes start at (0,0)),
 *   -b: border (NS-2 simulation crash when nodes approach the border. No need in NS-3)
 * 
 * 
 *  To run this scenario:
 *  ./waf --run "scratch/ndn-offloading-twoInterfaces"
 * 
 *  Enable logging:
 *  NS_LOG=ndn.Consumer:ndn.Consumer ./waf --run "scratch/ndn-offloading-twoInterfaces"
 * 
 *  Optional parameters: (to use different mobility files)
 *  ./waf --run "scratch/ndn-offloading-twoInterfaces \
 *  --tracePed=./traces/manhattan-25-pedestrian-0.ns_movements \
 *  --traceVeh=./traces/manhattan-25-vehicle-0.ns_movements \ 
 *  --dataDepotStop=4100.0" 
 * 
 * 
 */
#include "ns3/core-module.h"

#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-producer.hpp"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/helper/ndn-app-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include <ns3/ndnSIM/helper/ndn-global-routing-helper.hpp>
#include "ns3/animation-interface.h"
#include "ns3/point-to-point-helper.h"
#include "utils/tracers/energyTracer.hpp"

#include <algorithm>
#include <vector>

#include "ns3/energy-module.h"
#include "ns3/li-ion-energy-source.h"
#include "src/energy/helper/li-ion-energy-source-helper.h"
#include "ns3/config-store-module.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("NDN-Offloading");

int
main(int argc, char* argv[])
{
  uint32_t pedestrianNodes = 25;
  uint32_t vehicleNodes = 25;
  uint32_t dataDepot = 4;
  double dataDepotStopTime = 0.0;
  double simTime = 2000.0; // the first 100 s are to populate the caches 
  
  // Values for random number generation
  const int range_from  = 5;
  const int range_to    = 100;
  const int DEFAULT_ENERGY = 31752;
  std::random_device                  rand_dev;
  std::mt19937                        generator(rand_dev());
  std::uniform_int_distribution<int>  distr(range_from, range_to);
  
  // 802.11 Parameters
  double m_txp = 15.0;
  //double m_txp2 = 30.0; // for LTE
  std::string phyMode = "ErpOfdmRate6Mbps";
  
  // Trace files
  std::string m_traceFilePed("./traces/25Nodes/manhattan-25-pedestrian-0.ns_movements");
  std::string m_traceFileVeh("./traces/25Nodes/manhattan-25-vehicle-0.ns_movements");  

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("pedNodes", "Number of pedestrian nodes ", pedestrianNodes);
  cmd.AddValue ("vehNodes", "Number of vehicular nodes ", vehicleNodes);
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("dataDepotStop", "Time at which the data repositories stop ", dataDepotStopTime);
  cmd.AddValue ("tracePed", "Pedestrian trace file ", m_traceFilePed);
  cmd.AddValue ("traceVeh", "Vehicles trace file ", m_traceFileVeh);
  cmd.Parse(argc, argv);
  
  std::cerr << "Total simulation time is " << simTime << "s" << std::endl;  // for debug
  
  // 802.11 configuration
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200")); 
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));  
  
  // Set maximum value of RTO to 4 seconds
  //Config::SetDefault ("ns3::ndn::RttEstimator::MaxRTO", TimeValue (Seconds (4.0)));
   
  //
  // Create node containers
  //
  // Mobile User Equipment (MUE)
  NodeContainer pedestrians;
  NodeContainer vehicles;
  pedestrians.Create(pedestrianNodes);
  vehicles.Create(vehicleNodes);  
  
  // Data Repositories (Data Depot)
  NodeContainer dtdepot; 
  dtdepot.Create(dataDepot); 
  
  // Create the base station (eNodeB)
  NodeContainer enbNodes;
  enbNodes.Create(1);  
  
  // Create a Packet Data Network Gateway (PGW)
  NodeContainer pgwNodes;
  pgwNodes.Create(1);
  
  // Create a single RemoteHost (Producer)
  NodeContainer remoteHostNodes;
  remoteHostNodes.Create (1);
  
  
  //
  // Configure PHY layer 1 - WiFi
  //
  WifiHelper wifi1 = WifiHelper::Default();
  wifi1.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi1.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
				 "DataMode", StringValue (phyMode), 
				 "ControlMode", StringValue (phyMode));

  YansWifiChannelHelper wifiChannel1; // = YansWifiChannelHelper::Default ();
  wifiChannel1.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel1.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                  "Exponent", DoubleValue (3.0));

  YansWifiPhyHelper wifiPhy1Helper = YansWifiPhyHelper::Default();
  wifiPhy1Helper.SetChannel (wifiChannel1.Create ());
  wifiPhy1Helper.Set("ChannelNumber",UintegerValue(6));
  wifiPhy1Helper.Set("TxPowerStart", DoubleValue(m_txp)); //Minimum available transmission level (dbm)
  wifiPhy1Helper.Set("TxPowerEnd", DoubleValue(m_txp)); //Maximum available transmission level (dbm)

  // Configure MAC layer
  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default();
  wifiMacHelper.SetType ("ns3::AdhocWifiMac");   // Set to adhoc mode
  
  //Node wifi Device for energy model link to device
  NodeContainer EnergyConsumerNodes;
  EnergyConsumerNodes.Add(pedestrians);
  EnergyConsumerNodes.Add(vehicles);
  // Install WiFi-1 on MUE and Data Depot
  NetDeviceContainer wifiNetDevices = wifi1.Install(wifiPhy1Helper, wifiMacHelper, EnergyConsumerNodes);
  //wifiNetDevices = wifi1.Install(wifiPhy1Helper, wifiMacHelper, vehicles);
  NetDeviceContainer dataDepotNetDevices = wifi1.Install(wifiPhy1Helper, wifiMacHelper, dtdepot);
  
  std::cerr << "Finished installing WiFi (Ch.6)" << std::endl;  // for debug
  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  LiIonEnergySourceHelper liionSourceHelper;
  EnergySourceContainer sources;
  // configure energy source
  for (uint32_t i = 0; i < EnergyConsumerNodes.GetN(); i++){
	  liionSourceHelper.Set ("LiIonEnergySourceInitialEnergyJ", DoubleValue (distr(generator) * DEFAULT_ENERGY / 100.0));
	  sources.Add(liionSourceHelper.Install (EnergyConsumerNodes.Get(i)));
  }
  

  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  //radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer wifideviceModels = radioEnergyHelper.Install (wifiNetDevices, sources);
 /***************************************************************************/

  
  //
  // Configure PHY layer 2 - LTE
  //
  /*
  WifiHelper wifi2 = WifiHelper::Default();
  wifi2.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi2.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
								"DataMode", StringValue (phyMode), 
								"ControlMode", StringValue (phyMode));

  YansWifiChannelHelper wifiChannel2; // = YansWifiChannelHelper::Default ();
  wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel2.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                 "Exponent", DoubleValue (3.0));

  YansWifiPhyHelper wifiPhy2Helper = YansWifiPhyHelper::Default();
  wifiPhy2Helper.SetChannel (wifiChannel2.Create ());
  wifiPhy1Helper.Set("ChannelNumber",UintegerValue(11));
  wifiPhy2Helper.Set("TxPowerStart", DoubleValue(m_txp2)); //Minimum available transmission level (dbm)
  wifiPhy2Helper.Set("TxPowerEnd", DoubleValue(m_txp2)); //Maximum available transmission level (dbm)
  
  // Install WiFi-2 on MUE and eNB
  NetDeviceContainer lteNetDevices = wifi2.Install(wifiPhy2Helper, wifiMacHelper, pedestrians);
  lteNetDevices = wifi2.Install(wifiPhy2Helper, wifiMacHelper, vehicles);
  lteNetDevices = wifi2.Install(wifiPhy2Helper, wifiMacHelper, enbNodes);
   
  std::cerr << "Finished installing WiFi (Ch.11) - LTE" << std::endl;  // for debug
  */
  
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", StringValue ("25Mbps"));
  p2ph.SetChannelAttribute ("Delay", StringValue ("50ms"));
  
  NodeContainer nc;
  NetDeviceContainer netDevC;
  for (uint32_t i = 0; i < pedestrians.GetN(); i++){
	  nc = NodeContainer (pedestrians.Get (i), enbNodes.Get (0));
	  netDevC = p2ph.Install (nc);
  }
  
  for (uint32_t i = 0; i < vehicles.GetN(); i++){
	  nc = NodeContainer (vehicles.Get (i), enbNodes.Get (0));
	  netDevC = p2ph.Install (nc);
  }
  
  std::cerr << "Finished installing P2P Links (LTE)" << std::endl;  // for debug
  
  // P2P links between eNB, PGW, and RemoteHost (Producer)
  Ptr<Node> remoteHost = remoteHostNodes.Get (0);
  Ptr<Node> pgw = pgwNodes.Get (0);
  Ptr<Node> enB = enbNodes.Get (0);
  
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.050)));
  NetDeviceContainer lteDevices = p2p.Install (pgw, remoteHost);
  lteDevices = p2p.Install (pgw, enB);

  //
  // Configure Nodes position
  //
  MobilityHelper mobility;
  
  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper pedestrianMobility = Ns2MobilityHelper (m_traceFilePed);
  Ns2MobilityHelper vehicleMobility = Ns2MobilityHelper (m_traceFileVeh);
  
  // Install Mobility Model on MUE
  pedestrianMobility.Install (pedestrians.Begin (), pedestrians.End ());
  vehicleMobility.Install (vehicles.Begin (), vehicles.End ());  
  
  // Install Mobility Model on the eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();  
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbPositionAlloc->Add (Vector(250.0, 250.0, 0.0));
  mobility.SetPositionAllocator(enbPositionAlloc);
  mobility.Install(enbNodes);   
  
  // Install Mobility Model on the PGW
  Ptr<ListPositionAllocator> pgwPositionAlloc = CreateObject<ListPositionAllocator> ();  
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  pgwPositionAlloc->Add (Vector(250.0, 260.0, 0.0));
  mobility.SetPositionAllocator(pgwPositionAlloc);
  mobility.Install(pgwNodes);  
  
  // Install Mobility Model on the RemoteHost
  Ptr<ListPositionAllocator> remHostPositionAlloc = CreateObject<ListPositionAllocator> ();  
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  remHostPositionAlloc->Add (Vector(250.0, 270.0, 0.0));
  mobility.SetPositionAllocator(remHostPositionAlloc);
  mobility.Install(remoteHostNodes);
  
  // Configure position of Data Depot nodes
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
			  	 "MinX", DoubleValue (125.0), 
			  	 "MinY", DoubleValue (125.0),
			  	 "DeltaX", DoubleValue (250.0),
			  	 "DeltaY", DoubleValue (250.0),
			  	 "GridWidth", UintegerValue (2),
			  	 "LayoutType", StringValue ("RowFirst"));
  mobility.Install(dtdepot);
  
  std::cerr << "Finished installing Mobility" << std::endl;  // for debug
  
  //
  // Install NDN stack on nodes
  //
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  
  // Configure Cache
  // eNB, PGW, and Producer do not cache
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache"); // cache size (10k entries)
  ndnHelper.Install(enbNodes);
  ndnHelper.Install(pgwNodes);
  ndnHelper.Install(remoteHostNodes);
  
  // Data Repositories have larger cache 100 MB
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "15000"); // cache size (100k entries)
  ndnHelper.Install(dtdepot);
  
  // MUE have 10 MB caches (10,000 x 1,024 B)
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000"); // cache size (10k entries)
  ndnHelper.Install(pedestrians);
  ndnHelper.Install(vehicles);
  
  std::cerr << "Finished installing NDN Stack" << std::endl;  // for debug
  
  // Choosing forwarding strategy - This is our custom Broadcast Strategy
  ndn::StrategyChoiceHelper::Install(pedestrians,"/", "/localhost/nfd/strategy/broadcast");//multicast");
  ndn::StrategyChoiceHelper::Install(vehicles,"/", "/localhost/nfd/strategy/broadcast");//multicast");
  ndn::StrategyChoiceHelper::Install(enbNodes,"/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install(pgwNodes,"/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install(remoteHostNodes,"/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install(dtdepot,"/", "/localhost/nfd/strategy/broadcast");//multicast");

  std::cerr << "Finished installing Strategy" << std::endl;  // for debug
    
  //
  // Install NDN applications
  //
  std::string ndnPrefix = "/ndn/prefix";
  
  //
  // Install Consumer application
  //  
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("s", StringValue ("1.4"));
  consumerHelper.SetAttribute("Frequency", StringValue ("50"));
  //consumerHelper.SetAttribute("Randomize", StringValue ("uniform"));
  // Number of different content (sequence numbers) that will be requested by the applications
  //consumerHelper.SetAttribute("MaxSeq", StringValue ("50000")); // 50,000
  // Number of the Contents in total
  consumerHelper.SetAttribute("NumberOfContents", StringValue ("40000")); // total number of contents (1M)  
  consumerHelper.SetPrefix(ndnPrefix);
  consumerHelper.Install(pedestrians); 
  consumerHelper.Install(vehicles);
  
  std::cerr << "Finished installing Consumer App" << std::endl;  // for debug
  
  //
  // Install Producer Aplication
  //
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(ndnPrefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(remoteHost);
  
  std::cerr << "Finished installing Producer App" << std::endl;  // for debug
  
  
  // Turn off Wifi Interface from data depot (getting node out of the topology)
  Ptr<WifiNetDevice> wifidevice;
  Ptr<WifiPhy> wifiphy;
  for (uint32_t i = 0; i < 4; i++){
	  wifidevice = DynamicCast<WifiNetDevice> (dataDepotNetDevices.Get(i));
	  wifiphy = wifidevice->GetPhy();
	  Simulator::Schedule(Seconds(dataDepotStopTime), &WifiPhy::SetSleepMode, wifiphy);
  }  
  std::cerr << "Data Repos will stop at t = " << dataDepotStopTime << "s" << std::endl;
  
  
  Simulator::Stop(Seconds(simTime));
  
  // Tracers
  ns3::ndn::energyTracer::InstallAll("./scratch/app-delays-trace.txt");
  //ndn::AppDelayTracer::InstallAll("./scratch/app-delays-trace.txt");
  ndn::CsTracer::Install(pedestrians, "./scratch/cs-trace-ped.txt", Seconds(1.0));
  ndn::CsTracer::Install(vehicles, "./scratch/cs-trace-veh.txt", Seconds(1.0));
  ndn::CsTracer::Install(dtdepot, "./scratch/cs-trace-dataRepo.txt", Seconds(1.0));
  //ndn::L3RateTracer::Install(pedestrians, "./scratch/consumerL3Trace.txt", Seconds(1.0));
  //ndn::L3RateTracer::Install(vehicles, "./scratch/producerL3Trace.txt", Seconds(1.0)); 
  
  std::cerr << "Finished installing Tracers" << std::endl;  // for debug
  
  Simulator::Run();  
  Simulator::Destroy();
  
  return 0;

}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}

