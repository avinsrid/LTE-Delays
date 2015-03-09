from __future__ import division
import sys
import os
import pylab

######################################################
#  Python file to read xml data and plot it in a graph
#  The figures are saved in a pdf file
#  To run it: $ ./waf --pyrun "xmlread.py NAME_OF_XML_FILE"
######################################################


try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

#Parse elements from xml
et = ElementTree.parse(sys.argv[1]);
bitrates = []
losses = []
delays = []

for flow in et.findall("FlowStats/Flow"):
   #Get flowIds information
   for tpl in et.findall("Ipv4FlowClassifier/Flow" ):
       if tpl.get('flowId') == flow.get('flowId'):
           break
   if tpl.get("destinationPort") == '698':
       continue

   losses.append(int(flow.get('lostPackets')))
   
   rxPackets = int(flow.get('rxPackets'))
   if rxPackets == 0:
       bitrates.append(0)
   else:
       t0 = long(float(flow.get('timeFirstRxPacket')[:-2]))
       t1 = long(float(flow.get('timeLastRxPacket')[:-2]))
       duration = (t1 - t0)*1e-9
       bitrates.append(8*long(float(flow.get('rxBytes'))/duration*1e-3))
       delays.append(float(flow.get('delaySum')[:-2])*1e-9/rxPackets)

#Plot graphics
pylab.subplot(311)
pylab.hist(bitrates, bins=40)
pylab.xlabel("Flow bitrate (bit/s)")
pylab.ylabel("Number of flows")

pylab.subplot(312)
pylab.hist(losses, bins=40)
pylab.xlabel("Number of lost packets")
pylab.ylabel("Number of flows")

pylab.subplot(313)
pylab.hist(delays, bins = 10)
pylab.xlabel("Delay(s)")
pylab.ylabel("Number of flows")

#Save data in a pdf file 
pylab.subplots_adjust(hspace=0.4)
pylab.savefig("results.pdf")




       


      




