/* $Id: icedemo->c 4624 2013-10-21 06:37:30Z ming $ */
/*
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *remove something。 see p2p_cross.h
 */

#include "p2p_cross.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>   		// basic system data types
#include <sys/socket.h>  		// basic socket definitions
#include <sys/time.h>    		// timeval{} for select()
#include <netinet/in.h>  		// sockaddr_in{} and other Internet defns
#include <fcntl.h>
#include <pthread.h>

#include <udtc/jni/udtc/udtcLib.h>
#include <udtc/jni/udtc/proto.h>
#include <android/log.h>
#define  LOG_TAG "p2p_cross.c"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define	 printf(...)	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define	 puts(...)	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

typedef void (*pthread_cleanup_push_routine_t)(void*);

static pthread_mutexattr_t mutexattr;
static pthread_cond_t	ice_cond;
static pthread_mutex_t	ice_mutex;
static pthread_cond_t   nego_cond;
static pthread_mutex_t  nego_mutex;

/* This is our global variables */
#define THIS_FILE   "icedemo.c"

/* For this demo app, configure longer STUN keep-alive time
 * so that it does't clutter the screen output.
 */
#define KA_INTERVAL 300


g_app_t *icedemo;
static char* g_line[16] = {0};
static char sdp64[REMOTE_SDP_LEN];

static struct sockaddr_in server_addr;
static int server_sock;
static assist_hole_t assist_hole;

static struct pj_getopt_option long_options[] = {
	{ "comp-cnt",           1, 0, 'c'},
	{ "nameserver",		1, 0, 'n'},
	{ "max-host",		1, 0, 'H'},
	{ "help",		0, 0, 'h'},
	{ "stun-srv",		1, 0, 's'},
	{ "turn-srv",		1, 0, 't'},
	{ "turn-tcp",		0, 0, 'T'},
	{ "turn-username",	1, 0, 'u'},
	{ "turn-password",	1, 0, 'p'},
	{ "turn-fingerprint",	0, 0, 'F'},
	{ "regular",		0, 0, 'R'},
	{ "log-file",		1, 0, 'L'},
};


/* Utility to display error messages */
static void icedemo_perror(const char *title, pj_status_t status)
{
	char errmsg[PJ_ERR_MSG_SIZE];

	pj_strerror(status, errmsg, sizeof(errmsg));
	LOGE("%s: %s", title, errmsg);
}


/* Utility: display error message and exit application (usually
 * because of fatal error.
 */
static void err_exit(const char *title, pj_status_t status)
{
	if (status != PJ_SUCCESS)
	{
		icedemo_perror(title, status);
	}
	LOGI("Shutting down..");

	if (icedemo->icest)
		pj_ice_strans_destroy(icedemo->icest);

	pj_thread_sleep(500);

	icedemo->thread_quit_flag = PJ_TRUE;
	if (icedemo->thread)
	{
		pj_thread_join(icedemo->thread);
		pj_thread_destroy(icedemo->thread);
	}

	if (icedemo->ice_cfg.stun_cfg.ioqueue)
		pj_ioqueue_destroy(icedemo->ice_cfg.stun_cfg.ioqueue);

	if (icedemo->ice_cfg.stun_cfg.timer_heap)
		pj_timer_heap_destroy(icedemo->ice_cfg.stun_cfg.timer_heap);

	pj_caching_pool_destroy(&icedemo->cp);

	pj_shutdown();

	if (icedemo->log_fhnd)
	{
		fclose(icedemo->log_fhnd);
		icedemo->log_fhnd = NULL;
	}

//	exit(status != PJ_SUCCESS);
}

