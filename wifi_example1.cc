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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

// WiFi example that builds the following Network Topology:
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

//Creates a log component in the script: it means that logging messages can be enabled in this script by setting NS_LOG environment
//variable in many parts of the program
NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  //Number of devices to be created
  bool verbose = true;
  uint32_t nCsma = 3;
  uint32_t nWifi = 3;

  //Add command line parameters that can be changed in the command line execution in the linux terminal
  //if entered ./waf --run="wifi_example.cc --help" the descriptions will show up.
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  //Creates two nodes that will connect the wifi and LAN via point-to-point link
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  //PointToPointHelper does the low-evel work requires to put the link together
  PointToPointHelper pointToPoint; //instantiates an object
  //tells the object to use 5Mbps as the Datarate when create a PointToPointNetDevice
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); 
  //tells the object to use 2ms as the value of the transmission delay of every channel that is created
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
 
  //Install configurations of PointToPoint into p2pNodes 
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  //Declare the node that will be on the CSMA network
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1)); //Gets the first node of point to point communication and install csma configuration in it
  csmaNodes.Create (nCsma); //Create extra CSMA nodes that will compose the network

  //Instantiates a CsmaHelper and sets Attributes
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  //Create a NetDeviceContainer to keep track of the CSMA devices and install the devices on the nodes
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //Create nodes for the Wifi network
  NodeContainer wifiStaNodes; //Station nodes - PC, laptop...
  wifiStaNodes.Create (nWifi);
  //Set one of the nodes to be part of the point to point link
  NodeContainer wifiApNode = p2pNodes.Get (0); //Access Point node - Example: Wifi Router

  //Creates a Wifi Channel with a default propagation loss and propagation delay models
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  //Usage of default PHY layer configuration
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  
  //Creates a channel object and associate it to the PHY layer object manager to 
  //make sure that the PHY layer objects share the same wireless medium and can communicate and interfere
  phy.SetChannel (channel.Create ());

  //MAC layer configuration - choose non-QoS
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager"); //tells the helper which type of rate control algorithm that will be used - AARF

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  //Configure the type of MAC, the SSID of the infrastructure network
  Ssid ssid = Ssid ("ns-3-ssid"); //Creates an 802.11 Service Set Identifier (SSID)
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false)); //Will use non-Qos station - nqsta - Do not perform active probing 

  //After setting up all PHY and MAC layers parameters, the Install method creates wifi devices of the configured stations (STA)
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  //AP configuration
  //Set MAC layers with Non-Qos Access Point and BeaconGeneration (?) with interval between beacons of 2.5 seconds
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  //Creates AP node with the defined configurations
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  //Sets up the mobility model
  MobilityHelper mobility; //Instantiates a MobilityHelper object

  //Set some attributes controlling the position allocator functionality
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  //Tell the nodes to move in a random direction at a random speed inside the boundaries defined
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  //Tell the MobilityHelper to install the mobility models on the STA nodes
  mobility.Install (wifiStaNodes);

  //Set AP nodes to have a constant position
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install(csmaNodes);
  mobility.Install(p2pNodes);

  //Nodes, devices and channels were created. Mobility models were set up
  //Now, begin the protocol stack configuration
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  //Assign IP addresses t0 the device interfaces
  Ipv4AddressHelper address;
  //Assign 10.1.1.0 to create two addresses needed for the two point-to-point devices
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  //Assign 10.1.2.0 to the CSNMA devices
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  //Assign 10.1.3.0 to Wifi devices
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  //Create echoserver and client to exchange UDP packets
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  //Enable internetwork routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));

  //Create tracing to cover all networks
  pointToPoint.EnablePcapAll ("third");
  phy.EnablePcap ("third", apDevices.Get (0));
  csma.EnablePcap ("third", csmaDevices.Get (0), true);

  AnimationInterface anim ("wireless-animation.xml"); // Mandatory
 /* for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiStaNodes.Get (i), "STA"); // Optional
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 255, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < wifiApNode.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiApNode.Get (i), "AP"); // Optional
      anim.UpdateNodeColor (wifiApNode.Get (i), 0, 255, 0); // Optional
    }
  for (uint32_t i = 0; i < csmaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (csmaNodes.Get (i), "CSMA"); // Optional
      anim.UpdateNodeColor (csmaNodes.Get (i), 0, 0, 255); // Optional 
    }*/

  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
