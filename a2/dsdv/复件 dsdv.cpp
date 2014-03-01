#include <cstdio>
#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <time.h>
#include <string>
#include <map>
#include <algorithm>
#include <limits>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define snprintf _snprintf
#define snscanf _snscanf
//#define
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#define ENSURE(s) if(!(s)){ std::cout<<"FATAL "<<__LINE__<<'\n'; exit(0); }

const unsigned c_interval = 1;
const unsigned c_buf_size = 1024;
const unsigned c_dgram_size_ctrl = 512;
const unsigned c_max_line_size = 50;

char g_buf[c_buf_size];

int main(int argc, char **argv){
#ifdef WIN32 
	{ 
		WSADATA wsaData;
		ENSURE(!WSAStartup(MAKEWORD(2,2), &wsaData));
	}
#endif
	using namespace std;
	ENSURE(3==argc);
	int s = socket(AF_INET,SOCK_DGRAM,0);	
	ENSURE(s>=0);

	typedef float cost_type;
	typedef unsigned long sn_type;//seq #
	typedef pair<sn_type,cost_type> lk_type;
	typedef pair<string,lk_type> rt_type;//routing information
	typedef map<string,rt_type> rt_tbl_type;
	rt_tbl_type rts;
	typedef pair< sockaddr, cost_type > dc_type; // direct connection
	typedef map< string, dc_type > dc_tbl_type;
	dc_tbl_type dcs;

	unsigned max_datagram_size;
	{
		int ol = sizeof(max_datagram_size);
		ENSURE(!getsockopt(s,SOL_SOCKET,SO_MAX_MSG_SIZE,(char*)&max_datagram_size,&ol));
		assert(sizeof(max_datagram_size)==ol);
		//max_datagram_size = ntohl(max_datagram_size);
	}
	max_datagram_size = min(max_datagram_size,min(c_buf_size,c_dgram_size_ctrl));

	ENSURE(3*c_max_line_size<max_datagram_size);

	{
		sockaddr_in sa;
		sa.sin_family      = AF_INET;
		sa.sin_port        = htons((u_short)atoi(argv[1]));
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		ENSURE(!bind(s,(sockaddr*)&sa,sizeof(sa)));
	}

	clock_t next = clock();
	assert(next>0);
	for(sn_type prtN=1;true;++prtN){
		string name;
		{
			int n = 0;
			ifstream st(argv[2]);
			st>>n>>name;
			ENSURE(name.size()+1<=c_max_line_size);
			for(int i=0;i<n;++i){
				string n2;
				st>>n2;
				dc_type &dc = dcs[n2];//whether an insertion is needed is automatically handled by operator[]
				string host,port;
				st>>host>>dc.second>>port;
				if(dc.second<0) dc.second = numeric_limits<cost_type>::infinity();
				struct addrinfo *pai;
				ENSURE(!getaddrinfo(host.c_str(), port.c_str(), NULL, &pai));
				ENSURE(pai);
				dc.first = *pai->ai_addr;//ignore the rest of the linked list
			}
		}



		clock_t remaining = 0;
		while((remaining = next-clock())>0){ //use subtraction to handle wraparound
			timeval t = { 0, long(remaining*1000000LL/CLOCKS_PER_SEC) };
			//cout<<t.tv_usec<<endl;
			fd_set f;
			FD_ZERO(&f);
			FD_SET(s, &f);
			int r = select( s+1, &f, NULL, NULL, &t );
			ENSURE(r>=0);
			if(r){
				assert(1==r);
				ENSURE(FD_ISSET(s, &f));
				int l = recvfrom(s,g_buf,c_buf_size,0,NULL,NULL);
				//cout<<l<<endl;
#ifdef WIN32
				//On a UDP-datagram socket, WSAECONNRESET would indicate that a previous send operation resulted in an ICMP "Port Unreachable" message.
				ENSURE(l>0||(-1==l && WSAECONNRESET==WSAGetLastError()));
				if(l>0)
#else
				ENSURE(l>0);
#endif
				{//strstream is deprecated and I want to avoid the copying incurred by a std::stringstream here, so the only choice is to use plain old snscanf
					ENSURE(l<c_buf_size);
					const char *end = g_buf+l;
					g_buf[l] = '\0';
					char sbuf[64];
					const char *p = g_buf;
					if( 1==snscanf(p,end-p,"%60s\n%n",sbuf,&l) ){
						p+=l; ENSURE(p<=end);
						string hop_to(sbuf);
						sn_type s;
						cost_type c;
						while(3==snscanf(p,end-p,"%60s %d %f\n%n",sbuf,&s,&c,&l)){
							p+=l; ENSURE(p<=end);
							const string dest(sbuf);
							if(dest == name) continue;
							rt_type &rt = rts[dest];//in case of a new entry, std::pair will default-construct its int to 0, and the seq# received is always positive
							lk_type &lk = rt.second;
							dc_tbl_type::const_iterator i = dcs.find(hop_to);
							ENSURE(i!=dcs.end());
							if(c<0) c = numeric_limits<cost_type>::infinity();// The text representation of infinity varies across compilers, so a negative floating-point number is sent instead
							c+=i->second.second;
							if( s>lk.first || (s==lk.first&&c<lk.second) ){
								rt = rt_type(hop_to,lk_type(s,c));
							}
						}
					}
					//string from = 
				}
			}
		}
		next+=c_interval*CLOCKS_PER_SEC;




		cout<<"## print-out number "<<prtN<<'\n';
		char *start = g_buf + snprintf(g_buf,c_max_line_size,"%s\n",name.c_str());
		ENSURE(!*start);
		char *p = start;

#define PUT(N,S,C) \
	{ \
		const int r = snprintf( p,c_max_line_size,"%s %d %f\n",(N),(S),(C) ); \
		if(r>=c_max_line_size){ std::cout<<r<<endl<<g_buf; } \
		ENSURE(r>0 && r<c_max_line_size); \
		p+=r; \
	}

		PUT( name.c_str(), prtN*2, 0.0f );
		for(rt_tbl_type::const_iterator i=rts.begin();;){
			{
				const int len = p-g_buf;
				if(rts.end()==i||len+c_max_line_size>=max_datagram_size){
					for(dc_tbl_type::const_iterator i=dcs.begin(); i!= dcs.end(); ++i){ //put hostname resolution here so the program can handle host-file change
						sockaddr sa = i->second.first;//copying is needed under linux, and is still correct under Windows
						ENSURE(len == sendto(s,g_buf,len,0,(sockaddr*)&sa,sizeof(sa)));//it's said that udp always sends a full datagram
					}
					//cout<<g_buf;
					p = start;
				}
			}
			if(rts.end()==i) break;
			const char *n = i->first.c_str();
			const rt_type &rt = i->second;
			const lk_type &lk = rt.second;
			sn_type s = lk.first;
			cost_type c = lk.second;
			assert(c>=0);
			if(numeric_limits<float>::infinity() == c){
				++s;
				c = -1;
			}else{
				cout<<"shortest path to node "<<n<<" (seq# "<<s<<"): the next hop is "<<rt.first.c_str()<<" and the cost is "<<c<<endl;
			}
			PUT(n,s,c);
			++i;
		}
	}
	return 0;
}
