/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_com.h"

namespace receiver{
	std::deque<bool> win_acked;
	std::deque<char> rcv_buf; //This holds not only packets in the window but also packets before the window but not yet able to be assembled and passed to the higher layer.
	sqn_type sqn_base = starting_sqn;
}

using namespace receiver;

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */

void Receiver_FromLowerLayer(struct packet *pkt)
{
	/*std::cout<<"\nRecv "<<sqn_base<<'\n';*/
	assert(pkt);
	const structured_packet &rd = *(const structured_packet*)pkt;
	if(corrupted(rd)) return;
	const unsigned off = rd.header.sqn-sqn_base; //use subtraction to handle wraparound
	if(off<win_sz||sqn_base+win_acked.size()-rd.header.sqn<win_sz){
		packet_u ack;
		ack.structured.header.sqn = rd.header.sqn;
		fill_checksum(ack.structured);
		Receiver_ToLowerLayer(&ack.raw);
	}
	if(off>=win_sz) return;
	if(win_acked.size()<=off){
		rcv_buf.resize( (off + 1 - win_acked.size()) * chunk_size + rcv_buf.size() );
		win_acked.resize(off + 1,false);
	}
	win_acked[off] = true;
	size_t roff = win_acked.size() - off;
	std::copy(rd.body, rd.body+chunk_size, rcv_buf.end() - chunk_size * roff);
	while( (!win_acked.empty()) && win_acked.front() ){
		win_acked.pop_front();
		++sqn_base;
	}
	while(true){
		size_t done = rcv_buf.size() - win_acked.size() * chunk_size;
		if(done < msg_size_size) break;
		message msg = {extract_size(rcv_buf.begin())};
		if(done - msg_size_size < unsigned(msg.size)) break;
		if(!msg.size) break;
		assert(msg.size>0);
		msg.data = new char[msg.size];
		size_t n = msg_size_size+msg.size;
		std::copy(rcv_buf.begin()+msg_size_size, rcv_buf.begin()+n, msg.data);
		rcv_buf.erase(rcv_buf.begin(), rcv_buf.begin()+n+(chunk_size-n%chunk_size)%chunk_size);
		Receiver_ToUpperLayer(&msg);
		delete[] msg.data;
	}
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
	assert(sizeof(packet_u)==sizeof(packet));
	assert(sizeof(structured_packet)==sizeof(packet));
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}
