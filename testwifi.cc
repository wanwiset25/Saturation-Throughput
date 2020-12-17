#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/qos-txop.h"
#include "ns3/wifi-mac.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/wifi-net-device.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ABCD");

void runSim(double simulationTime, uint32_t nWifi, uint32_t payloadSize, uint32_t minCw, uint32_t maxCw, bool verbose, bool tracing){
  //uint32_t slot = 50; //slot time in microseconds
  //uint32_t sifs = 28; //SIFS duration in microseconds

  std::cout << "nWifi=" << nWifi << '\n';
  std::cout << "payloadSize=" << payloadSize << '\n';
  std::cout << "minCw=" << minCw << '\n';
  std::cout << "maxCw=" << maxCw << '\n';
  std::cout << "simulationTime=" << simulationTime << '\n';

  if (verbose)
    {
      LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
      LogComponentEnable ("Txop", LOG_LEVEL_INFO);
      LogComponentEnable ("MacLow", LOG_LEVEL_INFO);
      LogComponentEnable ("WifiPhy", LOG_LEVEL_ALL);
    }

  //create nodes
  NodeContainer wifiApNode;
  wifiApNode.Create(1);
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  //create phy, channel, and mac
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel", "Speed", StringValue("1000000"));
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b); //modified wifi-mac.cc to match 802.11 standards
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
				"DataMode", StringValue("DsssRate1Mbps")); //ch datarate 1 Mbps

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::AdhocWifiMac",
	       "Ssid", SsidValue (ssid));

  //install phy,mac to devices
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  //Once installation is done, we overwrite the standard timing values
  //Code unused, directly modify wifi-mac.cc instead
  //Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Slot", TimeValue (MicroSeconds (slot)));
  //Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Sifs", TimeValue (MicroSeconds (sifs)));

  //set min,max CW
  Ptr<NetDevice> dev = wifiApNode.Get (0) -> GetDevice (0);
  Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
  Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
  PointerValue ptr;
  Ptr<Txop> dca;
  wifi_mac->GetAttribute ("Txop", ptr);
  dca = ptr.Get<Txop> ();
  dca-> SetMinCw(minCw);
  dca-> SetMaxCw(maxCw);

  for (uint32_t i = 0; i < nWifi; i++)
    {
        Ptr<NetDevice> dev = wifiStaNodes.Get (i) -> GetDevice (0);
  	Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
  	Ptr<WifiMac> wifi_mac = wifi_dev->GetMac ();
	PointerValue ptr;
  	Ptr<Txop> dca;
  	wifi_mac->GetAttribute ("Txop", ptr);
  	dca = ptr.Get<Txop> ();
  	dca-> SetMinCw(minCw);
  	dca-> SetMaxCw(maxCw);
    }

  //set position of devices, AP in the center, stations circle around AP
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  float rho = 1;
  float pi = 3.14159265;
  for (uint32_t  i=0;i<nWifi;i++){
	double theta = i*2*pi/nWifi;
  	positionAlloc->Add (Vector (rho*cos(theta), rho*sin(theta), 0.0));
  }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  //set IP addresses
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiStaInterfaces;
  Ipv4InterfaceContainer wifiApInterface;
  wifiApInterface = address.Assign (apDevices);
  wifiStaInterfaces = address.Assign (staDevices);

  //install server to capture packets
  uint16_t port = 900;
  UdpServerHelper server(port);
  ApplicationContainer serverApp = server.Install(wifiApNode.Get(0));
  serverApp.Start(Seconds(0.9));
  serverApp.Stop(Seconds(simulationTime+2));
 
  //install clients
  InetSocketAddress destA (wifiApInterface.GetAddress(0), port);
  OnOffHelper clientA ("ns3::UdpSocketFactory", destA);
  clientA.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientA.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientA.SetAttribute ("DataRate", StringValue ("2000kb/s"));
  clientA.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  ApplicationContainer clientAppA = clientA.Install (wifiStaNodes.Get (0));
  for (uint32_t  i=1;i<nWifi;i++){
  	clientAppA.Add(clientA.Install(wifiStaNodes.Get(i)));
  }
  clientAppA.Start (Seconds (1));
  clientAppA.Stop (Seconds (simulationTime + 1));

  //install server2 and client2 in a separate port to populate arp before main app start. Packets not counted in results since different dest port. If we don't populate arp before simulation, the first transmission of each node will require additional ARP and ACK packet. This causes some nodes to never be able to transmit and interferes with the result.
  uint16_t port2 = 901;
  UdpServerHelper server2(port2);
  ApplicationContainer serverApp2 = server2.Install(wifiApNode.Get(0));
  serverApp2.Start(Seconds(0.1));
  serverApp2.Stop(Seconds(0.89));
  
  UdpClientHelper client2 (wifiApInterface.GetAddress(0), port2);
  client2.SetAttribute ("MaxPackets", UintegerValue (1));
  client2.SetAttribute ("Interval", TimeValue (MicroSeconds(1000)));
  client2.SetAttribute ("PacketSize", UintegerValue (100));
  for (uint32_t  i=0;i<nWifi;i++){
  	ApplicationContainer clientApps2 = client2.Install (wifiStaNodes.Get (i));
 	clientApps2.Start (Seconds (0.11+i*0.01));
  	clientApps2.Stop (Seconds (0.88));
  }

  //Flow monitor for debug
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  //Populate route in L3
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (tracing == true)
    {
      phy.EnablePcapAll("testwifi");
    }

  Simulator::Stop (Seconds (simulationTime+1));
  Simulator::Run ();

  //Calculate throughput after Simulation
  uint64_t totalPacketsThroughAP = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
  double throughput = totalPacketsThroughAP * (payloadSize) * 8 / (simulationTime * 1000000.0);
  std::cout << "Packet count = " << totalPacketsThroughAP << '\n';
  std::cout << "Throughput_UDP " << throughput << " Mbit/s" << '\n';
  double throughput_L2 = totalPacketsThroughAP * (payloadSize+28) * 8 / (simulationTime * 1000000.0); //Payload + IP header 20 bytes + UDP header 8 bytes
  std::cout << "Throughput_MAC " << throughput_L2 << " Mbit/s" << '\n';
  std::cout <<"*********************************************************"<<'\n';


  //print stats (debug)
  monitor -> CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
  FlowMonitor::FlowStatsContainer stats = monitor -> GetFlowStats();
  for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
  {
	//std::cout << "flow " << i->first << " packets=" <<i->second.rxPackets<<";"<<'\n';
	//std::cout << i->second.txBytes<<";"<<i->second.rxBytes <<";"<<'\n'; i->second.txPackets<<
  }

  Simulator::Destroy ();
}