#define CHECK(expr)	status=expr; \
			if (status!=PJ_SUCCESS) { \
			    err_exit(#expr, status); \
			}

/*
 * This function checks for events from both timer and ioqueue (for
 * network events). It is invoked by the worker thread.
 */
static pj_status_t handle_events(unsigned max_msec, unsigned *p_count)
{
	enum
	{
		MAX_NET_EVENTS = 1
	};
	pj_time_val max_timeout = { 0, 0 };
	pj_time_val timeout = { 0, 0 };
	unsigned count = 0, net_event_count = 0;
	int c;

	max_timeout.msec = max_msec;

	/* Poll the timer to run it and also to retrieve the earliest entry. */
	timeout.sec = timeout.msec = 0;
	c = pj_timer_heap_poll(icedemo->ice_cfg.stun_cfg.timer_heap, &timeout);
	if (c > 0)
		count += c;

	/* timer_heap_poll should never ever returns negative value, or otherwise
	 * ioqueue_poll() will block forever!
	 */
	pj_assert(timeout.sec >= 0 && timeout.msec >= 0);
	if (timeout.msec >= 1000)
		timeout.msec = 999;

	/* compare the value with the timeout to wait from timer, and use the
	 * minimum value.
	 */
	if (PJ_TIME_VAL_GT(timeout, max_timeout))
		timeout = max_timeout;

	/* Poll ioqueue.
	 * Repeat polling the ioqueue while we have immediate events, because
	 * timer heap may process more than one events, so if we only process
	 * one network events at a time (such as when IOCP backend is used),
	 * the ioqueue may have trouble keeping up with the request rate.
	 *
	 * For example, for each send() request, one network event will be
	 *   reported by ioqueue for the send() completion. If we don't poll
	 *   the ioqueue often enough, the send() completion will not be
	 *   reported in timely manner.
	 */
	do
	{
		c = pj_ioqueue_poll(icedemo->ice_cfg.stun_cfg.ioqueue, &timeout);
		if (c < 0)
		{
			pj_status_t err = pj_get_netos_error();
			pj_thread_sleep(PJ_TIME_VAL_MSEC(timeout));
			if (p_count)
				*p_count = count;
			return err;
		}
		else if (c == 0)
		{
			break;
		}
		else
		{
			net_event_count += c;
			timeout.sec = timeout.msec = 0;
		}
	} while (c > 0 && net_event_count < MAX_NET_EVENTS);

	count += net_event_count;
	if (p_count)
		*p_count = count;

	return PJ_SUCCESS;

}

/*
 * This is the worker thread that polls event in the background.
 */
static int icedemo_worker_thread(void *unused)
{
	PJ_UNUSED_ARG(unused);

	while (!icedemo->thread_quit_flag)
	{
		handle_events(200, NULL);
	}

	return 0;
}

/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about incoming data. By "data" it means application
 * data such as RTP/RTCP, and not packets that belong to ICE signaling (such
 * as STUN connectivity checks or TURN signaling).
 */
static void cb_on_rx_data(pj_ice_strans *ice_st, unsigned comp_id, void *pkt, pj_size_t size,
        const pj_sockaddr_t *src_addr, unsigned src_addr_len)
{
	char ipstr[PJ_INET6_ADDRSTRLEN + 10];

	PJ_UNUSED_ARG(ice_st);
	PJ_UNUSED_ARG(src_addr_len);
	PJ_UNUSED_ARG(pkt);

	// Don't do this! It will ruin the packet buffer in case TCP is used!
	//((char*)pkt)[size] = '\0';

//	LOGI("Component %d: received %d bytes data from %s: \"%.*s\"", comp_id, size,
//		 pj_sockaddr_print(src_addr, ipstr, sizeof(ipstr), 3), (unsigned) size, (char*) pkt);
	char tmp[size+1];
	sprintf(tmp,"%.*s",(unsigned) size,(char*) pkt);
	if(strcmp(tmp, "stop_sess") == 0)
	{
		LOGI("recv the ipc stop_sess");
		pthread_cond_signal(&ice_cond);
	}
}

/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about ICE state progression.
 */
static void cb_on_ice_complete(pj_ice_strans *ice_st, pj_ice_strans_op op, pj_status_t status)
{
	const char *opname = (
	        op == PJ_ICE_STRANS_OP_INIT ?
	                "initialization" : (op == PJ_ICE_STRANS_OP_NEGOTIATION ? "negotiation" : "unknown_op"));

	if (status == PJ_SUCCESS)
	{
		LOGI("ICE %s successful", opname);
		pthread_cond_signal(&nego_cond);
	}
	else
	{
		char errmsg[PJ_ERR_MSG_SIZE];

		pj_strerror(status, errmsg, sizeof(errmsg));
		LOGE("ICE %s failed: %s", opname, errmsg);
		pj_ice_strans_destroy(ice_st);
		icedemo->icest = NULL;
	}
}

/* log callback to write to file */
static void log_func(int level, const char *data, int len)
{
	pj_log_write(level, data, len);
	if (icedemo->log_fhnd)
	{
		if (fwrite(data, len, 1, icedemo->log_fhnd) != 1)
			return;
	}
}

/*
 * This is the main application initialization function. It is called
 * once (and only once) during application initialization sequence by
 * main().
 */
static pj_status_t icedemo_init(void)
{
	pj_status_t status;

	if (icedemo->opt.log_file)
	{
		icedemo->log_fhnd = fopen(icedemo->opt.log_file, "a");
		pj_log_set_log_func(&log_func);
	}

	/* Initialize the libraries before anything else */
	CHECK(pj_init());
	CHECK(pjlib_util_init());
	CHECK(pjnath_init());

	/* Must create pool factory, where memory allocations come from */
	pj_caching_pool_init(&icedemo->cp, NULL, 0);

	/* Init our ICE settings with null values */
	pj_ice_strans_cfg_default(&icedemo->ice_cfg);

	icedemo->ice_cfg.stun_cfg.pf = &icedemo->cp.factory;

	/* Create application memory pool */
	icedemo->pool = pj_pool_create(&icedemo->cp.factory, "icedemo", 512, 512, NULL);

	/* Create timer heap for timer stuff */
	CHECK(pj_timer_heap_create(icedemo->pool, 100, &icedemo->ice_cfg.stun_cfg.timer_heap));

	/* and create ioqueue for network I/O stuff */
	CHECK(pj_ioqueue_create(icedemo->pool, 16, &icedemo->ice_cfg.stun_cfg.ioqueue));

	/* something must poll the timer heap and ioqueue,
	 * unless we're on Symbian where the timer heap and ioqueue run
	 * on themselves.
	 */
	CHECK(pj_thread_create(icedemo->pool, "icedemo", &icedemo_worker_thread, NULL, 0, 0, &icedemo->thread));

	icedemo->ice_cfg.af = pj_AF_INET();

	/* Create DNS resolver if nameserver is set */
	if (icedemo->opt.ns.slen)
	{
		CHECK(pj_dns_resolver_create(&icedemo->cp.factory, "resolver", 0,
				icedemo->ice_cfg.stun_cfg.timer_heap,
				icedemo->ice_cfg.stun_cfg.ioqueue,
				&icedemo->ice_cfg.resolver));

		CHECK(pj_dns_resolver_set_ns(icedemo->ice_cfg.resolver, 1, &icedemo->opt.ns, NULL));
	}

	/* -= Start initializing ICE stream transport config =- */

	/* Maximum number of host candidates */
	if (icedemo->opt.max_host != -1)
		icedemo->ice_cfg.stun.max_host_cands = icedemo->opt.max_host;

	/* Nomination strategy */
	if (icedemo->opt.regular)
		icedemo->ice_cfg.opt.aggressive = PJ_FALSE;
	else
		icedemo->ice_cfg.opt.aggressive = PJ_TRUE;

	/* Configure STUN/srflx candidate resolution */
	if (icedemo->opt.stun_srv.slen)
	{
		char *pos;

		/* Command line option may contain port number */
		if ((pos = pj_strchr(&icedemo->opt.stun_srv, ':')) != NULL)
		{
			icedemo->ice_cfg.stun.server.ptr = icedemo->opt.stun_srv.ptr;
			icedemo->ice_cfg.stun.server.slen = (pos - icedemo->opt.stun_srv.ptr);
			icedemo->ice_cfg.stun.port = (pj_uint16_t) atoi(pos + 1);
		}
		else
		{
			icedemo->ice_cfg.stun.server = icedemo->opt.stun_srv;
			icedemo->ice_cfg.stun.port = PJ_STUN_PORT;
		}

		/* For this demo app, configure longer STUN keep-alive time
		 * so that it does't clutter the screen output.
		 */
		icedemo->ice_cfg.stun.cfg.ka_interval = KA_INTERVAL;
	}

	/* Configure TURN candidate */
	if (icedemo->opt.turn_srv.slen)
	{
		char *pos;

		/* Command line option may contain port number */
		if ((pos = pj_strchr(&icedemo->opt.turn_srv, ':')) != NULL)
		{
			icedemo->ice_cfg.turn.server.ptr = icedemo->opt.turn_srv.ptr;
			icedemo->ice_cfg.turn.server.slen = (pos - icedemo->opt.turn_srv.ptr);
			icedemo->ice_cfg.turn.port = (pj_uint16_t) atoi(pos + 1);
		}
		else
		{
			icedemo->ice_cfg.turn.server = icedemo->opt.turn_srv;
			icedemo->ice_cfg.turn.port = PJ_STUN_PORT;
		}

		/* TURN credential */
		icedemo->ice_cfg.turn.auth_cred.type = PJ_STUN_AUTH_CRED_STATIC;
		icedemo->ice_cfg.turn.auth_cred.data.static_cred.username = icedemo->opt.turn_username;
		icedemo->ice_cfg.turn.auth_cred.data.static_cred.data_type = PJ_STUN_PASSWD_PLAIN;
		icedemo->ice_cfg.turn.auth_cred.data.static_cred.data = icedemo->opt.turn_password;

		/* Connection type to TURN server */
		if (icedemo->opt.turn_tcp)
			icedemo->ice_cfg.turn.conn_type = PJ_TURN_TP_TCP;
		else
			icedemo->ice_cfg.turn.conn_type = PJ_TURN_TP_UDP;

		/* For this demo app, configure longer keep-alive time
		 * so that it does't clutter the screen output.
		 */
		icedemo->ice_cfg.turn.alloc_param.ka_interval = KA_INTERVAL;
	}

	/* -= That's it for now, initialization is complete =- */
	return PJ_SUCCESS;
}

/*
 * Create ICE stream transport instance, invoked from the menu.
 */
static void icedemo_create_instance(void)
{
	pj_ice_strans_cb icecb;
	pj_status_t status;

	if (icedemo->icest != NULL)
	{
		puts("ICE instance already created, destroy it first");
		return;
	}

	/* init the callback */
	pj_bzero(&icecb, sizeof(icecb));
	icecb.on_rx_data = cb_on_rx_data;
	icecb.on_ice_complete = cb_on_ice_complete;

	/* create the instance */
	status = pj_ice_strans_create("icedemo", /* object name  */
			&icedemo->ice_cfg,		/* settings	    */
			icedemo->opt.comp_cnt,	/* comp_cnt	    */
			NULL,					/* user data    */
			&icecb,					/* callback	    */
			&icedemo->icest)	;		/* instance ptr */

	if (status != PJ_SUCCESS)
		icedemo_perror("error creating ice", status);
	else
		LOGI("ICE instance successfully created");
}

/* Utility to nullify parsed remote info */
static void reset_rem_info(void)
{
	pj_bzero(&icedemo->rem, sizeof(icedemo->rem));
}

/*
 * Destroy ICE stream transport instance, invoked from the menu.
 */
static void icedemo_destroy_instance(void)
{
	LOGI("icedemo_destroy_instance.");
	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	pj_ice_strans_destroy(icedemo->icest);
	icedemo->icest = NULL;

	reset_rem_info();

	LOGI("ICE instance destroyed");
}

/*
 * Create ICE session, invoked from the menu.
 */
static void icedemo_init_session(unsigned rolechar)
{
	LOGI("ICE ice_init_session.");
	pj_ice_sess_role role = (
			pj_tolower((pj_uint8_t) rolechar) == 'o' ? PJ_ICE_SESS_ROLE_CONTROLLING : PJ_ICE_SESS_ROLE_CONTROLLED);
	pj_status_t status;

	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	if (pj_ice_strans_has_sess(icedemo->icest))
	{
		LOGE("Error: Session already created");
		return;
	}

	status = pj_ice_strans_init_ice(icedemo->icest, role, NULL, NULL);
	if (status != PJ_SUCCESS)
		icedemo_perror("error creating session", status);
	else
		LOGI("ICE session created");

	reset_rem_info();
}

/*
 * Stop/destroy ICE session, invoked from the menu.
 */
static void icedemo_stop_session(void)
{
	LOGI("icedemo_stop_session.");
	pj_status_t status;

	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	if (!pj_ice_strans_has_sess(icedemo->icest))
	{
		LOGE("Error: No ICE session, initialize first");
		return;
	}

	status = pj_ice_strans_stop_ice(icedemo->icest);
	if (status != PJ_SUCCESS)
		icedemo_perror("error stopping session", status);
	else
		LOGI("ICE session stopped");

	reset_rem_info();
}

#define PRINT(...)	    \
	printed = pj_ansi_snprintf(p, maxlen - (p-buffer),  \
				   __VA_ARGS__); \
	if (printed <= 0 || printed >= (int)(maxlen - (p-buffer))) \
	    return -PJ_ETOOSMALL; \
	p += printed

/* Utility to create a=candidate SDP attribute */
static int print_cand(char buffer[], unsigned maxlen, const pj_ice_sess_cand *cand)
{
	char ipaddr[PJ_INET6_ADDRSTRLEN];
	char *p = buffer;
	int printed;

	PRINT("a=candidate:%.*s %u UDP %u %s %u typ ", (int )cand->foundation.slen, cand->foundation.ptr,
	        (unsigned )cand->comp_id, cand->prio, pj_sockaddr_print(&cand->addr, ipaddr, sizeof(ipaddr), 0),
	        (unsigned )pj_sockaddr_get_port(&cand->addr));

	PRINT("%s\n", pj_ice_get_cand_type_name(cand->type));

	if (p == buffer + maxlen)
		return -PJ_ETOOSMALL;

	*p = '\0';

	return (int) (p - buffer);
}

/*
 * Encode ICE information in SDP.
 */
static int encode_session(char buffer[], unsigned maxlen)
{
	char *p = buffer;
	unsigned comp;
	int printed;
	pj_str_t local_ufrag, local_pwd;
	pj_status_t status;

	/* Write "dummy" SDP v=, o=, s=, and t= lines */
	PRINT("v=0\no=- 3414953978 3414953978 IN IP4 localhost\ns=ice\nt=0 0\n");

	/* Get ufrag and pwd from current session */
	pj_ice_strans_get_ufrag_pwd(icedemo->icest, &local_ufrag, &local_pwd,
	NULL, NULL);

	/* Write the a=ice-ufrag and a=ice-pwd attributes */
	PRINT("a=ice-ufrag:%.*s\na=ice-pwd:%.*s\n", (int )local_ufrag.slen, local_ufrag.ptr, (int )local_pwd.slen,
	        local_pwd.ptr);

	/* Write each component */
	for (comp = 0; comp < icedemo->opt.comp_cnt; ++comp)
	{
		unsigned j, cand_cnt;
		pj_ice_sess_cand cand[PJ_ICE_ST_MAX_CAND];
		char ipaddr[PJ_INET6_ADDRSTRLEN];

		/* Get default candidate for the component */
		status = pj_ice_strans_get_def_cand(icedemo->icest, comp + 1, &cand[0]);
		if (status != PJ_SUCCESS)
			return -status;

		/* Write the default address */
		if (comp == 0)
		{
			/* For component 1, default address is in m= and c= lines */
			PRINT("m=audio %d RTP/AVP 0\n"
					"c=IN IP4 %s\n", (int )pj_sockaddr_get_port(&cand[0].addr),
			        pj_sockaddr_print(&cand[0].addr, ipaddr, sizeof(ipaddr), 0));
		}
		else if (comp == 1)
		{
			/* For component 2, default address is in a=rtcp line */
			PRINT("a=rtcp:%d IN IP4 %s\n", (int )pj_sockaddr_get_port(&cand[0].addr),
			        pj_sockaddr_print(&cand[0].addr, ipaddr, sizeof(ipaddr), 0));
		}
		else
		{
			/* For other components, we'll just invent this.. */
			PRINT("a=Xice-defcand:%d IN IP4 %s\n", (int )pj_sockaddr_get_port(&cand[0].addr),
			        pj_sockaddr_print(&cand[0].addr, ipaddr, sizeof(ipaddr), 0));
		}

		/* Enumerate all candidates for this component */
		cand_cnt = PJ_ARRAY_SIZE(cand);
		status = pj_ice_strans_enum_cands(icedemo->icest, comp + 1, &cand_cnt, cand);
		if (status != PJ_SUCCESS)
			return -status;

		/* And encode the candidates as SDP */
		for (j = 0; j < cand_cnt; ++j)
		{
			printed = print_cand(p, maxlen - (unsigned) (p - buffer), &cand[j]);
			if (printed < 0)
				return -PJ_ETOOSMALL;
			p += printed;
		}
	}

	if (p == buffer + maxlen)
		return -PJ_ETOOSMALL;

	*p = '\0';
	return (int) (p - buffer);
}

/*
 * Show information contained in the ICE stream transport. This is
 * invoked from the menu.
 */
static void icedemo_show_ice(void)
{
	static char buffer[REMOTE_SDP_LEN];
	int len;

	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	puts("General info");
	puts("--------------------");
	printf("Component count    : %d\n", icedemo->opt.comp_cnt);
	printf("Status             : ");
	if (pj_ice_strans_sess_is_complete(icedemo->icest))
		puts("negotiation complete");
	else if (pj_ice_strans_sess_is_running(icedemo->icest))
		puts("negotiation is in progress");
	else if (pj_ice_strans_has_sess(icedemo->icest))
		puts("session ready");
	else
		puts("session not created");

	if (!pj_ice_strans_has_sess(icedemo->icest))
	{
		puts("Create the session first to see more info");
		return;
	}

	printf("Negotiated comp_cnt: %d\n", pj_ice_strans_get_running_comp_cnt(icedemo->icest));
	printf("Role               : %s\n",
	        pj_ice_strans_get_role(icedemo->icest) == PJ_ICE_SESS_ROLE_CONTROLLED ? "controlled" : "controlling");

	len = encode_session(buffer, sizeof(buffer));
	if (len < 0)
		err_exit("not enough buffer to show ICE status", -len);

	puts("");
	printf("Local SDP (paste this to remote host):");
	printf("--------------------------------------");
	printf("%s\n", buffer);

	//copy local sdp to global variable
	memset(sdp64, 0, REMOTE_SDP_LEN);
	strcpy(sdp64, buffer);

	puts("");
	puts("Remote info:\n"
		 "----------------------");
	if (icedemo->rem.cand_cnt == 0)
	{
		puts("  No remote info yet");
	}
	else
	{
		unsigned i;

		printf("Remote ufrag       : %s\n", icedemo->rem.ufrag);
		printf("Remote password    : %s\n", icedemo->rem.pwd);
		printf("Remote cand. cnt.  : %d\n", icedemo->rem.cand_cnt);

		for (i = 0; i < icedemo->rem.cand_cnt; ++i)
		{
			len = print_cand(buffer, sizeof(buffer), &icedemo->rem.cand[i]);
			if (len < 0)
				err_exit("not enough buffer to show ICE status", -len);

			printf("  %s", buffer);
		}
	}
}

/*
 * Input and parse SDP from the remote (containing remote's ICE information)
 * and save it to global variables.
 */
// ********************* add by LinZh107 ********************
static void icedemo_input_remote(void)
{
	unsigned media_cnt = 0;
	unsigned comp0_port = 0;
	char comp0_addr[80];

	puts("Paste SDP from remote host, end with empty line");
	LOGI("----------------------");

	reset_rem_info();

	comp0_addr[0] = '\0';

	int lines = 0;
	int curpt = 0;
	int offset = 0;
	int tmpsize = 0;
	pj_size_t len;
	len = strlen(sdp64);
	while (curpt <= len )
	{
		if(sdp64[curpt] == '\r' || sdp64[curpt] == '\n')
		{
			// parsing remote iceinfo to lines. add by LinZh107
			char tmp[128] = {0};
			tmpsize = curpt - tmpsize;
			memcpy(tmp, &sdp64[offset], tmpsize);

			// remove other character that we don't need. add by LinZh107
			int i=0;
			while(tmp[i] > 31)
				i++	;
			char *linec = malloc( i+1 * sizeof(char));
			memset(linec, 0, i+1 * sizeof(char) );
			memcpy(linec, tmp, i);

			// gathering the useful iceinfo. add by LinZh107
			g_line[lines] = linec;
			//LOGI("%d: %s", lines, g_line[lines]);

			// remove other character that we don't need. add by LinZh107
			while(sdp64[curpt]>122 || sdp64[curpt]<97)
			{
				curpt++;
				tmpsize++;
			}
			offset = curpt;
			lines ++;
		}
		curpt ++;
	}

	int i = 0;
	while(i < lines)
	{
		switch ( (g_line[i])[0] )
		{
		case 'm':
		{
			int cnt;
			char media[32], portstr[32];

			++media_cnt;
			if (media_cnt > 1)
			{
				puts("Media line ignored");
				break;
			}

			cnt = sscanf(g_line[i]+2, "%s %s RTP/", media, portstr);
			if (cnt != 2)
			{
				LOGE("Error parsing media line");
				goto on_error;
			}

			comp0_port = atoi(portstr);
		}
			break;
		case 'c':
		{
			int cnt;
			char c[32], net[32], ip[80];

			cnt = sscanf(g_line[i]+2, "%s %s %s", c, net, ip);
			if (cnt != 3)
			{
				LOGE("Error parsing connection line");
				goto on_error;
			}

			strcpy(comp0_addr, ip);
		}
			break;
		case 'a':
		{
			char *attr = strtok(g_line[i]+2, ": \t\r\n");
			if (strcmp(attr, "ice-ufrag") == 0)
			{
				strcpy(icedemo->rem.ufrag, attr + strlen(attr) + 1);
			}
			else if (strcmp(attr, "ice-pwd") == 0)
			{
				strcpy(icedemo->rem.pwd, attr + strlen(attr) + 1);
			}
			else if (strcmp(attr, "rtcp") == 0)
			{
				char *val = attr + strlen(attr) + 1;
				int af, cnt;
				int port;
				char net[32], ip[64];
				pj_str_t tmp_addr;
				pj_status_t status;

				cnt = sscanf(val, "%d IN %s %s", &port, net, ip);
				if (cnt != 3)
				{
					LOGE("Error parsing rtcp attribute");
					goto on_error;
				}

				if (strchr(ip, ':'))
					af = pj_AF_INET6();
				else
					af = pj_AF_INET();

				pj_sockaddr_init(af, &icedemo->rem.def_addr[1], NULL, 0);
				tmp_addr = pj_str(ip);
				status = pj_sockaddr_set_str_addr(af, &icedemo->rem.def_addr[1], &tmp_addr);
				if (status != PJ_SUCCESS)
				{
					LOGE("Invalid IP address");
					goto on_error;
				}
				pj_sockaddr_set_port(&icedemo->rem.def_addr[1], (pj_uint16_t) port);

			}
			else if (strcmp(attr, "candidate") == 0)
			{
				char *sdpcand = attr + strlen(attr) + 1;
				int af, cnt;
				char foundation[32], transport[12], ipaddr[80], type[32];
				pj_str_t tmpaddr;
				int comp_id, prio, port;
				pj_ice_sess_cand *cand;
				pj_status_t status;

				cnt = sscanf(sdpcand, "%s %d %s %d %s %d typ %s", foundation, &comp_id, transport, &prio, ipaddr, &port,
						type);
				if (cnt != 7)
				{
					LOGE("error: Invalid ICE candidate line");
					goto on_error;
				}

				cand = &icedemo->rem.cand[icedemo->rem.cand_cnt];
				pj_bzero(cand, sizeof(*cand));

				if (strcmp(type, "host") == 0)
					cand->type = PJ_ICE_CAND_TYPE_HOST;
				else if (strcmp(type, "srflx") == 0)
					cand->type = PJ_ICE_CAND_TYPE_SRFLX;
				else if (strcmp(type, "relay") == 0)
					cand->type = PJ_ICE_CAND_TYPE_RELAYED;
				else
				{
					LOGE("Error: invalid candidate type '%s'", type);
					goto on_error;
				}

				cand->comp_id = (pj_uint8_t) comp_id;
				pj_strdup2(icedemo->pool, &cand->foundation, foundation);
				cand->prio = prio;

				if (strchr(ipaddr, ':'))
					af = pj_AF_INET6();
				else
					af = pj_AF_INET();

				tmpaddr = pj_str(ipaddr);
				pj_sockaddr_init(af, &cand->addr, NULL, 0);
				status = pj_sockaddr_set_str_addr(af, &cand->addr, &tmpaddr);
				if (status != PJ_SUCCESS)
				{
					LOGE("Error: invalid IP address '%s'", ipaddr);
					goto on_error;
				}

				pj_sockaddr_set_port(&cand->addr, (pj_uint16_t) port);

				++icedemo->rem.cand_cnt;

				if (cand->comp_id > icedemo->rem.comp_cnt)
					icedemo->rem.comp_cnt = cand->comp_id;
			}
		}	// end case.
			break;
		}	// end switch
		free(g_line[i]);
		g_line[i] = NULL;
		i ++;
	} // end while(lines --)

	if (icedemo->rem.cand_cnt == 0 || icedemo->rem.ufrag[0] == 0 || icedemo->rem.pwd[0] == 0 || icedemo->rem.comp_cnt == 0)
	{
		LOGE("Error: not enough info");
		goto on_error;
	}

	if (comp0_port == 0 || comp0_addr[0] == '\0')
	{
		LOGE("Error: default address for component 0 not found");
		goto on_error;
	}
	else
	{
		int af;
		pj_str_t tmp_addr;
		pj_status_t status;

		if (strchr(comp0_addr, ':'))
			af = pj_AF_INET6();
		else
			af = pj_AF_INET();

		pj_sockaddr_init(af, &icedemo->rem.def_addr[0], NULL, 0);
		tmp_addr = pj_str(comp0_addr);
		status = pj_sockaddr_set_str_addr(af, &icedemo->rem.def_addr[0], &tmp_addr);
		if (status != PJ_SUCCESS)
		{
			LOGE("Invalid IP address in c= line");
			goto on_error;
		}
		pj_sockaddr_set_port(&icedemo->rem.def_addr[0], (pj_uint16_t) comp0_port);
	}

	LOGI("End input remote ICE info.");
	return;

	on_error: reset_rem_info();
}

/*
 * Start ICE negotiation! This function is invoked from the menu.
 */
static void icedemo_start_nego(void)
{
	LOGI("Calling ICE negotiation .");

	pj_str_t rufrag, rpwd;
	pj_status_t status;

	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	if (!pj_ice_strans_has_sess(icedemo->icest))
	{
		LOGE("Error: No ICE session, initialize first");
		return;
	}

	if (icedemo->rem.cand_cnt == 0)
	{
		LOGE("Error: No remote info, input remote info first");
		return;
	}

//	LOGI("cand_cnt=%d, ufrag=%s, pwd=%s, comp_cnt=%d",
//			icedemo->rem.cand_cnt, icedemo->rem.ufrag, icedemo->rem.pwd, icedemo->rem.comp_cnt);

	status = pj_ice_strans_start_ice(icedemo->icest, pj_cstr(&rufrag, icedemo->rem.ufrag),
	        pj_cstr(&rpwd, icedemo->rem.pwd), icedemo->rem.cand_cnt, icedemo->rem.cand);
	if (status != PJ_SUCCESS)
		icedemo_perror("Error starting ICE", status);
	else
		LOGI("Calling ICE negotiation ...");
}

/*
 * Send application data to remote agent.
 */
static void icedemo_send_data(unsigned comp_id, const char *data)
{
	pj_status_t status;

	if (icedemo->icest == NULL)
	{
		LOGE("Error: No ICE instance, create it first");
		return;
	}

	if (!pj_ice_strans_has_sess(icedemo->icest))
	{
		LOGE("Error: No ICE session, initialize first");
		return;
	}

	/*
	 if (!pj_ice_strans_sess_is_complete(icedemo->icest)) {
	 PJ_LOG(1,(THIS_FILE, "Error: ICE negotiation has not been started or is in progress"));
	 return;
	 }
	 */

	if (comp_id < 1 || comp_id > pj_ice_strans_get_running_comp_cnt(icedemo->icest))
	{
		LOGE("Error: invalid component ID");
		return;
	}

	status = pj_ice_strans_sendto(icedemo->icest, comp_id, data, strlen(data), &icedemo->rem.def_addr[comp_id - 1],
	        pj_sockaddr_get_len(&icedemo->rem.def_addr[comp_id - 1]));
	if (status != PJ_SUCCESS)
		icedemo_perror("Error sending data", status);
	else
		LOGI("Data sent");
}

/*
 * Display help for the menu.
 */
static void icedemo_help_menu(void)
{
	puts("");
	puts("-= Help on using ICE and this icedemo program =-");
	puts("");
	puts("This application demonstrates how to use ICE in pjnath without having\n"
			"to use the SIP protocol. To use this application, you will need to run\n"
			"two instances of this application, to simulate two ICE agents.\n");

	puts("Basic ICE flow:\n"
			" create instance [menu \"c\"]\n"
			" repeat these steps as wanted:\n"
			"   - init session as offerer or answerer [menu \"i\"]\n"
			"   - display our SDP [menu \"s\"]\n"
			"   - \"send\" our SDP from the \"show\" output above to remote, by\n"
			"     copy-pasting the SDP to the other icedemo application\n"
			"   - parse remote SDP, by pasting SDP generated by the other icedemo\n"
			"     instance [menu \"r\"]\n"
			"   - begin ICE negotiation in our end [menu \"b\"], and \n"
			"   - immediately begin ICE negotiation in the other icedemo instance\n"
			"   - ICE negotiation will run, and result will be printed to screen\n"
			"   - send application data to remote [menu \"x\"]\n"
			"   - end/stop ICE session [menu \"e\"]\n"
			" destroy instance [menu \"d\"]\n"
			"");

	puts("");
	puts("This concludes the help screen.");
	puts("");
}

/*
 * Display console menu
 */
static void icedemo_print_menu(void)
{
	puts("");
	puts("+----------------------------------------------------------------------+");
	puts("|                    M E N U                                           |");
	puts("+---+------------------------------------------------------------------+");
	puts("| c | create           Create the instance                             |");
	puts("| d | destroy          Destroy the instance                            |");
	puts("| i | init o|a         Initialize ICE session as offerer or answerer   |");
	puts("| e | stop             End/stop ICE session                            |");
	puts("| s | show             Display local ICE info                          |");
	puts("| r | remote           Input remote ICE info                           |");
	puts("| b | start            Begin ICE negotiation                           |");
	puts("| x | send <compid> .. Send data to remote                             |");
	puts("+---+------------------------------------------------------------------+");
	puts("| h |  help            * Help! *                                       |");
	puts("| q |  quit            Quit                                            |");
	puts("+----------------------------------------------------------------------+");
}


static int tp_pjice_trx_ice(void* p2p)
{
	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *)p2p;
	int recvLen = 0;
	struct timeval timeout;
	fd_set wrset;
	fd_set reset;

	p2p_msg_t p2p_msg;
	memset(&p2p_msg, 0, sizeof(p2p_msg_t));
	p2p_msg.head.type = MSG_ASSIST_HOLE;
	p2p_msg.head.ack = 0;
	strcpy(p2p_msg.head.data, sdp64);
	memcpy(&p2p_msg.msg_data, &assist_hole, sizeof(assist_hole));

	int cnt = 2;
	while (!p2p_param->exit_flag)
	{
		cnt --;
		if(cnt == 0)
			break;
		FD_ZERO(&reset);
		FD_SET((unsigned int) server_sock, &wrset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0; 	//5000 = 5ms
		if (select(server_sock + 1, NULL, &wrset, NULL, &timeout) <= 0)
			continue;
		sendto(server_sock, (char *)&p2p_msg, sizeof(p2p_msg), 0,
					(struct sockaddr*)&server_addr, sizeof(server_addr)); //add by lk
		break;
	}

	struct sockaddr_in remote_addr;
	socklen_t sinLen = sizeof(remote_addr);

	memset(&p2p_msg, 0, sizeof(p2p_msg_t));
	memset(sdp64, 0, REMOTE_SDP_LEN);

	LOGI(">> recving from server_sock...");

	cnt = 4;
	while (!p2p_param->exit_flag)
	{
		cnt --;
		if(cnt < 0)
			break;
		FD_ZERO(&reset);
		FD_SET((unsigned int) server_sock, &reset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0; 	//5000 = 5ms
		if (select(server_sock + 1, &reset, NULL, NULL, &timeout) <= 0)
			continue;
		recvLen = recvfrom(server_sock, (char *)&p2p_msg, sizeof(p2p_msg_t), 0,
				(struct sockaddr *)&remote_addr, &sinLen);
		if(recvLen == sizeof(p2p_msg_t))
			break;
	}

	if(recvLen > 0 &&
			p2p_msg.head.type == MSG_ASSIST_HOLE_ACK &&
			p2p_msg.head.ack == 1){
		strcpy(sdp64, p2p_msg.head.data);
		LOGI("Exit---->tp_pjice_trx_ice()");
		return 0;
	}
	else{
		p2p_msg.head.type = MSG_ASSIST_HOLE_CANCEL;
		p2p_msg.head.ack = 0;
		memcpy(&p2p_msg.msg_data, &assist_hole, sizeof(assist_hole));
		sendto(server_sock, (char *)&p2p_msg, sizeof(p2p_msg), 0,
							(struct sockaddr*)&server_addr, sizeof(server_addr));
		LOGE("Exit---->tp_pjice_trx_ice()");
		return -1;
	}
}



/*
 * Main console loop.
 * Modify By LinZh107 at 150301
 */
static int icedemo_console(void *p2p)
{
	int iRet = -1;

	// waite for ice_finish signal
	struct timeval now;
	struct timespec timeout;

	/* ************************ */
	icedemo_create_instance();
#if 0
	int cnt = 3000;		//	wait for 1000 * 5000 usec
	int state_flags = PJ_ICE_STRANS_STATE_INIT;

	LOGI("checking icest state..");
	while(!p2p_param->exit_flag && cnt != 0)
	{
		state_flags = pj_ice_strans_get_state(icedemo->icest);
		if(PJ_ICE_STRANS_STATE_READY == state_flags)
			break;
		else
		{
			cnt --;
			usleep(1000);
		}
	}
	if(PJ_ICE_STRANS_STATE_READY != state_flags)
	{
		LOGE("error creating ice Failed!");
		return -1;
	}

#else
	if(PJ_ICE_STRANS_STATE_READY != pj_ice_strans_get_state(icedemo->icest))
	{
		gettimeofday(&now, NULL);
		timeout.tv_sec = now.tv_sec + 3;
		timeout.tv_nsec = now.tv_usec * 1000;
		/* ************************ */
		icedemo_create_instance();

		pthread_cleanup_push((pthread_cleanup_push_routine_t)pthread_mutex_unlock, (void*)&nego_mutex);
		pthread_mutex_lock(&nego_mutex);
		iRet =  pthread_cond_timedwait(&nego_cond, &nego_mutex, &timeout);
		pthread_mutex_unlock(&nego_mutex);
		pthread_cleanup_pop(0);
		if(0 != iRet )
		{
			LOGE("error creating ice Failed!");
			return -1;
		}
	}
#endif

	/* ************************ */
	icedemo_init_session('o');

	/* ************************ */
	icedemo_show_ice();

	// send iceinfo to remote and get remote ice back.
	int ret = tp_pjice_trx_ice(p2p);
	if(ret != 0)
		return -1;

	/* ************************ */
	icedemo_input_remote();

	// *** Add by LinZh107 ***
	// The following code couldn't remove !!!
	if (icedemo->rem.cand_cnt != 0)
	{
		unsigned i;
		LOGI("Remote ufrag       : %s\n", icedemo->rem.ufrag);
		LOGI("Remote password    : %s\n", icedemo->rem.pwd);
		LOGI("Remote cand. cnt.  : %d\n", icedemo->rem.cand_cnt);

		for (i = 0; i < icedemo->rem.cand_cnt; ++i)
		{
			char buffer[256] = {0};
			int len = print_cand(buffer, sizeof(buffer), &icedemo->rem.cand[i]);
			if (len < 0)
				err_exit("not enough buffer to show ICE status", -len);
			LOGI("  %s", buffer);
		}
	}

	/* ************************ */
	icedemo_start_nego();
#if 0
	usleep(100*1000);
	if(icedemo->icest)
	{
		state_flags = PJ_ICE_STRANS_STATE_NEGO;
		cnt = 7000;
		while(!p2p_param->exit_flag &&
				PJ_ICE_STRANS_STATE_NEGO == state_flags &&
				cnt != 0)
		{
			usleep(1000);
			if(!icedemo->icest || !pj_ice_strans_has_sess(icedemo->icest))
				break;
			if(pj_ice_strans_sess_is_complete(icedemo->icest))
				state_flags = pj_ice_strans_get_state(icedemo->icest);
			cnt --;
		}
		if(PJ_ICE_STRANS_STATE_RUNNING != state_flags)
		{
			LOGE("Error: start nego Failed!");
			return -1;
		}
		LOGI("After Check start nego!");
		return 0;
	}
	else
		return -1;

#else
	if(PJ_ICE_STRANS_STATE_RUNNING != pj_ice_strans_get_state(icedemo->icest))
	{
		gettimeofday(&now, NULL);
		timeout.tv_sec = now.tv_sec + 7;
		timeout.tv_nsec = now.tv_usec * 1000;

		pthread_cleanup_push((pthread_cleanup_push_routine_t)pthread_mutex_unlock, (void*)&nego_mutex);
		pthread_mutex_lock(&nego_mutex);
		iRet =  pthread_cond_timedwait(&nego_cond, &nego_mutex, &timeout);
		pthread_mutex_unlock(&nego_mutex);
		pthread_cleanup_pop(0);
		if(0 != iRet)
		{
			LOGE("error start nego Failed!");
			return -1;
		}
	}
#endif

    return 0;
}// end icedemo_console()

/*
 * Display program usage.
 */
static void icedemo_usage()
{
	puts("Usage: icedemo [optons]");
	printf("icedemo v%s by pjsip.org\n", pj_get_version());
	puts("");
	puts("General options:");
	puts(" --comp-cnt, -c N          Component count (default=1)");
	puts(" --nameserver, -n IP       Configure nameserver to activate DNS SRV");
	puts("                           resolution");
	puts(" --max-host, -H N          Set max number of host candidates to N");
	puts(" --regular, -R             Use regular nomination (default aggressive)");
	puts(" --log-file, -L FILE       Save output to log FILE");
	puts(" --help, -h                Display this screen.");
	puts("");
	puts("STUN related options:");
	puts(" --stun-srv, -s HOSTDOM    Enable srflx candidate by resolving to STUN server.");
	puts("                           HOSTDOM may be a \"host_or_ip[:port]\" or a domain");
	puts("                           name if DNS SRV resolution is used.");
	puts("");
	puts("TURN related options:");
	puts(" --turn-srv, -t HOSTDOM    Enable relayed candidate by using this TURN server.");
	puts("                           HOSTDOM may be a \"host_or_ip[:port]\" or a domain");
	puts("                           name if DNS SRV resolution is used.");
	puts(" --turn-tcp, -T            Use TCP to connect to TURN server");
	puts(" --turn-username, -u UID   Set TURN username of the credential to UID");
	puts(" --turn-password, -p PWD   Set password of the credential to WPWD");
	puts(" --turn-fingerprint, -F    Use fingerprint for outgoing TURN requests");
	puts("");
}

/******************************
 * Time out for connect()
 * Write by LinZh
 ******************************
static int connect_nonb(int udt_sock, const struct sockaddr *cu_addr, int add_len, int sec)
{
	int iRet;
	int flags, error = -1, intLen;
	intLen = sizeof(int);

	flags = fcntl(udt_sock, F_GETFL, 0);
	fcntl(udt_sock, F_SETFL, flags | O_NONBLOCK);//设置非阻塞模式

	iRet = udtc_connect(udt_sock, cu_addr, add_len);
	if (iRet == -1)
	{
		struct timeval tval;
		tval.tv_sec = sec;
		tval.tv_usec = 0;
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(udt_sock, &wset);
		if(select(udt_sock+1, NULL, &wset, NULL, &tval) > 0)
		{
			LOGI(">>>> udtc_connect error");
			udtc_getsockopt(udt_sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *) &intLen);
			if (error == 0)
				iRet = 0;
			else
			{
				LOGE("udtc_getsockopt error.");
				iRet = -1;
			}
		}
		else
		{
			LOGE("select udt_sock error.");
			iRet = -1;
		}
	}
	else
		LOGI("udtc_connect return iRet = %d",iRet);

	fcntl(udt_sock, F_SETFL, flags);
	return iRet;
}
*/


/*
 * Add by LinZh107 at 2015-03-09
 * do udt_bind and connect after we get the udp_sock.
 */
static int tp_pjice_udtc_connect(void* p2p)
{
	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *)p2p;

	LOGI("in tp_pjice_udtc_connect.");
	int iRet = -1;
	pj_ice_strans *ice_st = icedemo->icest;
	if(!ice_st || !pj_ice_strans_has_sess(icedemo->icest))
	{
		LOGE("ICE_Session has been destroyed.");
		return -1;
	}
	int comp_id = 1;
	if(!ice_st->comp[comp_id-1])
		return -1;
	else if(!ice_st->comp[comp_id-1]->stun_sock)
		return -1;
	else if(!ice_st->comp[comp_id-1]->stun_sock->active_sock)
		return -1;

	pj_ioqueue_key_t *key =  ice_st->comp[comp_id-1]->stun_sock->active_sock->key;
	if(!key)
		return -1;

	pj_ice_sess *ice_se = ice_st->ice;
	pj_ice_sess_comp *comp = &ice_se->comp[comp_id-1];

	struct sockaddr_in remote_addr;
	int add_len = (AF_INET6 == comp->valid_check->rcand->addr.addr.sa_family)?
			sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
	memcpy(&remote_addr, &comp->valid_check->rcand->addr, add_len);
	LOGI("remote_addr = %s:%d ",inet_ntoa(remote_addr.sin_addr),
			ntohs(remote_addr.sin_port));

	printf("The ActiveSocket key->fd = %d \n",key->fd);
	key->connecting = 107;
	icedemo_stop_session();
	usleep(1000);
	icedemo_destroy_instance();

	int udt_sock = udtc_socket(AF_INET, SOCK_STREAM, 0);
	if (udt_sock < 0)
	{
		LOGE("Create UDT udt_sock fail");
		return -2;
	}

	//***** get/set socket opction ***** LinZh107
	int szOptval = 1;
	int rcv_buf = 64000;	//16000
	int snd_buf = 64000;	//16000
	int sndTimeout = 1000;	//据说Linux 的sock编程可通过设置SNDTIMEO 来设置connect 超时
	int rcvTimeout = 5000;	//5 sec
	if(	(-1 == 	fcntl(key->fd, F_SETFL, 0)) //设置阻塞模式
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_REUSEADDR, (char*) &szOptval,sizeof(int)))
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_RENDEZVOUS, (char *) &szOptval, sizeof(int)))
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_SNDBUF, (char*) &snd_buf, sizeof(int)))
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_RCVBUF, (char*) &rcv_buf, sizeof(int)))
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_SNDTIMEO, (char*) &sndTimeout, sizeof(int)))
		|| (-1 == udtc_setsockopt(udt_sock, 0, UDT_RCVTIMEO, (char*) &rcvTimeout, sizeof(int))) )
	{
		udtc_close(udt_sock);
		close(key->fd);
		LOGE("udtc_setsockopt fail");
		return -2;
	}
	//************* end **************** LinZh107

	if (udtc_bind2(udt_sock, key->fd) == -1)
	{
		LOGE("udtc_bind2 failed");
		udtc_close(udt_sock);
		close(key->fd);
		return -2;
	}

	// if the upper layer set exit_flag, return immediately.
	if(p2p_param->exit_flag)
	{
		LOGE("udt bind succeed but upper layer request exit.");
		udtc_close(udt_sock);
		close(key->fd);
		return -2;
	}

	LOGI(">>>> udtc_connect begin");
	iRet = /*-1;//*/udtc_connect(udt_sock, (struct sockaddr *) &remote_addr, add_len);
	if (iRet == -1)
	{
		LOGE(">>>> udtc_connect failed");
		udtc_close(udt_sock);
		close(key->fd);
		return -2;
	}

	p2p_msg_t p2p_msg;
	memset(&p2p_msg, 0, sizeof(p2p_msg_t));
	int recvLen;
	recvLen = udtc_recvn(udt_sock, (char *)&p2p_msg, sizeof(p2p_msg));
	if(recvLen == sizeof(p2p_msg))
	{
		p2p_msg.head.type = MSG_HOLE_TEST_ACK;
		p2p_msg.head.ack = 1;
		int sendLen = udtc_sendn(udt_sock, (char *)&p2p_msg, sizeof(p2p_msg));
		if(sendLen == sizeof(p2p_msg))
		{
			p2p_param->udt_sock = udt_sock;
			LOGI(">>>> udtc_connect succeed!");
			return 0;
		}
	}

	udtc_close(udt_sock);
	LOGE(">>>> udtc_send failed.");
	return -2;
}

