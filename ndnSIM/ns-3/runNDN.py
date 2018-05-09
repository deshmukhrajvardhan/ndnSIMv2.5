#!/usr/bin/python

'''
  This script executes ten different runs of the simulation scenario. 
  It creates folders called `manhattan-50-pedestrian-<Seed Number>` 
  and moves the simulation files to these folders.

  Usage:
  This file should reside in the ~/ndnSIM/ns3 directory. To run, just
  execute nohup ./runNDN.py &

  Author:
  Thiago Teixeira
  University of Massachusetts Amherst

'''

import getopt
import glob
import subprocess
import sys
import time
import os

from datetime import datetime, date
from pathlib import Path

NODE_COUNT = '50' # how many nodes are pedestrian and how many are vehicles
runScenario = '" ./waf --run "scratch/ndn-offloading-twoInterfaces'
storeDirectory = '$HOME/manhattan-' + str(int(NODE_COUNT) * 2) +'Nodes-'

'''
Execute simulations
'''
def runSimulations ():
    for seed in range(5,9):
	now1 = datetime.now().time() # Get current time
	print "Experiment started at: " + str(now1)
	
	wafString = 'NS_GLOBAL_VALUE="RngRun=' + str(seed) + runScenario + \
		' --traceVeh=./traces/' + NODE_COUNT + 'Nodes/manhattan-' + NODE_COUNT + '-vehicle-' + \
		str(seed) + '.ns_movements' + \
		' --tracePed=./traces/' + NODE_COUNT + 'Nodes/manhattan-' + NODE_COUNT + '-pedestrian-' \
		+ str(seed) + '.ns_movements' + \
		' --dataDepotStop=1.0 --pedNodes=' + NODE_COUNT + ' --vehNodes=' + NODE_COUNT + '"'
		
	print wafString
	sys.stdout.flush()
	subprocess.call(wafString, shell=True)		
	moveFiles(seed)
	
	now2 = datetime.now().time()
	elapsedTime = datetime.combine(date.today(), now2) - datetime.combine(date.today(), now1)
	print "Experiment ended with elapsed time of: " + str(elapsedTime)
	print "**********************************"
    
    return

'''
Move simulation files to folders
'''
def moveFiles (seed):	
    # Create trace diretories
    directory = storeDirectory + str(seed) #+ '-cars-only'
    # TODO: If folder exists, just copy the files
    subprocess.call("mkdir " + directory, shell=True)
    print "Created directory: " + str(directory)
    
    # If trace file exist, move file
    
    # App-delays-trace
    if os.path.isfile("./scratch/app-delays-trace.txt"):
	print "    app-delays-trace file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/app-delays-trace.txt " + directory, shell=True)
    else:
	print "    app-delays-trace file does not exist!"

    # Cs-trace-ped
    if os.path.isfile("./scratch/cs-trace-ped.txt"):
	print "    cs-trace-ped file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/cs-trace-ped.txt " + directory, shell=True)
    else:
	print "    cs-trace-ped file does not exist!"
	
    # Cs-trace-veh
    if os.path.isfile("./scratch/cs-trace-veh.txt"):
	print "    cs-trace-veh file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/cs-trace-veh.txt " + directory, shell=True)
    else:
	print "    cs-trace-ped file does not exist!"
		        
    # Cs-trace-dataRepo
    if os.path.isfile("./scratch/cs-trace-dataRepo.txt"):
	print "    cs-trace-dataRepo file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/cs-trace-dataRepo.txt " + directory, shell=True)
    else:
	print "    cs-trace-dataRepo file does not exist!"

    # Energy-trace file
    if os.path.isfile("./scratch/energy_level.txt"):
        print "    energy_level file exists. Moving file to trace folder!"
        subprocess.call("mv ./scratch/energy_level.txt " + directory, shell=True)
    else:
        print "    energy_level file does not exist!"


    '''
    if os.path.isfile("./scratch/consumerL3Trace.txt"):
	print "    consumerL3trace file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/consumerL3Trace.txt " + directory, shell=True)
    else:
	print "    consumerL3Trace file does not exist!"
			
    if os.path.isfile("./scratch/producerL3Trace.txt"):
	print "    producerL3trace file exists. Moving file to trace folder!"
	subprocess.call("mv ./scratch/producerL3Trace.txt " + directory, shell=True)
    else:
	print "    producerL3Trace file does not exist!"
    '''
    return
	

runSimulations()