int 
main (int argc, char *argv[])
{

  bool verbose = false;
  uint32_t nWifi = 1;
  bool tracing = false;
  double simulationTime = 10;
  uint32_t payloadSize = 995;  //995 UDP payload + 28 bytes IP&UDP header = 1023 MAC payload
  uint32_t minCw = 31;  //case1: [31,255], case2: [31, 1023], case3: [127, 1023]
  uint32_t maxCw = 255;
  //uint32_t slot = 50; //slot time in microseconds
  //uint32_t sifs = 28; //SIFS duration in microseconds


  //Pass argument from command line
  //./waf --run "scratch/testwifi --verbose=false --tracing=false --simulationTime=20" > result3 2>&1  

  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("payloadSize", "define payloadSize", payloadSize);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("minCw", "min dcf window", minCw);
  cmd.AddValue ("maxCw", "max dcf window", maxCw);
  cmd.AddValue ("simulationTime", "simulation time", simulationTime);
  cmd.Parse (argc,argv);
  
  

  //Collect results
  runSim(simulationTime, 1, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 5, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 10, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 15, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 20, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 30, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 50, payloadSize, minCw, maxCw, verbose, tracing);

  minCw = 31;
  maxCw = 1023;

  runSim(simulationTime, 1, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 5, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 10, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 15, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 20, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 30, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 50, payloadSize, minCw, maxCw, verbose, tracing);

  minCw = 127;
  maxCw = 1023;

  runSim(simulationTime, 1, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 5, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 10, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 15, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 20, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 30, payloadSize, minCw, maxCw, verbose, tracing);
  runSim(simulationTime, 50, payloadSize, minCw, maxCw, verbose, tracing);


  return 0;
}  