#if 0
static int tp_pjice_trx_stop(void* p2p)
{
	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *)p2p;
	LOGI(">> recving stop_sess request ...");
	int recvLen = 0;

	p2p_msg_t p2p_msg;
	memset(&p2p_msg, 0, sizeof(p2p_msg_t));

	struct sockaddr_in remote_addr;
	socklen_t sinLen = sizeof(remote_addr);

	struct timeval timeout;
	fd_set wrset;
	fd_set reset;

	int cnt = 5;
	while (!p2p_param->exit_flag)
	{
		cnt --;
		if(cnt < 0)
			break;
		FD_ZERO(&reset);
		FD_SET((unsigned int) server_sock, &reset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0; 	//5000 = 5ms
		if (select(server_sock + 1, &reset, NULL, NULL, &timeout) <= 0)
			continue;
		recvLen = recvfrom(server_sock, (char *)&p2p_msg, sizeof(p2p_msg_t), 0,
				(struct sockaddr *)&remote_addr, &sinLen);
		break;
	}

	if(recvLen <= 0 ||
			p2p_msg.head.type != MSG_HOLE_SUCCEED ||
			p2p_msg.head.ack != 0)
	{
		LOGE(">> tp_pjice_trx_stop() failed.");
		return -1;
	}

	p2p_msg.head.type = MSG_HOLE_SUCCEED_ACK;
	p2p_msg.head.ack = 1;
	cnt = 2;
	while (!p2p_param->exit_flag)
	{
		cnt --;
		if(cnt == 0)
			break;
		FD_ZERO(&reset);
		FD_SET((unsigned int) server_sock, &wrset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0; 	//5000 = 5ms
		if (select(server_sock + 1, NULL, &wrset, NULL, &timeout) <= 0)
			continue;
		sendto(server_sock, (char *)&p2p_msg, sizeof(p2p_msg), 0,
					(struct sockaddr*)&server_addr, sizeof(server_addr)); //add by LinZh107
		break;
	}
	LOGI(">> tp_pjice_trx_stop() ok.");
	return 0;
}
#endif

static int tp_pjice_init(void *p2p)
{
	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *)p2p;
	server_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_sock <= 0)
		return -1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("183.63.52.49");
	server_addr.sin_port = htons(SERVER_PORT);

	int flags = fcntl(server_sock, F_GETFL, 0);
	fcntl(server_sock, F_SETFL, flags|O_NONBLOCK);

	int sock_flag = 1;
	if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &sock_flag, sizeof(sock_flag)) < 0)
	{
		LOGE("tp_pjice_send_ice setsockopt Error.");
		return -1;
	}

	memset(&assist_hole, 0, sizeof(assist_hole));
	strcpy(assist_hole.pu_id, p2p_param->puid);

	return 0;
}

