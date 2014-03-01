#pragma once

#include "rdt_struct.h"
#include <cstddef>
#include <algorithm>

typedef unsigned int uint32; //32-bit unsigned integer. Please change this definition on platforms with non-32-bit ints

typedef uint32 checksum_type;
typedef uint32 sqn_type;//sequence number

struct structured_packet{
	struct header_type{
		checksum_type checksum;
		sqn_type sqn; //also used for ACK.
	};
	header_type header;
	const static size_t body_size = RDT_PKTSIZE - sizeof(header_type);
	char body[body_size];
};

const size_t chunk_size = structured_packet::body_size;

union packet_u{
	// Here I ignore the problem of different binary representations of data on different machines,
	// specifically endianness, since there is only one executable running on one machine.
	structured_packet structured;
	packet raw;
};

inline bool corrupted(const structured_packet &p){
	assert(chunk_size%sizeof(checksum_type)==0);
	checksum_type s = p.header.checksum+p.header.sqn;
	const checksum_type *end = (checksum_type*)(p.body+chunk_size);
	for(const checksum_type *i = (const checksum_type*)p.body; i!=end; ++i){
		s+=*i;
	}
	return bool(s);
}

inline void fill_checksum(structured_packet &p){
	assert(chunk_size%sizeof(checksum_type)==0);
	checksum_type s = 0-p.header.sqn;
	const checksum_type *end = (checksum_type*)(p.body+chunk_size);
	for(const checksum_type *i = (const checksum_type*)p.body; i!=end; ++i){
		s-=*i;
	}
	p.header.checksum = s;
}

const size_t msg_size_size = sizeof(int);

template<typename I> int extract_size(I i){
	int sz;
	std::copy(i,i+msg_size_size,(char*)&sz);
	return sz;
}
template<typename I> void put_size(I i,int sz){
	const char *p = (const char*)&sz;
	std::copy(p,p+msg_size_size,i);
}

const sqn_type starting_sqn = 0;

const size_t win_sz = 1024; //window size
