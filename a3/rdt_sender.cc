/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include <cassert>
#include <set>
#include <utility>
#include <algorithm>
#include <deque>
#include <iostream>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_com.h"


namespace sender{
	typedef std::set< std::pair<double,sqn_type> > ack_tmlims_type; //"absolute" ACK expiration time
	ack_tmlims_type ack_tmlims;
	typedef std::deque< std::pair<packet_u,double> > snd_buf_type;
	snd_buf_type snd_buf; //This holds not only packets in the window but also packets after the window but not yet able to be sent.
	sqn_type sqn_base = starting_sqn;
	sqn_type sqn_last = sqn_base;
	const double time_out = 0.3;
	const double time_margin = 0;
}

using namespace sender;
/* event handler, called when a message is passed from the upper layer at the 
   sender */

void ResetTimer(){
	if(ack_tmlims.empty()) Sender_StopTimer();
	else Sender_StartTimer(std::max(ack_tmlims.begin()->first-GetSimulationTime(),time_margin));
}

void TryToSend(){
	while(true){
		sqn_type n = sqn_last - sqn_base;
		assert(n<=win_sz);
		if(n>=win_sz) break;
		assert(n<=snd_buf.size());
		if(n>=snd_buf.size()) break;
		assert(snd_buf[n].first.structured.header.sqn == sqn_last);
		assert(snd_buf[n].second<0);
		Sender_ToLowerLayer(&snd_buf[n].first.raw);
		double next_t = GetSimulationTime()+time_out;
		ack_tmlims.insert(std::make_pair(next_t,sqn_last));
		snd_buf[n].second = next_t;
		++sqn_last;
	}
}

void Sender_FromUpperLayer(struct message *msg)
{
	size_t ml = msg->size+msg_size_size;
	sqn_type sqn = snd_buf.size() + sqn_base ;
	size_t chunk_n = ml/chunk_size + size_t(bool(ml%chunk_size));
	snd_buf.resize(chunk_n+snd_buf.size());
	{
		const char *p = msg->data;
		for(snd_buf_type::iterator i = snd_buf.end() - chunk_n; i!=snd_buf.end(); ++i){
			structured_packet &sp = i->first.structured;
			if(p == msg->data){
				put_size(sp.body,msg->size);
				const char *end = p+chunk_size-msg_size_size;
				if(msg->data+msg->size>=end){
					std::copy(p, end, sp.body+msg_size_size);
					p = end;
				}else{
					const char *early_end = msg->data+msg->size;
					std::copy(p, early_end, sp.body+msg_size_size);
					std::fill_n(sp.body+msg_size_size+(early_end-p),end-early_end,0);
				}
			}else if(msg->data+msg->size>=p+chunk_size){
				assert( (msg->data+msg->size==p+chunk_size && i+1==snd_buf.end()) || (i+1!=snd_buf.end()) );
				const char *end = p+chunk_size;
				std::copy(p, end, sp.body);
				p = end;
			}else{
				assert(i+1==snd_buf.end());
				const char *end = msg->data+msg->size;
				std::copy(p, end, sp.body);
				std::fill_n(sp.body+(end-p),p+chunk_size-end,0);
			}
			sp.header.sqn = sqn++;
			fill_checksum(sp);
			i->second = -1;
			assert( unsigned(i - snd_buf.begin()) == sp.header.sqn - sqn_base );
		}
	}
	TryToSend();
	ResetTimer();
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{	
	assert(pkt);
	const structured_packet &rd = *(const structured_packet*)pkt;
	if(corrupted(rd)) return;
	const unsigned off = rd.header.sqn-sqn_base;
	if(off>=win_sz) return;
	if(off>=snd_buf.size()) return;
	double t = snd_buf[off].second;
	sqn_type s = snd_buf[off].first.structured.header.sqn;
	assert(s == rd.header.sqn);
	if(t<0) return;
	ack_tmlims.erase(std::make_pair(t,s));
	snd_buf[off].second = -1;
	while((!snd_buf.empty())&&snd_buf.front().second<0){
		assert(snd_buf.front().first.structured.header.sqn == sqn_base);
		snd_buf.pop_front();
		++sqn_base;
	}
	TryToSend();
	ResetTimer();
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
	/*std::cout<<"\nSend "<<sqn_base<<'\t'<<sqn_last<<'\n';*/
	assert(!ack_tmlims.empty());
	ack_tmlims_type::iterator itr = ack_tmlims.begin();
	sqn_type s = itr->second;
	size_t n = s - sqn_base;
	assert(snd_buf[n].first.structured.header.sqn == s);
	assert(snd_buf[n].second == itr->first);
	Sender_ToLowerLayer(&snd_buf[n].first.raw);
	ack_tmlims.erase(itr);
	double next_t = GetSimulationTime()+time_out;
	ack_tmlims.insert(std::make_pair(next_t,s));
	snd_buf[n].second = next_t;
	ResetTimer();
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}
