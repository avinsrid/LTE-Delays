/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/flow-monitor-module.h"
#include <ns3/flow-monitor-helper.h>
#include "ns3/config-store.h"
#include "ns3/stats-module.h"
#include "ns3/netanim-module.h"

//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 Network Topology:
 
    UE   ------ eNB    enB --------- UE  
 7.0.0.2          -    -          7.0.0.3
                   -  -     
                    PGW 7.0.0.1                          
                     - 
                     -
                  RemoteHost 1.0.0.2

  This simulation inserts LTE devices in the nodes and packets are randomly sent/received between them.
  Delays are input in the nodes as a random variable and the end-to-end delay of each packet is measured 
  and displayed for the user.
  In each flow, 200 packets are sent.

 */
NS_LOG_COMPONENT_DEFINE ("Use-case-1-Final");


// This function generates a random variable that has a Normal Distribution
Ptr<NormalRandomVariable> GenerateNormalRandomVariable(double mean, double variance){
    Ptr<NormalRandomVariable> x = CreateObject<NormalRandomVariable> ();
    x->SetAttribute ("Mean", DoubleValue (mean));
    x->SetAttribute ("Variance", DoubleValue (variance));
    return x;
}

// This function inserts delay in the nodes
bool netDevCb(
  Ptr<NetDevice> device,
  Ptr<const Packet> pkt,
  uint16_t  protocol,
  const Address &from)
{ 
    static Ptr<NormalRandomVariable> x = GenerateNormalRandomVariable(5, 3); //Create a random variable
    Ptr<Node> node = device->GetNode (); //Define which node is chosen
    Simulator::Schedule(MilliSeconds(x->GetValue()), &Node::NonPromiscReceiveFromDevice, node, device, pkt, protocol, from); //Insert delay
    //std::cout << "Input Delay: " << x->GetValue() << " ms" << std::endl;
    return  true;
}
 
// This function calculates the delay of each packet sent over the network

void
Rx (Ptr<const Packet> packet, const Address& add)
{
     SeqTsHeader seqTs;
     packet->PeekHeader(seqTs);
     int64_t receive_time, send_time, delay;

     receive_time = Simulator::Now().GetMilliSeconds(); //Get time that packet is received
     send_time = seqTs.GetTs().GetMilliSeconds(); //Record time that packet was sent
 
     delay = (receive_time - send_time); //delay calculation

     std::cout << "Output Delay: " << delay << " ms" << std::endl;
  
}

