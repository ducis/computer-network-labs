#Create a simulator object
set ns [new Simulator]

#Open the nam trace file
set nf [open proj3B.nam w]
$ns namtrace-all $nf

#open the measurement output files
set f(1) [open proj3B_gpout1.tr w]
set f(2) [open proj3B_gpout2.tr w]


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
	  close $f(2)
	#Execute nam on the trace file
        #exec nam proj3B.nam &
        #exec gnuplot proj3B.dem &
        exit 0
}


proc record1 {} {
        global f qmon oldbw0 flowmon
	#Get an instance of the simulator
	  set ns [Simulator instance]
	#Set the time after which the procedure should be called again
        set time 0.1
	#Get the current time
        set now [$ns now]
      #How many bytes have been received by the traffic sinks?
        set bwf1 [$qmon set bdepartures_]
        set fcl [$flowmon classifier]
        set fids { 1 2 }
        foreach i $fids {
              set flow($i) [$fcl lookup auto 0 0 $i]
              if { $flow($i) != "" } {
	               #Calculate the throughput and write it to the files
                     puts $f($i) "$now \t [expr [$flow($i) set bdepartures_]*8/$time/4000000] \t [expr [$flowmon set bdepartures_]*8/$time/4000000] \t [$qmon set bdepartures_]"
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
$ns duplex-link $n0 $n2 4Mb 10ms DropTail
$ns duplex-link $n1 $n2 16Mb 10ms DropTail
$ns duplex-link $n3 $n2 4Mb 10ms DropTail 
$ns queue-limit $n2 $n3 40


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


#4Mbps TCP traffic source
#Create a TCP agent and attach it to node n0
set tcp0 [new Agent/TCP]
$ns attach-agent $n0 $tcp0
$tcp0 set fid_ 2
$tcp0 set class_ 2
#$tcp0 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp0 set window_ 40
$tcp0 set packetSize_ 460
#$tcp0 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink [new Agent/TCPSink]
$ns attach-agent $n3 $sink
#Connect both agents
$ns connect $tcp0 $sink

# create an FTP source "application";
set ftp [new Application/FTP]
#$ftp set maxpkts_ 1000
$ftp attach-agent $tcp0


#1Mbps TCP traffic source (one)
#Create a TCP agent and attach it to node n1
set tcp1 [new Agent/TCP]
$ns attach-agent $n1 $tcp1
$tcp1 set fid_ 1
$tcp1 set class_ 1
#$tcp1 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp1 set window_ 40
$tcp1 set packetSize_ 460
#$tcp1 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink1 [new Agent/TCPSink]
$ns attach-agent $n3 $sink1
#Connect both agents
$ns connect $tcp1 $sink1

# create an FTP source "application";
set ftp1 [new Application/FTP]
#$ftp1 set maxpkts_ 250
$ftp1 attach-agent $tcp1


#1Mbps TCP traffic source (two)
#Create a TCP agent and attach it to node n1
set tcp2 [new Agent/TCP]
$ns attach-agent $n1 $tcp2
$tcp2 set fid_ 1
$tcp2 set class_ 1
#$tcp2 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp2 set window_ 40
$tcp2 set packetSize_ 460
#$tcp2 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink2 [new Agent/TCPSink]
$ns attach-agent $n3 $sink2
#Connect both agents
$ns connect $tcp2 $sink2

# create an FTP source "application";
set ftp2 [new Application/FTP]
#$ftp2 set maxpkts_ 250
$ftp2 attach-agent $tcp2


#1Mbps TCP traffic source (three)
#Create a TCP agent and attach it to node n1
set tcp3 [new Agent/TCP]
$ns attach-agent $n1 $tcp3
$tcp3 set fid_ 1
$tcp3 set class_ 1
#$tcp3 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp3 set window_ 40
$tcp3 set packetSize_ 460
#$tcp3 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink3 [new Agent/TCPSink]
$ns attach-agent $n3 $sink3
#Connect both agents
$ns connect $tcp3 $sink3

# create an FTP source "application";
set ftp3 [new Application/FTP]
#$ftp3 set maxpkts_ 250
$ftp3 attach-agent $tcp3


#1Mbps TCP traffic source (four)
#Create a TCP agent and attach it to node n1
set tcp4 [new Agent/TCP]
$ns attach-agent $n1 $tcp4
$tcp4 set fid_ 1
$tcp4 set class_ 1
#$tcp4 set rate_ 4Mb
# window_ * (packetsize_ + 40) / RTT
$tcp4 set window_ 40
$tcp4 set packetSize_ 460
#$tcp4 set overhead_ 0.001
#Create a TCP sink agent and attach it to node n3
set sink4 [new Agent/TCPSink]
$ns attach-agent $n3 $sink4
#Connect both agents
$ns connect $tcp4 $sink4

# create an FTP source "application";
set ftp4 [new Application/FTP]
#$ftp4 set maxpkts_ 250
$ftp4 attach-agent $tcp4


#Schedule events for all the flows
$ns at 0.0 "record1"
$ns at 0.0 "$ftp produce 2000"
$ns at 0.0 "$ftp1 produce 1000"
$ns at 0.0 "$ftp2 produce 1000"
$ns at 0.0 "$ftp3 produce 1000"
$ns at 0.0 "$ftp4 produce 1000"
$ns at 10.0 "$ftp1 stop"
$ns at 10.0 "$ftp2 stop"
$ns at 10.0 "$ftp3 stop"
$ns at 10.0 "$ftp4 stop"
$ns at 10.0 "$ftp stop"
#Call the finish procedure after 6 seconds of simulation time
$ns at 10.1 "finish"

#Run the simulation
$ns run

