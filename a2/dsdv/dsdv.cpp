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
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define HEAD_STR "wope3p4623=-23i0i9572hg;;;,.=-+47jbd"

#define ENSURE(s) if(!(s)){ \
	std::cout<<"FATAL "<<__LINE__<<'\n'; \
	/*char buf[128];*/ \
	/*std::ofstream("err.txt",std::ios_base::app)<<_strtime(buf)<<"FATAL"<<__LINE__<<std::endl;*/ \
	exit(0); \
}
using namespace std;

const int c_udp_rep = 2;
const time_t c_interval = 10;
const int c_buf_size = 1024;
const int c_dgram_size_ctrl = 512;
const int c_max_line_size = 50;
const int c_smallbuf_size = c_max_line_size*3;

char g_buf[c_buf_size];
char g_smallbuf[c_smallbuf_size];//used for sending incremental updates

template<typename T> const T to_actual_cost(const T &t){ return t<0 ? std::numeric_limits<T>::infinity() : t; }
template<typename T> const T to_portable_cost(const T &t){ return t==std::numeric_limits<T>::infinity() ? -1 : t; }


typedef float cost_type;
typedef int sn_type;//seq #
struct rt_rec{
	string nh;//next_hop
	sn_type s;
	cost_type c;
	rt_rec():s(0),c(-1.0){}
};
class rt_group{
	rt_rec el[2];
public:
	rt_rec &operator[](size_t i){
		return el[i];
	}
	const rt_rec &operator[](size_t i) const{
		return el[i];
	}
};
typedef map<string,rt_group> rt_tbl_type;
typedef pair< sockaddr, cost_type > dc_type; // direct connection
typedef map< string, dc_type > dc_tbl_type;