int main (int argc, char *argv[])
{
  //Set value
  uint16_t numberOfNodes = 2;
  double simTime = 10;
  double distance = 10000;
  double interPacketInterval = 20;
  char filename[50];

  CommandLine cmd;
  

  //The result show the UdpClient and PacketSink information
  Packet::EnablePrinting ();
  Time::SetResolution (Time::NS);
  RngSeedManager::SetSeed (3);
  //RngSeedManager::SetRun (2); // Uncomment this line and change the Run Number to change the randomness of the random variable
  //Uncomment these lines to get more information of the packets sent and received during the simulation
  //LogComponentEnable("UdpClient",LOG_LEVEL_ALL);
  //LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable("UdpServer",LOG_LEVEL_ALL);
    
  cmd.Parse(argc, argv);

  //Activate EPC (Evolved Packet Core) model: it allows Ipv4 networking usage with LTE devices

  //Create LTE Helper object: provide methods to add UEs and eNBs and configure them
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  //Implements EPC based on point-to-point links
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  //Tell LTE Helper that EPC will be used
  lteHelper->SetEpcHelper (epcHelper);
  
  //Allow input of LTE configuration parameters
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  //Parse again so that overriden default values can be override from the command line
  cmd.Parse(argc, argv);

  //Create Pgw pointer Packet Data Network Gateway(Pgw)
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0); 
  
  // Create the internet stack
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet - point to point connection
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.01)));

  //Add some Value
  std::string probeName;
  std::string probeTrace;

  //Set connection between pgw and remoteHost
  //Give IP Address
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  probeName = "ns3::Ipv4PacketProbe";
  probeTrace = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";

  // Specify routes so that the remote host can reach LTE UEs
  // pointerToPointEpcHelper will by default assign to LTE UEs an IP address
  // in the 7.0.0.0 network
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  //Create UE and eNB Nodes
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model on network components
  Ptr<ListPositionAllocator> positionAlloc_ue = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAlloc_enb = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAlloc_RH = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAlloc_PGW = CreateObject<ListPositionAllocator> ();
  
  positionAlloc_ue->Add (Vector(0 , 0, 0));
  positionAlloc_ue->Add (Vector(4*distance, 0, 0));
  positionAlloc_enb->Add (Vector(distance, 0, 0));
  positionAlloc_enb->Add (Vector(3*distance, 0, 0));
  positionAlloc_RH->Add (Vector(2*distance, distance, 0));
  positionAlloc_PGW->Add (Vector(2*distance, distance/2, 0));
 
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc_ue);
  mobility.Install(ueNodes);
  mobility.SetPositionAllocator(positionAlloc_enb);
  mobility.Install(enbNodes);
  mobility.SetPositionAllocator(positionAlloc_RH);
  mobility.Install(remoteHost);
  mobility.SetPositionAllocator(positionAlloc_PGW);
  mobility.Install(pgw);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
     Ptr<Node> ueNode = ueNodes.Get (u);
     // Set the default gateway for the UE
     Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
     ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
  {
    lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
    //   side effect: the default EPS bearer will be activated
  }

  // Gets the nodes information to apply the delays 
  Ptr<NetDevice> Device1 = enbLteDevs.Get(0);
  Ptr<NetDevice> Device2 = enbLteDevs.Get(1);
  Ptr<NetDevice> Device3 = internetDevices.Get(0);

  // Callback function to apply the delays
  Callback<bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&> cb;
  cb = MakeCallback(&netDevCb);
  Device1->SetReceiveCallback(cb);
  Device2->SetReceiveCallback(cb);
  Device3->SetReceiveCallback(cb);
  

  // Install and start applications on UEs and remote host

  // Define ports for packet sink creation
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  //Define the exchange packets process

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute("PacketSize", UintegerValue(100));       
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(400));
      
      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute("PacketSize",UintegerValue(100));
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(400));

      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute("PacketSize",UintegerValue(100));
      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue(400));

      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      clientApps.Add (dlClient.Install (remoteHost));
     
 if (u+1 < ueNodes.GetN ())
        {
          clientApps.Add (client.Install (ueNodes.Get(u+1)));
        }
      else
        {
          clientApps.Add (client.Install (ueNodes.Get(0)));
        }
    }

  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  lteHelper->EnableTraces ();

  // enable PCAP tracing
  p2ph.EnablePcapAll("lena-epc-first");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  monitor = flowmon.Install(ueNodes);
  monitor = flowmon.Install(remoteHost);
  monitor = flowmon.GetMonitor ();
  monitor->SetAttribute("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute("PacketSizeBinWidth", DoubleValue (2000));

  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&Rx));

  AnimationInterface anim ("test-animation.xml");
  
  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (ueNodes.Get (i), "UE"); // Optional
      anim.UpdateNodeColor (ueNodes.Get (i), 255, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (enbNodes.Get (i), "ENB"); // Optional
      anim.UpdateNodeColor (enbNodes.Get (i), 0, 255, 0); // Optional   
    }
  for (uint32_t i = 0; i < remoteHostContainer.GetN (); ++i)
    {
      anim.UpdateNodeDescription (remoteHostContainer.Get (i), "Remote Host"); // Optional
      anim.UpdateNodeColor (remoteHostContainer.Get (i), 0, 0, 255); // Optional 
    }

      anim.UpdateNodeDescription (pgw, "PGW"); // Optional
      anim.UpdateNodeColor (pgw, 255, 0, 255); // Optional 

    for (uint32_t i = 0; i < 6; ++i)
    {
      anim.UpdateNodeSize (i, 5000, 5000);
    } 

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  monitor->CheckForLostPackets ();
  sprintf(filename, "flow-monitor-file.xml");
  monitor->SerializeToXmlFile (filename, true, true );
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // first 2 FlowIds are for ECHO apps, we don't want to display them
      if (i->first > 0)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / simTime / 1024 / 1024  << " Mbps\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / simTime / 1024 / 1024  << " Mbps\n";
          std::cout << " Mean Delay: " << i->second.delaySum/i->second.rxPackets << "\n";
          
          }
    } 
  
  Simulator::Destroy();
  return 0;

}
