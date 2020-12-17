University of Southern California 
EE597 Project – Saturation Throughput
Wireless Networks 
by
Wanwiset Peerapatanapokin
Jiang Zhang
************************************************************************************

Part 1: Analysis of saturation throughput is implemented in project.m. It also visually presents the final combined result.

project.m - MATLAB code, simply run the script in MATLAB to generate analysis results and plot simulation points.



Part 2: Simulation of saturation throughput is done using NS-3 version 3.29. The main function is implemented in testwifi.cc. Some behavior and parameters of NS-3 are also modified in order to match the 802.11 DCF model. This is done by modifying the source code wifi-mac.cc and wifi-phy.cc. The file result.txt carries the simulation output used in the report of this project.

To run the simulation, place testwifi.cc in the scratch folder of the NS-3 directory. For example: Desktop/ns-allinone-3.29/ns-3.29/scratch/testwifi.cc

Then place the modified source code wifi-mac.cc and wifi-phy.cc in ../ns-3.29/src/wifi/model folder

Finally, using the linux terminal, go to NS-3 directory and run the command as the following: ../ns-3.29$ ./waf --run "scratch/testwifi --verbose=false --tracing=false --payloadSize=995 --simulationTime=100"

To output result in text file use the command '> "filename" 2>&1' at the end, for example: ../ns-3.29$ ./waf --run "scratch/testwifi --verbose=false --tracing=false --payloadSize=995 --simulationTime=100" > result 2>&1

We can also pass parameters directly from the command. In this setup, 'verbose', 'tracing', 'payloadSize', and 'simulationTime'  are valid arguments. If verbose=true the logging component will be enabled and we can monitor the behavior of NS-3 closely. If tracing = true, packet tracing will be recorded and can be viewed with WireShark. payloadSize and simulationTime are simulation parameters, as discussed in the report.