int main(int argc, char **argv){
#ifdef WIN32 
	{ 
		WSADATA wsaData;
		ENSURE(!WSAStartup(MAKEWORD(2,2), &wsaData));
	}
#endif
	ENSURE(3==argc);
	int sck = socket(AF_INET,SOCK_DGRAM,0);	
	ENSURE(sck>=0);

	rt_tbl_type rts;
	dc_tbl_type dcs;

	int max_datagram_size = 400;//576 is the minimum maximum reassembly buffer size.
	max_datagram_size = min(max_datagram_size,min(c_buf_size,c_dgram_size_ctrl));

	ENSURE(3*c_max_line_size<max_datagram_size);

	{
#ifndef WIN32 //not used on my computer
		int b = 1;
		ENSURE(!setsockopt(sck,SOL_SOCKET,SO_REUSEADDR,(char*)&b,sizeof(b)));
#endif
		sockaddr_in sa;
		sa.sin_family      = AF_INET;
		sa.sin_port        = htons((u_short)atoi(argv[1]));
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		ENSURE(!bind(sck,(sockaddr*)&sa,sizeof(sa)));
	}

#define BCAST(_REP,_BUF,_LEN) \
	for(int iRep = 0;iRep<(_REP);++iRep){ \
		for(dc_tbl_type::const_iterator i=dcs.begin(); i!= dcs.end(); ++i){ \
			if(i->second.second<0) continue; \
			sockaddr sa = i->second.first;/*copying is needed under linux, and is still correct under Windows*/ \
			ENSURE((_LEN) == sendto(sck,(_BUF),(_LEN),0,(sockaddr*)&sa,sizeof(sa)));/*it's said that udp always sends a full datagram*/ \
		} \
	} /*cout<<(_BUF);*/

	time_t next = time(NULL);
	//assert(next>0);
	sn_type seq_number = 2;
	for(sn_type prtN=1;true;++prtN){
		seq_number = prtN*2;
		string name;
		{
			int n = 0;
			ifstream st(argv[2]);
			st.rdbuf()->pubsetbuf(NULL,0);
			st>>n>>name;
			ENSURE(int(name.size())+1<=c_max_line_size);
			for(int i=0;i<n;++i){
				string n2;
				st>>n2;
				dc_type &dc = dcs[n2];//whether an insertion is needed is automatically handled by operator[]
				string host,port;
				st>>host>>dc.second>>port;
				struct addrinfo *pai;
				ENSURE(!getaddrinfo(host.c_str(), port.c_str(), NULL, &pai));
				ENSURE(pai);
				dc.first = *pai->ai_addr;//ignore the rest of the linked list
			}
		}

		{// handle link breakdown
			for(rt_tbl_type::iterator i=rts.begin(); i!=rts.end(); ++i){
				for(int j=0;j<2;++j){
					rt_rec &rt = i->second[j];
					if(rt.c>=0 && dcs.find(rt.nh)->second.second<0){
						rt.c = -1;
						++rt.s;
						const int len = snprintf(g_smallbuf,c_smallbuf_size,HEAD_STR"%s %d\n%s %d %f\n",name.c_str(),seq_number,i->first.c_str(),rt.s,rt.c);
						ENSURE(len>0 && len<c_smallbuf_size);
						BCAST(c_udp_rep,g_smallbuf,len);
						//cout<<g_smallbuf;
					}
				}
			}
		}

		time_t remaining = 0;
		while((remaining = next-time(NULL))>0){
			timeval t = { 0, 0 }; t.tv_usec = long(remaining*1000000);
			fd_set f;
			FD_ZERO(&f);
			FD_SET(sck, &f);
			int r = select( sck+1, &f, NULL, NULL, &t );
			ENSURE(r>=0);
			if(r){
				assert(1==r);
				ENSURE(FD_ISSET(sck, &f));
				int l = recvfrom(sck,g_buf,c_buf_size,0,NULL,NULL);
				//cout<<l<<endl;
#ifdef WIN32
				//On a UDP-datagram socket, WSAECONNRESET would indicate that a previous send operation resulted in an ICMP "Port Unreachable" message.
				ENSURE(l>0||(-1==l && WSAECONNRESET==WSAGetLastError()));
				if(l>0)
#else
				ENSURE(l>0);
#endif
				{//strstream is deprecated and I want to avoid the copying incurred by a std::stringstream here, so the only choice is to use snscanf
					ENSURE(l<c_buf_size);
					const char *end = g_buf+l;
					g_buf[l] = '\0';
					char sbuf[64];
					const char *p = g_buf;
					sn_type ss;
					//if( 2==snscanf(p,end-p,"%60s%d\n%n",sbuf,&ss,&l) ){
					if( 2==sscanf(p,HEAD_STR"%60s%d\n%n",sbuf,&ss,&l) ){ //there seems to be no snscanf under linux
						p+=l; ENSURE(p<=end);
						rt_rec new_rt;
						new_rt.nh = sbuf;
						dc_tbl_type::const_iterator i = dcs.find(new_rt.nh);
						ENSURE(i!=dcs.end());
						const cost_type dc_c = i->second.second;
						const cost_type dc_actual_c = to_actual_cost(dc_c);
						cost_type incoming_c;
						//while(3==snscanf(p,end-p,"%60s%d%f\n%n",sbuf,&new_rt.s,&incoming_c,&l)){
						while(3==sscanf(p,"%60s%d%f\n%n",sbuf,&new_rt.s,&incoming_c,&l)){
							p+=l; ENSURE(p<=end);
							const string dest(sbuf);
							if(dest == name) continue;
							rt_group &rt = rts[dest];
							const cost_type actual_c = dc_actual_c+to_actual_cost(incoming_c);
							new_rt.c = actual_c==numeric_limits<cost_type>::infinity() ? -1 : actual_c;
							bool dirty = false;
							if( new_rt.s>rt[0].s ){
								rt[1] = new_rt.s&1?new_rt:rt[0];
								rt[0] = new_rt;
								dirty = true;
							}else{
								for(int i=0;i<2;++i){
									if( new_rt.s==rt[i].s && actual_c<to_actual_cost(rt[i].c) ){
										rt[i] = new_rt;
										dirty = true;
									}
								}
							}
							if(dirty){
								const int len = snprintf(g_smallbuf,c_smallbuf_size,HEAD_STR"%s %d\n%s %d %f\n",name.c_str(),ss,dest.c_str(),new_rt.s,new_rt.c);
								ENSURE(len>0 && len<c_smallbuf_size);
								BCAST(c_udp_rep,g_smallbuf,len);
							}
						}
					}
					//string from = 
				}
			}
		}
		next+=c_interval;

		{// periodic update and printout

#define PUT(_NODE,_SEQN,_COST) \
	{ \
		const int r = snprintf( p,c_max_line_size,"%s %d %f\n",(_NODE),(_SEQN),(_COST) ); \
		ENSURE(r>0 && r<c_max_line_size); \
		p+=r; \
	}

			cout<<"## print-out number "<<prtN<<'\n';
			char *start = g_buf + snprintf(g_buf,c_max_line_size,HEAD_STR"%s %d\n",name.c_str(),seq_number);
			ENSURE(!*start);
			char *p = start;
			PUT( name.c_str(),seq_number,0.0f );
			for(rt_tbl_type::const_iterator i=rts.begin();;){
				{
					const int len = p-g_buf;
					if(rts.end()==i||len+c_max_line_size>=max_datagram_size){
						BCAST(c_udp_rep,g_buf,len);
						//cout<<g_buf;
						p = start;
					}
				}
				if(rts.end()==i) break;
				const char *n = i->first.c_str();
				const rt_group &rg = i->second;
				const rt_rec &rt = to_actual_cost(rg[0].c)<=to_actual_cost(rg[1].c)?rg[0]:rg[1];
				if(rt.c>=0){
					cout<<"shortest path to node "<<n<<" (seq# "<<rt.s<<"): the next hop is "<<rt.nh.c_str()<<" and the cost is "<<rt.c<<endl;
				}
				PUT(n,rt.s,rt.c);
				++i;
			}
		}
	}
	return 0;
}