/*
 * And here's the main()
 */
//int main(int argc, char *argv[])
int tp_pjice_main(void *p2p)
{
	// init mutexattr &mutex &cond
	pthread_mutexattr_init(&mutexattr);
	pthread_mutex_init(&ice_mutex, &mutexattr);
    pthread_mutex_init(&nego_mutex, &mutexattr);
	pthread_cond_init(&ice_cond, NULL);
    pthread_cond_init(&nego_cond, NULL);

	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *)p2p;
	int ret;
	icedemo = (g_app_t *)malloc(sizeof(g_app_t));
	memset(icedemo, 0, sizeof(g_app_t));

    pj_status_t status = 0;

    icedemo->opt.comp_cnt = 1;
    icedemo->opt.max_host = -1;

    icedemo->opt.stun_srv.ptr = "183.63.52.49"; //*/"stun.ekiga.net";
    icedemo->opt.stun_srv.slen = strlen(icedemo->opt.stun_srv.ptr);
	LOGI("icedemo->opt.stun_srv = %s ",icedemo->opt.stun_srv.ptr);

	status = icedemo_init();
	if (status != PJ_SUCCESS)
	{
		LOGE("icedemo_init() failed.");
		free(icedemo);
		icedemo = NULL;
		return -1;
	}

	/*
	 * do the server_connect init.
	 */
	ret = tp_pjice_init(p2p);
	if(0 != ret)
	{
		LOGE("Init tp_pjice() failed.");
		ret = -2;
		goto main_out;
	}

	/*
	 * do the icedemo traversal.
	 */
	ret = icedemo_console(p2p);
	if(0 != ret)
	{
		usleep(10*1000);
		goto main_out;
	}

	// waite for remote ice_finish signal
	struct timeval now;
	struct timespec timeout;
	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + 3;
	timeout.tv_nsec = now.tv_usec * 1000;

	pthread_cleanup_push((pthread_cleanup_push_routine_t)pthread_mutex_unlock, (void*)&ice_mutex);
	pthread_mutex_lock(&ice_mutex);
	ret =  pthread_cond_timedwait(&ice_cond, &ice_mutex, &timeout);
	pthread_mutex_unlock(&ice_mutex);
	pthread_cleanup_pop(0);

	/*
     * do the udt_connect.
     */
    if(0 != ret)
        LOGE("recv STOP_SESS error.");
	else
	{
        icedemo_send_data(1, "stop_sess");
		ret = tp_pjice_udtc_connect(p2p);
	}


// return without doing nothing.
main_out:
	if(ret != 0 && ret != -2)
	{
	    icedemo_stop_session();
	    usleep(1000);
	    icedemo_destroy_instance();
	}

	close(server_sock);
	server_sock = -1;

	err_exit("Quitting..", PJ_SUCCESS);

	free(icedemo);
	p2p_param->exit_flag = 1;

	//destroy cond & mutex
    pthread_cond_destroy(&nego_cond);
	pthread_cond_destroy(&ice_cond);
	pthread_mutex_destroy(&nego_mutex);
	pthread_mutex_destroy(&ice_mutex);
	pthread_mutexattr_destroy(&mutexattr);

	return 0;
}
