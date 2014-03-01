#Create a simulator object
set ns [new Simulator]

#Open the nam trace file
set nf [open proj3A.nam w]
$ns namtrace-all $nf
#open the measurement output files
set f(1) [open proj3A_gpout1.tr w]


#necessary to remember the old bandwidth
set oldbw0 0
set oldbw00 0
#Define different colors for nam data flows
$ns color 1 Blue
$ns color 2 Red


#Define a 'finish' procedure
proc finish {} {
        global ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Close the measurement files
        global f
	  close $f(1)
	#Execute nam on the trace file
        #exec nam proj3A.nam &
        #exec "d:\gnuplot\binary\pgnuplot.exe" proj3A.dem
        exit 0
}


proc record1 {} {
        global f qmon fmon oldbw0 flowmon
	#Get an instance of the simulator
	  set ns [Simulator instance]
	#Set the time after which the procedure should be called again
        set time 0.2
	#Get the current time
        set now [$ns now]
      #How many bytes have been received by the traffic sinks?
        set bwf1 [$qmon set bdepartures_]
        set fcl [$flowmon classifier]
        set fids { 1}
        foreach i $fids {
              set flow($i) [$fcl lookup auto 0 0 $i]
              if { $flow($i) != "" } {
	               #Calculate the throughput and write it to the files
                     puts $f($i) "$now \t [expr [$flow($i) set bdepartures_]*8/$time/10000000] \t [expr [$flowmon set bdepartures_]*8/$time/10000000] \t [$qmon set bdepartures_]"
                     #remember the old value
                     $flow($i) set barrivals_ 0
                     $flow($i) set bdepartures_ 0
                     $flow($i) set bdrops_ 0
		  }
        }
     	#Reset the bytes_ values on the traffic sinks
        $qmon set bdepartures_ 0
     	  $flowmon set barrivals_ 0
        $flowmon set bdepartures_ 0
     	#Re-schedule the procedure
        $ns at [expr $now+$time] "record1"
}


#Create four nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
#Create links between the nodes
$ns duplex-link $n0 $n2 10Mb 40ms DropTail
$ns duplex-link $n1 $n2 10Mb 200ms DropTail
$ns duplex-link $n3 $n2 10Mb 10ms DropTail 
$ns queue-limit $n2 $n3 25


#Flow Monitor
set linkn2n3 [$ns link $n2 $n3]
set flowmon [$ns makeflowmon Fid]
$ns attach-fmon $linkn2n3 $flowmon
#Monitor the queue for the link between node 2 and node 3
$ns duplex-link-op $n2 $n3 queuePos 0.5
set fmon [$ns monitor-queue $n0 $n2 ""]
set qmon [$ns monitor-queue $n2 $n3 ""]


#Setting for nam
$ns duplex-link-op $n0 $n2 orient right-down
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n2 $n3 orient right


#TCP traffic source
#Create a TCP agent and attach it to node n0
set tcp0 [new Agent/TCP]
$ns attach-agent $n0 $tcp0
$tcp0 set fid_ 2
$tcp0 set class_ 2
#$tcp0 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp0 set window_ 1000
$tcp0 set packetSize_ 460
#$tcp0 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink [new Agent/TCPSink]
$ns attach-agent $n3 $sink
#Connect both agents
$ns connect $tcp0 $sink

# create an FTP source "application";
set ftp [new Application/FTP]
$ftp set maxpkts_ 1000
$ftp attach-agent $tcp0


#TCP traffic source
#Create a TCP agent and attach it to node n0
set tcp1 [new Agent/TCP]
$ns attach-agent $n1 $tcp1
$tcp1 set fid_ 1
$tcp1 set class_ 1
#$tcp1 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp1 set window_ 160 
$tcp1 set packetSize_ 460
#$tcp1 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink1 [new Agent/TCPSink]
$ns attach-agent $n3 $sink1
#Connect both agents
$ns connect $tcp1 $sink1

# create an FTP source "application";
set ftp1 [new Application/FTP]
$ftp1 set maxpkts_ 1000
$ftp1 attach-agent $tcp1


#Schedule events for all the flows
$ns at 0.0 "record1"
$ns at 0.0 "$ftp1 start"
$ns at 4.0 "$ftp start"
$ns at 10.0 "$ftp stop"
$ns at 10.0 "$ftp1 stop"
#Call the finish procedure after 6 seconds of simulation time
$ns at 10.1 "finish"

#Run the simulation
$ns run

