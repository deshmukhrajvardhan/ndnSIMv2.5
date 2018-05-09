#Trace Files

This folder contains the NS-2 mobility trace files, generated using
the bonnmotion tool.

The files were generated using the following commands:

### Vehicular Mobility  (~45 km/h)

./bm -f manhattan-100-vehicle-X ManhattanGrid -n 100 -x 1000 -y 1000 -u 10 -v 10 -m 13 -s 5 -d 4000 -i 3600 -b 0

### Pedestrian Mobility (1 km/h)

./bm -f manhattan-100-pedestrian-X ManhattanGrid -n 100 -x 1000 -y 1000 -u 10 -v 10 -m 1 -s 1 -d 4000 -i 3600 -b 0

### Manhattan Grid Options

./bm -hm ManhattanGridBonnMotion 3.0.1

OS: Linux 4.4.0-96-generic
Java: Oracle Corporation 1.8.0_131


== ManhattanGrid =====================================================
Version:      v1.0
Description:  Application to construct ManhattanGrid mobility scenarios
Affiliation:  University of Bonn - Institute of Computer Science 4 (http://net.cs.uni-bonn.de/)
Authors:      University of Bonn
Contacts:     bonnmotion@lists.iai.uni-bonn.de

App:
	-D print stack trace
Scenario:
	-a <attractor parameters (if applicable for model)>
	-c [use circular shape (if applicable for model)]
	-d <scenario duration>
	-i <number of seconds to skip>
	-n <number of nodes>
	-x <width of simulation area>
	-y <height of simulation area>
	-z <depth of simulation area>
	-R <random seed>
	-J <2D, 3D> Dimension of movement output
ManhattanGrid:
	-c <speed change probability>
	-e <min. speed>
	-m <mean speed>
	-o <max. pause>
	-p <pause probability>
	-q <update distance>
	-s <speed standard deviation>
	-t <turn probability>
	-u <no. of blocks along x-axis>
	-v <no. of blocks along y-axis>
