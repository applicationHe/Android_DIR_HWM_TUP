/*
 *Copyright	:(C) 2014 TOPVS.
 *File name  	:udtcLib.h
 *Description	:head file of libudtcLib.a
 *Date		:2014-04-10
 *Author    	:liuqs
 */

#ifndef __UDTC_LIB_H__
#define __UDTC_LIB_H__

#include        <stdio.h>

enum EPOLLOpt
{
   // this values are defined same as linux epoll.h
   // so that if system values are used by mistake, they should have the same effect
   UDT_EPOLL_IN = 0x1,
   UDT_EPOLL_OUT = 0x4,
   UDT_EPOLL_ERR = 0x8
};

enum UDTOpt
{
   UDT_MSS,             // the Maximum Transfer Unit
   UDT_SNDSYN,          // if sending is blocking
   UDT_RCVSYN,          // if receiving is blocking
   UDT_CC,              // custom congestion control algorithm
   UDT_FC,              // Flight flag size (window size)
   UDT_SNDBUF,          // maximum buffer in sending queue
   UDT_RCVBUF,          // UDT receiving buffer size
   UDT_LINGER,          // waiting for unsent data when closing
   UDP_SNDBUF,          // UDP sending buffer size
   UDP_RCVBUF,          // UDP receiving buffer size
   UDT_MAXMSG,          // maximum datagram message size
   UDT_MSGTTL,          // time-to-live of a datagram message
   UDT_RENDEZVOUS,      // rendezvous connection mode
   UDT_SNDTIMEO,        // send() timeout
   UDT_RCVTIMEO,        // recv() timeout
   UDT_REUSEADDR,       // reuse an existing port or create a new one
   UDT_MAXBW,           // maximum bandwidth (bytes per second) that the connection can use
   UDT_STATE,           // current socket state, see UDTSTATUS, read only
   UDT_EVENT,           // current avalable events associated with the socket
   UDT_SNDDATA,         // size of data in the sending buffer
   UDT_RCVDATA          // size of data available for recv
};

typedef struct __trace_info_t
{
   // global measurements
   int64_t msTimeStamp;                 // time since the UDT entity is started, in milliseconds
   int64_t pktSentTotal;                // total number of sent data packets, including retransmissions
   int64_t pktRecvTotal;                // total number of received packets
   int pktSndLossTotal;                 // total number of lost packets (sender side)
   int pktRcvLossTotal;                 // total number of lost packets (receiver side)
   int pktRetransTotal;                 // total number of retransmitted packets
   int pktSentACKTotal;                 // total number of sent ACK packets
   int pktRecvACKTotal;                 // total number of received ACK packets
   int pktSentNAKTotal;                 // total number of sent NAK packets
   int pktRecvNAKTotal;                 // total number of received NAK packets
   int64_t usSndDurationTotal;		// total time duration when UDT is sending data (idle time exclusive)

   // local measurements
   int64_t pktSent;                     // number of sent data packets, including retransmissions
   int64_t pktRecv;                     // number of received packets
   int pktSndLoss;                      // number of lost packets (sender side)
   int pktRcvLoss;                      // number of lost packets (receiver side)
   int pktRetrans;                      // number of retransmitted packets
   int pktSentACK;                      // number of sent ACK packets
   int pktRecvACK;                      // number of received ACK packets
   int pktSentNAK;                      // number of sent NAK packets
   int pktRecvNAK;                      // number of received NAK packets
   double mbpsSendRate;                 // sending rate in Mb/s
   double mbpsRecvRate;                 // receiving rate in Mb/s
   int64_t usSndDuration;		// busy sending time (i.e., idle time exclusive)

   // instant measurements
   double usPktSndPeriod;               // packet sending period, in microseconds
   int pktFlowWindow;                   // flow window size, in number of packets
   int pktCongestionWindow;             // congestion window size, in number of packets
   int pktFlightSize;                   // number of packets on flight
   double msRTT;                        // RTT, in milliseconds
   double mbpsBandwidth;                // estimated bandwidth, in Mb/s
   int byteAvailSndBuf;                 // available UDT sender buffer size
   int byteAvailRcvBuf;                 // available UDT receiver buffer size
}trace_info_t;


#ifdef __cplusplus
extern "C" {
#endif

int udtc_initLib(); 
int udtc_deinitLib(); 

int udtc_socket(int af, int type, int protocol);
int udtc_bind(int sock, struct sockaddr *addr, int addr_len);
int udtc_bind2(int udt_sock, int udp_sock);

int udtc_listen(int sock, int backlog);
int udtc_accept(int sock, struct sockaddr *addr, int *addr_len);
int udtc_recv(int sock, char *buf, int len, int flags);
int udtc_close(int sock);

int udtc_connect(int sock, struct sockaddr *addr, int addr_len);
int udtc_send(int sock, char *buf, int len, int flags);
int udtc_perfmon(int sock, trace_info_t *trace);

int udtc_recvn(int sock, char *buf, int len);
int udtc_sendn(int sock, char *buf, int len);


int udtc_epoll_create();
int udtc_epoll_add_usock(int eid, int u, const int* events);
int udtc_epoll_remove_usock(int eid, int u);
int udtc_epoll_wait2(int eid, int* readfds, int* rnum, int* writefds, int* wnum, long long msTimeOut);
int udtc_epoll_release(int eid);


int udtc_getsockopt(int sock, int level, int optname, char *optval, int *optlen);
int udtc_setsockopt(int sock, int level, int optname, char *optval, int optlen);


#ifdef __cplusplus
}
#endif


#endif //__UDTC_LIB_H__


