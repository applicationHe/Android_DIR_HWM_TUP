/*
 *Copyright	:(C) 2013 TOPVS
 *File name  	:hdLib.c
 *Description   :libhd.a file
 *Date		:2013.12.16
 *Author    	:liuqs
 */

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "udt.h"
#include "cc.h"
//#include "util.h"
#include "udtcLib.h"

#include <android/log.h>
#define	LOG_TAG    "udtcLib.c"
#define	LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define	LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


using namespace std;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//the following func are exported.
int udtc_initLib() //add by liuqs on 20131217
{
	UDT::startup();

	return 0;
}

int udtc_deinitLib() //add by liuqs on 20131217
{
	UDT::cleanup();

	return 0;
}


int udtc_socket(int af, int type, int protocol)
{
	int ret = UDT::socket(af, type, protocol);
	if(ret == UDT::INVALID_SOCK)
		return -1;	

	int val = 0;
	UDT::setsockopt(ret, 0, UDT_LINGER, &val, sizeof(val));
	
	return ret ;
}

int udtc_bind(int sock, struct sockaddr *addr, int addr_len)
{
	int ret = UDT::bind(sock, addr, addr_len);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}

int udtc_bind2(int udt_sock, int udp_sock)
{
	int ret = UDT::bind2(udt_sock, udp_sock);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}

int udtc_listen(int sock, int backlog)
{
	int ret = UDT::listen(sock, backlog);
	if(ret == UDT::ERROR)
		return -1;

	return ret;  
}

int udtc_accept(int sock, struct sockaddr *addr, int *addr_len)
{
	int ret = UDT::accept(sock, addr, addr_len);
	if(ret == UDT::INVALID_SOCK)
		return -1;

	return ret; 
}

int udtc_recv(int sock, char *buf, int len, int flags)
{
	int ret = UDT::recv(sock, buf, len, flags);
	if(ret == UDT::ERROR)
		return -1;

	return ret; 
}

int udtc_close(int sock)
{
	int ret = UDT::close(sock);
	if(ret == UDT::ERROR)
		return -1;

	return ret; 
}

int udtc_connect(int sock, struct sockaddr *addr, int addr_len)
{
	int ret = UDT::connect(sock, addr, addr_len);
	if(ret == UDT::ERROR)
		return -1;
	
	return ret;
}

int udtc_send(int sock, char *buf, int len, int flags)
{
	int ret = UDT::send(sock, buf, len, flags);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}

int udtc_perfmon(int sock, trace_info_t *trace)
{
	int ret = UDT::perfmon(sock, (UDT::TRACEINFO *)trace);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}

int udtc_recvn(int sock, char *buf, int len)
{
	int left_size = len;
	int recv_size = 0;
	while (left_size > 0)
	{
		int size = udtc_recv(sock, &buf[recv_size], left_size, 0);
		if(size < 0)
		{
			//LOGE("udtc_recv failed");
			return -1;
		}
	
		recv_size += size;
		left_size -= size;
	}

	return recv_size;
}

int udtc_sendn(int sock, char *buf, int len)
{
	int left_size = len;
	int send_size = 0;
	while (left_size > 0)
	{
		int size = udtc_send(sock, &buf[send_size], left_size, 0);
		if(size < 0)
		{
			//LOGE("udtc_send failed");
			return -1;
		}
	
		send_size += size;
		left_size -= size;
	}

	return send_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//epoll
int udtc_epoll_create()
{
	int ret = UDT::epoll_create();
	if(ret < 0)
		return -1;
	
	return ret;
}

int udtc_epoll_add_usock(int eid, int u, const int* events)
{
	int ret = UDT::epoll_add_usock(eid, u, events);
	if(ret < 0)
		return -1;
	
	return ret;
}

int udtc_epoll_remove_usock(int eid, int u)
{
	int ret = UDT::epoll_remove_usock(eid, u);
	if(ret < 0)
		return -1;
	
	return ret;
}

int udtc_epoll_wait2(int eid, int* readfds, int* rnum, int* writefds, int* wnum, long long msTimeOut)
{
	int ret = UDT::epoll_wait2(eid, readfds, rnum, writefds, wnum, msTimeOut);
	if(ret < 0)
		return -1;
	
	return ret;
}

int udtc_epoll_release(int eid)
{
	int ret = UDT::epoll_release(eid);
	if(ret < 0)
		return -1;

	return ret;
}
///////////////////////////////////////////////////////////////////////////////////////////////

int udtc_getsockopt(int sock, int level, int optname, char *optval, int *optlen)
{
	int ret = UDT::getsockopt(sock, level, (UDT::SOCKOPT)optname, optval, optlen);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}

int udtc_setsockopt(int sock, int level, int optname, char *optval, int optlen)
{
	int ret = UDT::setsockopt(sock, level, (UDT::SOCKOPT)optname, optval, optlen);
	if(ret == UDT::ERROR)
		return -1;

	return ret;
}









