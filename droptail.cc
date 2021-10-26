#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

#define MIN    (1.00f)
#define MAX    (2.0f)

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Dumbbell");

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;


double randf( double min, double max )
{
    return min + (rand() / ( RAND_MAX / ( max - min) ) ) ;  
}

int
main (int argc, char *argv[])
{

  uint32_t redTest = 1;
  global_start_time = 0.0;
  global_stop_time = 320.0; 
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 3.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 2.0;

  uint32_t    nLeaf = 30; 
  uint32_t    modeBytes  = 0;
  uint32_t    queueDiscLimitPackets = 100;
  std::string queueDiscType = "FIFO";
  std::string bottleNeckLinkBw = "1Mbps";
  std::string bottleNeckLinkDelay = "100ms";

  CommandLine cmd;
  cmd.AddValue ("testNumber", "Run test 1", redTest);
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("queueDiscType", "Set QueueDisc type to RED", queueDiscType);
  cmd.AddValue ("modeBytes", "Set QueueDisc mode to Packets <0> or bytes <1>", modeBytes);

  cmd.Parse (argc,argv);

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::PfifoFastQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("10ms"));

  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  // Install Stack
  InternetStackHelper stack;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      stack.Install (d.GetLeft (i));
    }
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      stack.Install (d.GetRight (i));
    }

  stack.Install (d.GetLeft ());
  stack.Install (d.GetRight ());
  TrafficControlHelper tchBottleneck;
  tchBottleneck.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  QueueDiscContainer queueDiscs =  tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
  tchBottleneck.Install (d.GetRight ()->GetDevice (0));

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  uint16_t port = 5001;
  PPBPHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;

  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetRight (i)));
    }
  sinkApps.Start (Seconds (sink_start_time));
  sinkApps.Stop (Seconds (sink_stop_time));

  srand(time(NULL));
  ApplicationContainer clientApps;

  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      // Create an on/off app sending packets to the Right side
      AddressValue remoteAddress (InetSocketAddress (d.GetRightIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetLeft (i)));
    }
  double client = randf(MIN, MAX);
  clientApps.Start (Seconds (client));
  clientApps.Stop (Seconds (client_stop_time));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  bottleNeckLink.EnableAscii (ascii.CreateFileStream ("ads1.tr"), 0, 0);

  std::cout << "Running the simulation" << std::endl;
  Simulator::Stop (Seconds (sink_stop_time + 5));
  Simulator::Run ();

  uint32_t totalRxBytesCounter = 0;
  for (uint32_t i = 0; i < sinkApps.GetN (); i++)
    {
      Ptr <Application> app = sinkApps.Get (i);
      Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
      totalRxBytesCounter += pktSink->GetTotalRx ();
     }
  std::cout << "\nGoodput MBit/sec:" << std::endl;
  std::cout << (totalRxBytesCounter * 8/ 1e6)/Simulator::Now ().GetSeconds () << std::endl;

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();
  std::cout << "*** stats from Node left queue disc ***" << std::endl;
  std::cout << st << std::endl;

  std::cout << "Destroying the simulation" << std::endl;
  Simulator::Destroy ();
  return 0;

}
