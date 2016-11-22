/***************************************************************************
 *
 * Copyright    : 2007 santachi corp.
 * File name    : clientNetLib.c
 * Description  : implement all interface of clientNetLib
 * Created      : zhangkk --- 2007-03-06
 * Modified     : liuqs<liuqs_santachi@126.com> --- 2007-03-26
 *					add com param by liuqs on 2007-4-11
 *					add transparent cmd by liuqs on 2007-4-11
 *
 ***************************************************************************/
#include "clientNetLib_include.h"

#include <android/log.h>
#define  LOG_TAG "clientNetLib.c"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define printf(arg...)

char remote_icedata[512] = { 0 };
int remote_icelen = 0;

static device_info_t *g_device_info = NULL;
static REGISTER_CALLBACK g_register_callback = NULL;
static pthread_mutex_t hDeviceMutex;

//2014-4-18 LinZh107
static pthread_mutex_t alarmInfoMutex;
static pthread_mutex_t LogoutMutex;
static pthread_mutex_t CloseChannelMutex;

static msg_fragment_t *record_info = NULL;
static msg_file_down_Status *msg_progress = NULL;

static int isfull = 0;
static int stop = 0;
static int stopdownload = 0;


#define DATA_BUF_LEN (512 * 1024) //link to  dev_type.hint g_protocol = PROTOCOL_TCP;  //全局变量，用以判断链接协议!!!!!不能设置静态变量!!!!
typedef void (*pthread_cleanup_push_routine_t)(void*);

static void *tp_p2p_cross_thread(void *p2p) {
	pthread_detach(pthread_self());

	BOOL ret = tp_pjice_main(p2p);

	p2p_thorough_param_t *p2p_param = (p2p_thorough_param_t *) p2p;
	free(p2p_param);

	LOGI("Exit---->P2pThread %d", ret);
	return NULL;
}

// add by LinZh107 at 2015-3-18 15:04:04
// return p2p_sock if succeed.
static int tp_pjice_getsock(unsigned long user_id) {
	LOGI("Enter---->P2pThread");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;

	p2p_thorough_param_t *p2p_param = malloc(sizeof(p2p_thorough_param_t));
	memset(p2p_param, 0, sizeof(p2p_thorough_param_t));
	strcpy(p2p_param->puid, pServerInfo->pu_id);
	p2p_param->udt_sock = 0;
	p2p_param->exit_flag = 0;

	pthread_create(&pServerInfo->p2p_thread_id, NULL, tp_p2p_cross_thread,
			p2p_param);
	usleep(500 * 1000);
	int cnt = 1500; //15sec
	while (cnt > 0) {
		if (p2p_param->exit_flag)
			break;
		usleep(10000);
		cnt--;
	}
	p2p_param->exit_flag = 1;
	return p2p_param->udt_sock;
}

#if 0
static int my_log_fd;

int init_my_log(char *filename)
{
	return (my_log_fd=open(filename, O_CREAT|O_RDWR|O_TRUNC , 0777));
}

void my_log(const char *format, ...)
{
	char buf[512];
	va_list ap;

	if(my_log_fd <= 0)
	return;

	va_start(ap, format);
	vsprintf(buf, format, ap);
	va_end(ap);
	write(my_log_fd, buf, strlen(buf));
}
#endif

static ssize_t writen(int fd, const void *vptr, size_t n) {
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0; /* and call write() again */
			else
				return (-1); /* error */
		}
		if (errno == 5)
			return (-1);
		nleft -= nwritten;
		ptr += nwritten;
	}

	return (n);
}

static ssize_t readn(int fd, void *vptr, size_t n) {
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0; /* and call read() again */
			else
				return (-1);
		} else if (nread == 0) {
			break; /* EOF */
		}
		nleft -= nread;
		ptr += nread;
	}

	return (n - nleft); /* return >= 0 */
}

static int st_comm_semInit(sem_t *sem, int value) {
	return (sem_init(sem, 0, value));
}

static int st_comm_semDestroy(sem_t *sem) {
	return (sem_destroy(sem));
}

static int st_comm_semBPost(sem_t *sem) {
	int ret;
	int val;

	ret = sem_getvalue(sem, &val);
	if (val == 0)
		ret = sem_post(sem);

	return ret;
}

static int st_netcodec_semTrywait(sem_t *sem_id, int timeout) // 5sec = 5000ms
{
	int count = timeout / 20;
	int iRet;
	while (1) {
		iRet = sem_trywait(sem_id);
		if (iRet == 0) {
			return 0;
		} else if (iRet == EAGAIN) {
			LOGE("st_netcodec_semTrywait count %d", count);
			if (count == 0)
				return -1;
			count--;
			usleep(20 * 1000);
			continue;
		} else {
			if (count == 0)
				return -1;
			count--;
			usleep(20 * 1000);
			continue;
		}
	}
	return -1;
}

static int connect_nonb(int sockfd, const struct sockaddr *saptr,
		socklen_t salen, int msec) {
	int flags, n, error;
	socklen_t len;

	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	error = 0;
	if ((n = connect(sockfd, saptr, salen)) < 0) {
		if (errno != EINPROGRESS) {
			LOGE("connect_nonb error! return -1");
			return (-1);
		}
		//LOGI("try connect return %d!", n);
	}
	if (n == 0)
		goto done;

	// ******************** LinZh107 2014-12-26 ***********************************
	// connect() return non zero but EINPROGRESS is zero.
	// so maybe connect() was succeed.
	// below is the testing whether the socket connected or not.
	struct timeval tval;
	fd_set rset, wset;
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = msec / 1000;
	if (msec > 1000)
		tval.tv_usec = msec;
	else
		tval.tv_usec = msec * 1000;
	if (select(sockfd + 1, &rset, &wset, NULL, msec ? &tval : NULL) == 0) {
		close(sockfd);
		LOGE("connect nonb connect time out !");
		errno = ETIMEDOUT;
		return (-2);
	}
	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			LOGE("connect_nonb error! return -3");
			return (-3);
		}
	} else
		LOGE("FD_ISSET error: sockfd net set");

	done: if (fcntl(sockfd, F_SETFL, 0) < 0)
		LOGE(">>> fcntl change sock to block, flags = %d", flags);
	else {
		flags = fcntl(sockfd, F_GETFL, 0);
//		LOGI(">>> fcntl change sock to block, flags = %d", flags);
	}
	if (error) {
		close(sockfd);
		errno = error;
		LOGE("connect_nonb error! return -4");
		return (-4);
	}
	//LOGI("out connect_nonb return 0!");
	return (0);
}

static int st_comm_getIpAddr(char *ipaddr, size_t len) {
	int fd;
	char buffer[50];
	struct ifreq ifr;
	struct sockaddr_in *addr;

	if (PROTOCOL_TCP == g_protocol)
		fd = socket(AF_INET, SOCK_STREAM, 0);
	else
		fd = udtc_socket(AF_INET, SOCK_STREAM, 0);

	if (fd >= 0) {
		LOGI("tcp/udtc_socket=%d", fd);
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, COMM_ETHNAME, IFNAMSIZ);
		//strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name) - 1);
		//ifr.ifr_name[IFNAMSIZ - 1] = '\0';
		if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
			LOGI("ioctl(fd, SIOCGIFADDR, &ifr)==0=%d", fd);
			addr = (struct sockaddr_in *) &(ifr.ifr_addr);
			inet_ntop(AF_INET, &addr->sin_addr, buffer, 20);
		} else {
			LOGI("ioctl()!= 0/udtc_socket=%d", fd);
			if (PROTOCOL_TCP == g_protocol)
				close(fd);
			else
				udtc_close(fd);
			return (-1);
		}
	} else {
		return (-1);
	}

	if (strlen(buffer) > len - 1) {
		return (-1);
	}
	strncpy(ipaddr, buffer, len);
	if (PROTOCOL_TCP == g_protocol)
		close(fd);
	else
		udtc_close(fd);
	LOGI("----tcp/udtc close sock----");
	return (0);
}

static int addTcpSockInfo(server_connect_t *pConnect,
		tcp_sock_info_t *tcp_sock_info) {
	tcp_sock_info_t *loop;
	pthread_mutex_lock(&pConnect->tcpmutex_id);

	if (pConnect->tcp_sock_info == NULL)
		pConnect->tcp_sock_info = tcp_sock_info;
	else {
		loop = pConnect->tcp_sock_info;
		while (loop->next != NULL)
			loop = loop->next;
		loop->next = tcp_sock_info;
	}
	pthread_mutex_unlock(&pConnect->tcpmutex_id);
	return 0;
}

#if 1
static int delTcpSockInfo(server_connect_t *pConnect,
		tcp_sock_info_t *tcp_sock_info) {
	tcp_sock_info_t *loop = pConnect->tcp_sock_info;
	pthread_mutex_lock(&pConnect->tcpmutex_id);

	if (pConnect->tcp_sock_info == NULL || tcp_sock_info == NULL) {
		pthread_mutex_unlock(&pConnect->tcpmutex_id);
		return -1;
	} else if (loop == tcp_sock_info) {
		pConnect->tcp_sock_info = loop->next;
		free(tcp_sock_info);
		pthread_mutex_unlock(&pConnect->tcpmutex_id);
		return 0;
	} else {
		while (loop->next) {
			if (loop->next == tcp_sock_info) {
				loop->next = loop->next->next;
				free(tcp_sock_info);
				pthread_mutex_unlock(&pConnect->tcpmutex_id);
				return 0;
			}
			loop = loop->next;
		}
	}
	pthread_mutex_unlock(&pConnect->tcpmutex_id);
	return 0;
}

static int InsertMsg(server_connect_t *pConnect, int index, net_msg_t net_msg) {
	if (pConnect == NULL)
		return -1;
	pConnect->p_net_msg[index] = (net_msg_t*) malloc(sizeof(net_msg_t));
	memcpy(pConnect->p_net_msg[index], &net_msg, sizeof(net_msg_t));
	return 0;
}
#endif

static int GetMsg(server_connect_t *pConnect, int index, net_msg_t *p_net_msg) {
//	LOGI("GetMsg index = %d",index);
	if (pConnect == NULL)
		return -1;
	memcpy(p_net_msg, pConnect->p_net_msg[index], sizeof(net_msg_t));
	if (pConnect->p_net_msg[index] != NULL)
		free(pConnect->p_net_msg[index]);
	pConnect->p_net_msg[index] = NULL;
	return 0;
}

static int SendNetMsg(int msg_sock, int msg_type, int msg_subtype, char *buf,
		int datasize) {
	net_msg_t net_msg;
	net_msg.msg_head.msg_type = msg_type;
	net_msg.msg_head.msg_subtype = msg_subtype;
	net_msg.msg_head.msg_size = datasize;
	int sendLen = 0;

	if (datasize > sizeof(net_msg.msg_data)) {
		LOGE("SendNetMsg====> datasize too len ");
		return -1;
	}
	memcpy(net_msg.msg_data, (char*) buf, datasize);
	if (msg_sock < 0) {
		LOGE("SendNetMsg====> msg_sock error");
		return -1;
	}
	//LOGI("---->SendNetMsg %d to remote begin",msg_type);
	if (PROTOCOL_TCP == g_protocol)
		sendLen = writen(msg_sock, (char*) &net_msg,
				sizeof(msg_head_t) + datasize);
	else
		sendLen = udtc_sendn(msg_sock, (char*) &net_msg,
				sizeof(msg_head_t) + datasize);
	if (sendLen != (int) (sizeof(msg_head_t) + datasize)) {
		LOGE("err1  tcp/udtc_sendn error");
		return -1;
	}
	return 0;
}

static int SetServInfo(unsigned long user_id, int msg_type, int msg_subtype,
		int valid_index, void *pData, int datasize) {
//	LOGI("---->SetServInfo!");
	net_msg_t net_msg;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pServerInfo->server_connect == NULL)
		return (-1);
	pConnect->interval_valid[valid_index] = TRUE;

	SendNetMsg(pConnect->msg_sock, msg_type, msg_subtype, (char*) pData,
			datasize);
	if (st_netcodec_semTrywait(&pConnect->sem_id[valid_index],
			3000/*OVER_TIME/4*/) == 0) {
		if (GetMsg(pConnect, valid_index, &net_msg) < 0)
			return -1;
		if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
	} else {
		LOGE("semTrywait index:%d error", valid_index);
		pConnect->interval_valid[valid_index] = FALSE;
		return -1;
	}
	return 0;
}

static int GetServInfo(unsigned long user_id, int msg_type, int msg_subtype,
		int valid_index, void *pData, int datasize) {
	//LOGI("--- in  GetServInfo %d",msg_subtype);
	net_msg_t net_msg;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pServerInfo->server_connect == NULL)
		return (-1);

	pConnect->interval_valid[valid_index] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, msg_type, msg_subtype, NULL, 0) < 0)
		return -1;
	if (st_netcodec_semTrywait(&pConnect->sem_id[valid_index], 3000) != 0) {
		pConnect->interval_valid[valid_index] = FALSE;
		LOGE("--- out  GetServInfo %d", msg_subtype);
		return -1;
	}
	if (GetMsg(pConnect, valid_index, &net_msg) < 0) {
		//LOGE("--- out  GetServInfo %d",msg_subtype);
		return -1;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		//LOGE("--- out  GetServInfo %d",msg_subtype);
		return -1;
	}
	memcpy((char*) pData, (char*) net_msg.msg_data, datasize);
	//LOGI("--- out  GetServInfo %d",msg_subtype);
	return 0;
}

#if 0
static void getmbroadip(char* source, char* target, int ichannel)
{
	int p1, p2, p3, p4;
	sscanf(source, "%d.%d.%d.%d", &p1, &p2, &p3, &p4);
	sprintf(target, "%d.%d.%d.%d", (p1 + p2 + p3 + p4) % 16 + 224, (p1 + p2 + p3) % 256, (p1 + p2 + p4) % 256,
			ichannel);
}
#endif

// 处理告警的回调函数  Data: 2014-4-1 Author: yms
//2014-4-15 LinZh107
static int alarmInfoCallback(msg_alarm_info_t msg_alarm_info, void *context) {
	pthread_mutex_lock(&alarmInfoMutex);

	static int nInput_Cnt = 0;
	nInput_Cnt = nInput_Cnt % MSG_ALARM_INFO_GROUP;

	memcpy(&(g_AlarmInfoGroup[nInput_Cnt]), &msg_alarm_info,
			sizeof(msg_alarm_info_t));
	nInput_Cnt++;

	pthread_mutex_unlock(&alarmInfoMutex);
	//LOGI(">>> Enter alarmInfoCallback %s", g_AlarmInfoGroup[nInput_Cnt].alarm_info);
	return 0;
}

static void *MsgThread(void *param) {
	LOGI(" ");
	LOGI("Create----->MsgThread!");
	pthread_detach(pthread_self());

	int recvLen = 0;
	int data_len = 0;
	int msg_type;
	int msg_subtype;
	net_msg_t net_msg;
	server_connect_t *pConnect = (server_connect_t *) param;

	pConnect->msg_flag = 1;

	fd_set rdfds;
	struct timeval timeout;

	int previewCnt = 300; //300*50*1000 = 15 S
	int cntTimeout = 50 * 1000;

	//************* get/set socket opction **************** LinZh107
	if (PROTOCOL_TCP != g_protocol) {
		int rcvTimeout = cntTimeout / 1000; //100 msec
		int iRet = udtc_setsockopt(pConnect->msg_sock, 0, UDT_RCVTIMEO,
				(char*) &rcvTimeout, sizeof(int));
		iRet = udtc_setsockopt(pConnect->msg_sock, 0, UDT_SNDTIMEO,
				(char*) &rcvTimeout, sizeof(int));
		LOGI("udtc_setsockopt return %d", iRet);
	}
	//************* end **************** LinZh107

	int cnt = previewCnt;
	while (pConnect->msg_flag) {
		recvLen = -1;
		memset(&net_msg, 0, sizeof(net_msg_t));

		//************************* receive msg header *********************
		//********************** TCP protocol ******************************
		if (PROTOCOL_TCP == g_protocol) {
			/*
			 * The code below was modify by LinZh107
			 * to improve the tcp-Linked preferenc on 2014-10-10.
			 */
			FD_ZERO(&rdfds);
			FD_SET((unsigned int)pConnect->msg_sock, &rdfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = cntTimeout;
			//modify 2014-10-28 LinZh107  改小时间(多次检测)，提高用户体验(退出可以立即响应)
			switch (select(pConnect->msg_sock + 1, &rdfds, NULL, NULL, &timeout)) {
			case -1:
			case 0: //select错误 或者超时
				//LOGI("Msg Select Head timeout! Msg = %s", strerror(errno));
				cnt--;
				if (cnt == 0) {
					pConnect->bReloginFlag = TRUE;
					//here will exit the thread
					goto Exit;
				}
				continue;
			default:
				if (FD_ISSET(pConnect->msg_sock, &rdfds)) //测试sock是否可读，即是否网络上有数据
					recvLen = readn(pConnect->msg_sock, (char*) &net_msg,
							sizeof(msg_head_t));
				break;
			} // end switch
		}
		//  UDT/P2P protocol
		else
			recvLen = udtc_recvn(pConnect->msg_sock, (char*) &net_msg,
					sizeof(msg_head_t));

		if (sizeof(msg_head_t) != recvLen) {
			cnt--;
			if (cnt == 0) {
				pConnect->bReloginFlag = TRUE;
				goto Exit;
			}
			continue;
		}

		if ((data_len = net_msg.msg_head.msg_size) > 100 * 1024
				|| data_len < 0) {
			LOGE("MsgThread ====> read msg_head error2 = %d", data_len);
			continue;
		}
		//******************************************************************

		//*************************** receive msg body *********************
		//********************** TCP protocol ******************************
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&rdfds);
			FD_SET((unsigned int)pConnect->msg_sock, &rdfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = cntTimeout;
			switch (select(pConnect->msg_sock + 1, &rdfds, NULL, NULL, &timeout)) {
			case -1:
				LOGE("Msg Select msg_data error! Msg = %s", strerror(errno));
				pConnect->bReloginFlag = TRUE;
				goto Exit;
			case 0:
				//LOGI("Msg Select msg_data timeout! Msg = %s", strerror(errno));
				break; // can not be CONTINUE ,because the msg mabe don't have a body
			default:
				if (FD_ISSET(pConnect->msg_sock, &rdfds))
					recvLen = readn(pConnect->msg_sock,
							(char*) &net_msg + sizeof(msg_head_t), data_len);
				break;
			} // end switch
		}
		//********************** UDT/P2P protocol ******************************
		else
			recvLen = udtc_recvn(pConnect->msg_sock,
					(char*) &net_msg + sizeof(msg_head_t), data_len);
		//*************************** end receive msg *********************

		if (recvLen != data_len && sizeof(msg_head_t) != recvLen) {
			cnt--;
			if (cnt == 0) {
				pConnect->bReloginFlag = TRUE;
				//here will exit the thread
				goto Exit;
			}
			continue;
		}

		// before the over_time coming there are data come in ,so recount again.
		cnt = previewCnt;

		msg_type = net_msg.msg_head.msg_type;
		msg_subtype = net_msg.msg_head.msg_subtype;

		/**
		 * analyze the received data
		 */
		switch (msg_type) {
		case MSG_HEARTBEAT: {
#if 0
			if (!pConnect->interval_valid[INDEX_MSG_HEARTBEAT])
			return NULL;
			InsertMsg(pConnect,INDEX_MSG_HEARTBEAT,net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_HEARTBEAT]);
#endif
			//LOGI(">>>> MsgThread MSG_HEARTBEAT ACK");
		}
			break;
		case MSG_LOGIN: {
			if (!pConnect->interval_valid[INDEX_MSG_LOGIN])
				return NULL;
			InsertMsg(pConnect, INDEX_MSG_LOGIN, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_LOGIN]);
			//LOGI(">>>> MsgThread MSG_LOGIN ACK");
		}
			break;
		case MSG_SESSION_ID: {
			if (!pConnect->interval_valid[INDEX_MSG_SESSION_ID])
				return NULL;
			InsertMsg(pConnect, INDEX_MSG_SESSION_ID, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_SESSION_ID]);
			//LOGI(">>>> MsgThread MSG_SESSION_ID ACK");
		}
			break;
		case MSG_LOGOUT: {
			LOGI(">>>> MsgThread MSG_LOGOUT ACK");
			//************ here will exit the thread ************
			goto Exit;
		}
		case MSG_OPEN_CHANNEL: {
			//LOGI(">>>> MsgThread MSG_OPEN_CHANNEL ACK");
			if (!pConnect->interval_valid[INDEX_MSG_OPEN_CHANNEL])
				break;
			InsertMsg(pConnect, INDEX_MSG_OPEN_CHANNEL, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_OPEN_CHANNEL]);
		}
			break;
		case MSG_CLOSE_CHANNEL: {
			//LOGI(">>>> MsgThread MSG_CLOSE_CHANNEL ACK");
		}
			break;
		case MSG_ALARM_INFO: {
			//LOGI(">>>> MsgThread Recv MSG_ALARM_INFO");
			msg_alarm_info_t msg_alarm_info;
			memcpy(&msg_alarm_info, net_msg.msg_data, sizeof(msg_alarm_info_t));
			if (pConnect->alarm_info_callback != NULL) {
				pConnect->alarm_info_callback(msg_alarm_info, NULL);
			}
		}
			break;
		case MSG_ALARM_CONTROL: {
			//LOGI(">>>> INDEX_MSG_ALARM_CONTROL");
			if (!pConnect->interval_valid[INDEX_MSG_ALARM_CONTROL])
				break;
			InsertMsg(pConnect, INDEX_MSG_ALARM_CONTROL, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_ALARM_CONTROL]);
		}
			break;
		case MSG_START_ALARM: {
			if (!pConnect->interval_valid[INDEX_MSG_START_ALARM])
				break;
			InsertMsg(pConnect, INDEX_MSG_START_ALARM, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_START_ALARM]);
		}
			break;
		case MSG_STOP_ALARM: {
			if (!pConnect->interval_valid[INDEX_MSG_STOP_ALARM])
				break;
			InsertMsg(pConnect, INDEX_MSG_STOP_ALARM, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_STOP_ALARM]);
		}
			break;
		case MSG_GET_LOG: {
			if (!pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_GET_LOG_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_GET_LOG_INFO]);
		}
			break;
		case MSG_DELETE_LOG: {
			if (!pConnect->interval_valid[INDEX_MSG_DELETE_LOG_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_DELETE_LOG_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_DELETE_LOG_INFO]);
		}
			break;
		case MSG_GET_TIME: {
			if (!pConnect->interval_valid[INDEX_MSG_GET_TIME_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_GET_TIME_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_GET_TIME_INFO]);
		}
			break;
		case MSG_SET_TIME: {
			if (!pConnect->interval_valid[INDEX_MSG_SET_TIME_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_SET_TIME_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_SET_TIME_INFO]);
		}
			break;
		case MSG_REMOTE_START_HAND_RECORD: {
			if (!pConnect->interval_valid[INDEX_MSG_REMOTE_START_HAND_RECORD])
				break;
			InsertMsg(pConnect, INDEX_MSG_REMOTE_START_HAND_RECORD, net_msg);
			st_comm_semBPost(
					&pConnect->sem_id[INDEX_MSG_REMOTE_START_HAND_RECORD]);
		}
			break;
		case MSG_REMOTE_STOP_HAND_RECORD: {
			if (!pConnect->interval_valid[INDEX_MSG_REMOTE_STOP_HAND_RECORD])
				break;
			InsertMsg(pConnect, INDEX_MSG_REMOTE_STOP_HAND_RECORD, net_msg);
			st_comm_semBPost(
					&pConnect->sem_id[INDEX_MSG_REMOTE_STOP_HAND_RECORD]);
		}
			break;
		case MSG_VERSION_INFO: {
			if (!pConnect->interval_valid[INDEX_MSG_VERSION_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_VERSION_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_VERSION_INFO]);
		}
			break;
		case MSG_HARDDISK_INFO: {
			if (!pConnect->interval_valid[INDEX_MSG_HARDDISK_INFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_HARDDISK_INFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_HARDDISK_INFO]);
		}
			break;
		case MSG_SYSTEM_STATUS: {
			if (!pConnect->interval_valid[INDEX_MSG_SYSTEM_STATUS])
				break;
			InsertMsg(pConnect, INDEX_MSG_SYSTEM_STATUS, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_SYSTEM_STATUS]);
		}
			break;
		case MSG_UPDATE_STATUS: {
			if (!pConnect->interval_valid[INDEX_MSG_UPGRADE_STATUS])
				break;
			InsertMsg(pConnect, INDEX_MSG_UPGRADE_STATUS, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_UPGRADE_STATUS]);
		}
			break;
#if 0
			case MSG_START_TALK:
			{
				if (!pConnect->interval_valid[INDEX_MSG_START_TALK])
				break;
				InsertMsg(pConnect,INDEX_MSG_START_TALK,net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_START_TALK]);
			}break;
#endif
		case MSG_GET_USERINFO: {
			if (!pConnect->interval_valid[INDEX_MSG_GET_USERINFO])
				break;
			InsertMsg(pConnect, INDEX_MSG_GET_USERINFO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_GET_USERINFO]);
		}
			break;
		case MSG_ADD_USER: {
			if (!pConnect->interval_valid[INDEX_MSG_ADD_USER])
				break;
			InsertMsg(pConnect, INDEX_MSG_ADD_USER, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_ADD_USER]);
		}
			break;
		case MSG_DELETE_USER: {
			if (!pConnect->interval_valid[INDEX_MSG_DELETE_USER])
				break;
			InsertMsg(pConnect, INDEX_MSG_DELETE_USER, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_DELETE_USER]);
		}
			break;
		case MSG_MODIFY_USER: {
			if (!pConnect->interval_valid[INDEX_MSG_CHANGE_USER])
				break;
			InsertMsg(pConnect, INDEX_MSG_CHANGE_USER, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_CHANGE_USER]);
		}
			break;
		case MSG_PTZ_CONTROL: {
			if (!pConnect->interval_valid[INDEX_MSG_PTZ_CONTROL])
				break;
			InsertMsg(pConnect, INDEX_MSG_PTZ_CONTROL, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PTZ_CONTROL]);
		}
			break;
		case MSG_MATRIX_PROTOCOL: {
			if (!pConnect->interval_valid[INDEX_MSG_PAPAM_MATRIX])
				break;
			InsertMsg(pConnect, INDEX_MSG_PAPAM_MATRIX, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PAPAM_MATRIX]);
		}
			break;
		case MSG_GET_RECORD_LIST: {
			if (!pConnect->interval_valid[INDEX_MSG_GET_RECORD_LIST])
				break;
			InsertMsg(pConnect, INDEX_MSG_GET_RECORD_LIST, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_GET_RECORD_LIST]);
		}
			break;
		case MSG_DELETE_RECORD_FILE: {
			if (!pConnect->interval_valid[INDEX_MSG_DELETE_RECORD_FILE])
				break;
			InsertMsg(pConnect, INDEX_MSG_DELETE_RECORD_FILE, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_DELETE_RECORD_FILE]);
		}
			break;
		case MSG_START_BACK: {
			if (!pConnect->interval_valid[INDEX_MSG_START_BACK])
				break;
			InsertMsg(pConnect, INDEX_MSG_START_BACK, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_START_BACK]);
		}
			break;
		case MSG_STOP_BACK: {
			if (!pConnect->interval_valid[INDEX_MSG_STOP_BACK])
				break;
			InsertMsg(pConnect, INDEX_MSG_STOP_BACK, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_STOP_BACK]);
		}
			break;
		case MSG_DEFAULT_SETTING: {
			if (!pConnect->interval_valid[INDEX_MSG_DEFAULT_SETTING])
				break;
			InsertMsg(pConnect, INDEX_MSG_DEFAULT_SETTING, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_DEFAULT_SETTING]);
		}
			break;
		case MSG_DEFAULTOUTPUT_SETING: {
			if (!pConnect->interval_valid[INDEX_MSG_PARAM_RECOVERDEF_CONFIG])
				break; //可能现在用的就是出厂参数.
			InsertMsg(pConnect, INDEX_MSG_PARAM_RECOVERDEF_CONFIG, net_msg);
			st_comm_semBPost(
					&pConnect->sem_id[INDEX_MSG_PARAM_RECOVERDEF_CONFIG]);
		}
			break;
		case MSG_RESET_VIDEO_PARAM: {
			if (!pConnect->interval_valid[INDEX_MSG_RESET_VIDIO])
				break;
			InsertMsg(pConnect, INDEX_MSG_RESET_VIDIO, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_RESET_VIDIO]);
		}
			break;
		case MSG_CCD_DEFAULT: {
			if (!pConnect->interval_valid[INDEX_MSG_PARAM_CCD_DEFAULT])
				break;
			InsertMsg(pConnect, INDEX_MSG_PARAM_CCD_DEFAULT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_CCD_DEFAULT]);
		}
			break;
		case MSG_ITU_DEFAULT: {
			if (!pConnect->interval_valid[INDEX_MSG_PARAM_ITU_DEFAULT])
				break;
			InsertMsg(pConnect, INDEX_MSG_PARAM_ITU_DEFAULT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_ITU_DEFAULT]);
		}
			break;
		case MSG_VERSION_SETING: {
			if (!pConnect->interval_valid[INDEX_MSG_SETVERSION])
				break;
			InsertMsg(pConnect, INDEX_MSG_SETVERSION, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_SETVERSION]);
		}
			break;
		case MSG_HAND_RESTART: {
			if (!pConnect->interval_valid[INDEX_MSG_HAND_RESTART])
				break; //是否可以更改为return NULL;
			InsertMsg(pConnect, INDEX_MSG_HAND_RESTART, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_HAND_RESTART]);
		}
			break;
		case MSG_PARTITION: {
			if (!pConnect->interval_valid[INDEX_MSG_PARTITION])
				break;
			InsertMsg(pConnect, INDEX_MSG_PARTITION, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARTITION]);
		}
			break;
		case MSG_FORMAT: {
			if (!pConnect->interval_valid[INDEX_MSG_FORMAT])
				break;
			InsertMsg(pConnect, INDEX_MSG_FORMAT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_FORMAT]);
		}
			break;
		case MSG_FORMAT_STATUS: {
			if (!pConnect->interval_valid[INDEX_MSG_FORMAT_STATUS])
				break;
			InsertMsg(pConnect, INDEX_MSG_FORMAT_STATUS, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_FORMAT_STATUS]);
		}
			break;
		case MSG_FDISK_STATUS: {
			if (!pConnect->interval_valid[INDEX_MSG_FDISK_STATUS])
				break;
			InsertMsg(pConnect, INDEX_MSG_FDISK_STATUS, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_FDISK_STATUS]);
		}
			break;
		case MSG_TRANSPARENT_CMD: {
			if (!pConnect->interval_valid[INDEX_MSG_TRANSPARENT])
				break;
			InsertMsg(pConnect, INDEX_MSG_TRANSPARENT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_TRANSPARENT]);
		}
			break;
		case MSG_GET_AP_INFO: {
			if (!pConnect->interval_valid[INDEX_MSG_WIRDLESS_SEARCH])
				break;
			InsertMsg(pConnect, INDEX_MSG_WIRDLESS_SEARCH, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_WIRDLESS_SEARCH]);
		}
			break;
		case MSG_START_TRANSPARENT: {
			if (!pConnect->interval_valid[INDEX_MSG_START_TRANSPARENT])
				break;
			InsertMsg(pConnect, INDEX_MSG_START_TRANSPARENT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_START_TRANSPARENT]);
		}
			break;
		case MSG_STOP_TRANSPARENT: {
			if (!pConnect->interval_valid[INDEX_MSG_STOP_TRANSPARENT])
				break;
			InsertMsg(pConnect, INDEX_MSG_STOP_TRANSPARENT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_START_TRANSPARENT]);
		}
			break;
		case MSG_SEND_TRANSPARENT: {
			if (!pConnect->interval_valid[INDEX_MSG_SEND_TRANSPARENT])
				break;
			InsertMsg(pConnect, INDEX_MSG_SEND_TRANSPARENT, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_SEND_TRANSPARENT]);
		}
			break;
		case MSG_REV_TRANSPARENT: {
#if 0
			int i;
			//		dec_transparence_info_t transparence_info;
			transparence_info_t transparence_info;
			msg_transparent_send_t msg_transparent_send;
			memcpy(&msg_transparent_send,net_msg.msg_data,sizeof(msg_transparent_send_t));
			printf("==============MSG_REV_TRANSPARENT");
			for (i = 0; i < msg_transparent_send.cmd_size; i++)
			{
				printf("%02x",msg_transparent_send.cmd_buf[i]);
			}
			//	st_param_getTranspParam(&transparence_info);
			st_param_getTransparenceStruct(&transparence_info);
			if (transparence_info.down_flag == 1 && (transparence_info.serial_no == 0 || transparence_info.serial_no ==1))
			{
				st_com_sendDataToCom(transparence_info.serial_no, msg_transparent_send.cmd_buf, msg_transparent_send.cmd_size);
			}
			if (pConnect->bidi_transparent.bidi_ransparent_callback != NULL)
			{
				transparent_rev_t transparent_rev;
				memcpy(&transparent_rev,net_msg.msg_data,sizeof(transparent_rev_t));
				msg_transparent_rev_t msg_transparent_rev;
				memcpy(&msg_transparent_rev, &transparent_rev.msg_transparent_rev, sizeof(msg_transparent_rev_t));
				if (transparent_rev.transparent_id == pConnect->bidi_transparent.transparent_start.transparent_id)
				{
					pConnect->bidi_transparent.bidi_ransparent_callback(TRANSPARENT_SUCCESS, msg_transparent_rev,pConnect->bidi_transparent.context);
				}
				else
				{
					pConnect->bidi_transparent.bidi_ransparent_callback(TRANSPARENT_EXCEPTION, msg_transparent_rev,pConnect->bidi_transparent.context);
				}
			}

			//windows.
			//---------------------------------------------------------------------------------

			if (pConnect->bidi_transparent.bidi_ransparent_callback != NULL)
			{
				msg_transparent_rev_t msg_transparent_rev;
				memcpy(&msg_transparent_rev, net_msg.msg_data, sizeof(msg_transparent_rev_t));
				pConnect->bidi_transparent.bidi_ransparent_callback(TRANSPARENT_SUCCESS, msg_transparent_rev,pConnect->bidi_transparent.context);
			}
#endif
		}
			break;
		case MSG_INPUTFILE_CONFIG: {
			if (!pConnect->interval_valid[INDEX_INPUTFILE_CONFIG])
				break;
			InsertMsg(pConnect, INDEX_INPUTFILE_CONFIG, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_INPUTFILE_CONFIG]);
		}
			break;
		case MSG_OUTPUTFILE_CONFIG: {
			if (!pConnect->interval_valid[INDEX_OUTPUTFILE_CONFIG])
				break;
			InsertMsg(pConnect, INDEX_OUTPUTFILE_CONFIG, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_OUTPUTFILE_CONFIG]);
		}
			break;
		case MSG_3G_SIGNAL: {
			if (!pConnect->interval_valid[INDEX_MSG_3G_SIGNAL])
				break;
			InsertMsg(pConnect, INDEX_MSG_3G_SIGNAL, net_msg);
			st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_3G_SIGNAL]);
		}
			break;

		case MSG_GET_PARAM:
		case MSG_SET_PARAM: {
			switch (msg_subtype) {
			case PARAM_HANDIO_CONTROL:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HANDIO_CONTROL])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HANDIO_CONTROL, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_HANDIO_CONTROL]);
				break;
			case PARAM_TIMEIO_CONTROL:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_TIMEIO_CONTROL])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_TIMEIO_CONTROL, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_TIMEIO_CONTROL]);
				break;
			case PARAM_AUTO_POWEROFF:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_AUTO_POWEROFF])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_AUTO_POWEROFF, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_AUTO_POWEROFF]);
				break;
			case PARAM_AUTO_POWERON:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_AUTO_POWERON])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_AUTO_POWERON, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_AUTO_POWERON]);
				break;
			case PARAM_USER_INFO:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_USER_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_USER_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_USER_INFO]);
				break;
			case PARAM_HAND_RECORD:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HAND_RECORD])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HAND_RECORD, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_HAND_RECORD]);
				break;
			case PARAM_NETWORK_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_NETWORK_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_NETWORK_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_NETWORK_CONFIG]);
				break;
			case PARAM_MISC_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_MISC_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_MISC_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_MISC_CONFIG]);
				break;
			case PARAM_PROBER_ALRAM:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_PROBER_ALRAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_PROBER_ALRAM, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_PROBER_ALRAM]);
				break;
			case PARAM_RECORD_PARAM:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_RECORD_PARAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_RECORD_PARAM, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_RECORD_PARAM]);
				break;
			case PARAM_TERM_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_TERM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_TERM_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_TERM_CONFIG]);
				break;
			case PARAM_TIMER_RECORD:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_TIMER_RECORD])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_TIMER_RECORD, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_TIMER_RECORD]);
				break;
			case PARAM_VIDEO_LOSE:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_VIDEO_LOSE])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_VIDEO_LOSE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_VIDEO_LOSE]);
				break;
			case PARAM_VIDEO_MOVE:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_VIDEO_MOVE])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_VIDEO_MOVE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_VIDEO_MOVE]);
				break;
			case PARAM_VIDEO_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_VIDEO_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_VIDEO_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_VIDEO_CONFIG]);
				break;
			case PARAM_VIDEO_ENCODE:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_VIDEO_ENCODE])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_VIDEO_ENCODE, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_VIDEO_ENCODE]);
				break;
			case PARAM_ALARM_PARAM:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ALARM_PARAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ALARM_PARAM, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_ALARM_PARAM]);
				break;
			case PARAM_COM_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_COM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_COM_CONFIG, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_COM_CONFIG]);
				break;
			case PARAM_OSD_SET:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_OSDSET])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_OSDSET, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_OSDSET]);
				break;
			case PARAM_OVERLAY_INFO:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_OVERLAY])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_OVERLAY, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_OVERLAY]);
				break;
			case PARAM_MOTION_ZOOM:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_MOTION_ZOOM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_MOTION_ZOOM, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_MOTION_ZOOM]);
				break;
			case PARAM_PPPOE:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_PPPOE])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_PPPOE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_PPPOE]);
				break;
			case PARAM_RECORD_INFO:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_RECORD_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_RECORD_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_RECORD_INFO]);
				break;
			case PARAM_WIRELESS_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_WIRELESS_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_WIRELESS_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_WIRELESS_CONFIG]);
				break;
			case PARAM_MAIL_CONFIG:
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_MAIL_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_MAIL_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_MAIL_CONFIG]);
				break;
			case PARAM_FTP_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_FTP_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_FTP_CONFIG, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_FTP_CONFIG]);
			}
				break;
			case PARAM_SERIAL_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_SERIAL_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_SERIAL_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_SERIAL_INFO]);
			}
				break;
			case PARAM_TRANSPARENCE_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_TRANSPARENCE_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_TRANSPARENCE_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_TRANSPARENCE_INFO]);
			}
				break;
			case PARAM_SLAVE_ENCODE: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_SLAVE_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_SLAVE_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_SLAVE_INFO]);
			}
				break;
			case PARAM_DEVICE_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_DEVICE_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_DEVICE_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_DEVICE_INFO]);
			}
				break;
			case PARAM_AUDIO_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_AUDIO_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_AUDIO_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_AUDIO_INFO]);
			}
				break;
			case PARAM_MEMBER_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_MEMBER_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_MEMBER_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_MEMBER_INFO]);
			}
				break;
			case PARAM_CAMERA_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_CAMERA_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_CAMERA_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_CAMERA_INFO]);
			}
				break;
			case PARAM_NAS_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_NAS_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_NAS_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_NAS_INFO]);
			}
				break;
			case PARAM_ITU_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ITU_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ITU_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_ITU_INFO]);
			}
				break;
			case PARAM_TIMER_COLOR2GREY: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_COLOR_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_COLOR_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_COLOR_INFO]);
			}
				break;
			case PARAM_NA208_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_NA208_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_NA208_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_NA208_INFO]);
			}
				break;
			case PARAM_MATRIX_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PAPAM_MATRIX_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PAPAM_MATRIX_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PAPAM_MATRIX_INFO]);
			}
				break;
			case PARAM_TIMER_CAPTURE: {
				if (!pConnect->interval_valid[INDEX_MSG_TIMER_CAPTURE])
					break;
				InsertMsg(pConnect, INDEX_MSG_TIMER_CAPTURE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_TIMER_CAPTURE]);
			}
				break;
			case PARAM_TIMER_RESTART: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_TIMER_RESTART])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_TIMER_RESTART, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_TIMER_RESTART]);
			}
				break;
			case PARMA_IMMEDIATE_CAPTURE: {
				if (!pConnect->interval_valid[INDEX_MSG_IMMEDIATE_CAPTURE])
					break;
				InsertMsg(pConnect, INDEX_MSG_IMMEDIATE_CAPTURE, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_IMMEDIATE_CAPTURE]);
			}
				break;
			case PARMA_PTZ_CURISE: {
				if (!pConnect->interval_valid[INDEX_PARMA_PTZ_CURISE])
					break;
				InsertMsg(pConnect, INDEX_PARMA_PTZ_CURISE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_PARMA_PTZ_CURISE]);
			}
				break;
			case PARAM_USB_TYPE: {
				if (!pConnect->interval_valid[INDEX_PARAM_USB_TYPE])
					break;
				InsertMsg(pConnect, INDEX_PARAM_USB_TYPE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_PARAM_USB_TYPE]);
			}
				break;
			case PARAM_CDMA_TIME: {
				if (!pConnect->interval_valid[INDEX_MSG_CDMA_TIME])
					break;
				InsertMsg(pConnect, INDEX_MSG_CDMA_TIME, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_CDMA_TIME]);
			}
				break;
			case PARAM_CDMA_ALARM_ENABLE: {
				if (!pConnect->interval_valid[INDEX_MSG_CDMA_ALARM_ENABLE])
					break;
				InsertMsg(pConnect, INDEX_MSG_CDMA_ALARM_ENABLE, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_CDMA_ALARM_ENABLE]);
			}
				break;
			case PARAM_CDMA_NONEGO_ENABLE: {
				if (!pConnect->interval_valid[INDEX_MSG_CDMA_NONEGO_ENABLE])
					break;
				InsertMsg(pConnect, INDEX_MSG_CDMA_NONEGO_ENABLE, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_CDMA_NONEGO_ENABLE]);
			}
				break;
			case PARAM_CDMA_STREAM_ENABLE: {
				if (!pConnect->interval_valid[INDEX_MSG_CDMA_STREAM_ENABLE])
					break;
				InsertMsg(pConnect, INDEX_MSG_CDMA_STREAM_ENABLE, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_CDMA_STREAM_ENABLE]);
			}
				break;
			case PARAM_CDMA_ALARM_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_CDMA_ALARM_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_CDMA_ALARM_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_CDMA_ALARM_INFO]);
			}
				break;
			case PARAM_CENTER_SERVER_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_CENTER_SERVER_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_CENTER_SERVER_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_CENTER_SERVER_CONFIG]);
			}
				break;
			case PARAM_LENS_INFO_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_LENS_INFO_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_LENS_INFO_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_LENS_INFO_CONFIG]);
			}
				break;
			case PARAM_H3C_PLATFORM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG]);
			}
				break;
			case PARAM_3511_SYSINFO: {
				if (!pConnect->interval_valid[INDEX_3511_PARAM_SYSINFO])
					break;
				InsertMsg(pConnect, INDEX_3511_PARAM_SYSINFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_3511_PARAM_SYSINFO]);
			}
				break;
			case PARAM_SZ_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_SZ_PARAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_SZ_PARAM, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_SZ_PARAM]);
			}
				break;
			case PARAM_DEFAULT_ENCODER: {
				if (!pConnect->interval_valid[INDEX_MSG_DEFAULT_ENCODER])
					break;
				InsertMsg(pConnect, INDEX_MSG_DEFAULT_ENCODER, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_DEFAULT_ENCODER]);
			}
				break;
			case PARAM_DECODE_OSD: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_DECODE_OSD])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_DECODE_OSD, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_DECODE_OSD]);
			}
				break;
			case PARAM_DECODER_BUF_TIME: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_DECODER_BUF_TIME])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_DECODER_BUF_TIME, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_DECODER_BUF_TIME]);
			}
				break;
			case PARAM_DECODER_BUF_FRAME_COUNT: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT]);
			}
				break;
			case PARAM_H3C_INFO: {
				if (!pConnect->interval_valid[INDEX_H3C_INFO])
					break;
				InsertMsg(pConnect, INDEX_H3C_INFO, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_H3C_INFO]);
			}
				break;
			case PARAM_H3C_NETCOM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_H3C_NETCOM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_H3C_NETCOM_CONFIG, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_H3C_NETCOM_CONFIG]);
			}
				break;
			case PARAM_LPS_CONFIG_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_LPS_CONFIG_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_LPS_CONFIG_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_LPS_CONFIG_INFO]);
			}
				break;
			case PARAM_KEDA_PLATFORM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG]);
			}
				break;
			case PARAM_BELL_PLATFORM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG]);
			}
				break;
			case PARAM_ZTE_PLATFORM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG]);
			}
				break;
			case PARAM_HXHT_PLATFORM_CONFIG: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG]);
			}
				break;
			case PARAM_HXHT_ALARM_SCHEME: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HXHT_ALARM_SCHEME])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HXHT_ALARM_SCHEME, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_HXHT_ALARM_SCHEME]);
			}
				break;
			case PARAM_UDP_NAT_CLINTINFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_UDP_NAT_CLINTINFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_UDP_NAT_CLINTINFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_UDP_NAT_CLINTINFO]);
			}
				break;
			case PARAM_HXHT_NAT: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HXHT_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HXHT_NAT, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_HXHT_NAT]);
			}
				break;
			case PARAM_FENGH_NAT: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_FENGH_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_FENGH_NAT, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_FENGH_NAT]);
			}
				break;
			case PARAM_GONGX_NAT: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_GONGX_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_GONGX_NAT, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_GONGX_NAT]);
			}
				break;
			case PARAM_HAIDO_PARAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HAIDOU_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HAIDOU_NAT, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_HAIDOU_NAT]);
			}
				break;
			case PARAM_HAIDO_NAS: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_HAIDOU_NAS])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_HAIDOU_NAS, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_HAIDOU_NAS]);
			}
				break;
			case PARAM_LANGTAO_PARAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_LANGTAO_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_LANGTAO_NAT, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_LANGTAO_NAT]);
			}
				break;
			case PARAM_UT_PARAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_UT_NAT])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_UT_NAT, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_UT_NAT]);
			}
				break;
			case PARAM_NETSTAR_CONFIG: {
				if (!pConnect->interval_valid[INDEX_DECORD_PARAM_NETSTAR])
					break;
				InsertMsg(pConnect, INDEX_DECORD_PARAM_NETSTAR, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_DECORD_PARAM_NETSTAR]);
			}
				break;
			case PARAM_ZXLW_PARAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ZXLW_PARAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ZXLW_PARAM, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_ZXLW_PARAM]);
			}
				break;
			case PARAM_YIXUN_PARAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_YIXUN_PARAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_YIXUN_PARAM, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_YIXUN_PARAM]);
			}
				break;
			case PARAM_CHECK_TIME: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_CHECK_TIME])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_CHECK_TIME, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_CHECK_TIME]);
			}
				break;
			case PARAM_ZTE_STREAM: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ZTE_STREAM])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ZTE_STREAM, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_ZTE_STREAM]);
			}
				break;
			case PARAM_ZTE_ONLINE: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_ZTE_ONLINE])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_ZTE_ONLINE, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_MSG_PARAM_ZTE_ONLINE]);
			}
				break;
			case PARAM_DECODE_NUM: {
				if (!pConnect->interval_valid[INDEX_DECORD_PARAM_NUM])
					break;
				InsertMsg(pConnect, INDEX_DECORD_PARAM_NUM, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_DECORD_PARAM_NUM]);
			}
				break;
			case PARAM_DECODE_STAT: {
				if (!pConnect->interval_valid[INDEX_DECORD_PARAM_STAR])
					break;
				InsertMsg(pConnect, INDEX_DECORD_PARAM_STAR, net_msg);
				st_comm_semBPost(&pConnect->sem_id[INDEX_DECORD_PARAM_STAR]);
			}
				break;
			case PARAM_KEY_WEB_PWD: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_WEB_PWD])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_WEB_PWD, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_WEB_PWD]);
			}
				break;
			case PARAM_KEY_USER_PWD: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_USER_PWD])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_USER_PWD, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_USER_PWD]);
			}
				break;
			case PARAM_KEY_SERVER_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_SERVER_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_SERVER_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_SERVER_INFO]);
			}
				break;
			case PARAM_KEY_ENC_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_ENC_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_ENC_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_ENC_INFO]);
			}
				break;
			case PARAM_KEY_ENC_CHANNEL_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO]);
			}
				break;
			case PARAM_KEY_DEC_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_DEC_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_DEC_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_DEC_INFO]);
			}
				break;
			case PARAM_KEY_TOUR_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_TOUR_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_TOUR_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_TOUR_INFO]);
			}
				break;
			case PARAM_KEY_TOUR_STEP_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO]);
			}
				break;
			case PARAM_KEY_GROUP_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_GROUP_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_GROUP_INFO, net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_GROUP_INFO]);
			}
				break;
			case PARAM_KEY_GROUP_STEP_INFO: {
				if (!pConnect->interval_valid[INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO])
					break;
				InsertMsg(pConnect, INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO,
						net_msg);
				st_comm_semBPost(
						&pConnect->sem_id[INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO]);
			}
				break;
			default:
				break;
			} //switch(msg_subtype).
		}
			break; //case MSG_GET_PARAM,MSG_SET_PARAM:
		default:
			break;
		} //switch(msg_type).
	} //while(pConnect->msg_flag).
	Exit:
	LOGE("Exit---->MsgThread!");
	return NULL;
} //end MsgThread()

static void *TcpReceiveThread(void *param) {
	LOGI(" ");
	LOGI("Create----->TcpReceiveThread!");
	pthread_detach(pthread_self());

	int sendLen = 0;
	net_pack_t net_pack;
	memset(&net_pack, 0, sizeof(net_pack_t));

	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char *full_frame_data = malloc(DATA_BUF_LEN);
	memset(full_frame_data, 0, DATA_BUF_LEN);
	long oldtick = 0;
	time_t now_time;

	channel_info_t *channel_info = (channel_info_t*) param;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) channel_info->user_id;
	channel_info->protocol_type = PROTOCOL_TCP;
	pServerInfo->server_connect->bReopenFlag = FALSE;
	//LOGI("TcpReceiveThread! channel_info->sock = %d",channel_info->sock);

	fd_set rdfds;
	struct timeval timeout;

	const int previewCnt = 200; //200*50*1000 = 10 S
	int cntTimeout = 50 * 1000;

	//get/set socket opctionLinZh107
	if (PROTOCOL_TCP != g_protocol) {
		int rcvTimeout = cntTimeout / 100; //50 msec
		int iRet = udtc_setsockopt(channel_info->sock, 0, UDT_RCVTIMEO,
				(char*) &rcvTimeout, sizeof(int));
		udtc_setsockopt(channel_info->sock, 0, UDT_SNDTIMEO,
				(char*) &rcvTimeout, sizeof(int));
		LOGI("setsockopt return %d", iRet);
	}
	//******* (200*50ms = 10s) 十秒钟没数据过来 需要断线重连  ********

	int cnt = previewCnt;
	while (channel_info->preview_flag) {
		// add udt   modify by LinZh107
		//************************* receive msg header *********************
		//********************** TCP protocol ******************************
		int recvLen = -1;
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&rdfds);
			FD_SET((unsigned int) channel_info->sock, &rdfds);
			timeout.tv_sec = 0; //modify 2014-9-17 LinZh107  修复容易断线的问题，包括以下所有接收出错时的处理
			timeout.tv_usec = cntTimeout; //所有的select 再调用之前
			switch (select(channel_info->sock + 1, &rdfds, NULL, NULL, &timeout)) {
			case -1:
//				LOGE("Preview Select Head error! Msg = %s", strerror(errno));
//				if(channel_info->preview_flag == 1){
//					pServerInfo->server_connect->bReopenFlag = TRUE;
//					pServerInfo->server_connect->bReloginFlag = TRUE;
//				}
//				goto error;
			case 0:
				cnt--;
				if (cnt == 0 && channel_info->preview_flag == 1) {
					pServerInfo->server_connect->bReopenFlag = TRUE;
					pServerInfo->server_connect->bReloginFlag = TRUE;
					goto error;
				}
				continue; //再次轮询
			default:
				if (FD_ISSET(channel_info->sock, &rdfds)) //测试sock是否可读，即是否网络上有数据
					recvLen = readn(channel_info->sock, (char*) &net_pack,
							sizeof(pack_head_t));
				break;
			} // end switch
		}

		//********************** UDT/P2P protocol ******************************
		else
			recvLen = udtc_recvn(channel_info->sock, (char*) &net_pack,
					sizeof(pack_head_t));

		if (-1 == recvLen) {
			cnt--;
			if (cnt == 0 && channel_info->preview_flag == 1) {
				pServerInfo->server_connect->bReopenFlag = TRUE;
				pServerInfo->server_connect->bReloginFlag = TRUE;
				goto error;
			}
			LOGI("TcpReceive udtc_recv error. continue");
			continue;
		}

		if (first_frame == 0) {
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

		if (net_pack.pack_head.pack_size > MAX_NET_PACK_SIZE)
			continue;

		//*************************** receive msg body *********************
		//********************** TCP protocol ******************************
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&rdfds);
			FD_SET((unsigned int) channel_info->sock, &rdfds);
			timeout.tv_sec = 0; //modify 2014-9-17 LinZh107  修复容易断线的问题，包括以下所有接收出错时的处理
			timeout.tv_usec = cntTimeout; //所有的select 再调用之前
			switch (select(channel_info->sock + 1, &rdfds, NULL, NULL, &timeout)) {
			case -1:
				LOGE("Tcp select msg_data error! Msg = %s", strerror(errno));
				goto error;
			case 0:
				cnt--;
				if (cnt == 0 && channel_info->preview_flag == 1) {
					pServerInfo->server_connect->bReopenFlag = TRUE;
					pServerInfo->server_connect->bReloginFlag = TRUE;
					goto error;
				}
				continue; //can not be
			default:
				if (FD_ISSET(channel_info->sock, &rdfds))
					recvLen = readn(channel_info->sock,
							(char*) &net_pack + sizeof(pack_head_t),
							net_pack.pack_head.pack_size);
				break;
			} // end switch
		}
		//********************** UDT/P2P protocol ******************************
		else {
			recvLen = udtc_recvn(channel_info->sock,
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
		}

		if (sizeof(pack_head_t) == recvLen) {
			cnt--;
			if (cnt == 0 && channel_info->preview_flag == 1) {
				pServerInfo->server_connect->bReopenFlag = TRUE;
				pServerInfo->server_connect->bReloginFlag = TRUE;
				goto error;
			}
			continue;
		}

		// before the outtime coming there are data come in ,so recount again.
		cnt = previewCnt;

		//********************** analyze the receive data **********************
		// copy the net_pack data to full_frame_data buffer
		if (frame_no == net_pack.pack_head.frame_no) {
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
		} else //满一帧
		{
			if (full_frame_size + (int) sizeof(frame_head_t)
					== recv_frame_size) {
				if (channel_info->record_flag == TRUE) //录制
				{
					frame_t *pframe = (frame_t *) full_frame_data;
					if (pframe->frame_head.frame_type == VIDEO_FRAME)
						channel_info->bIflag = TRUE;
					if (TRUE == channel_info->bIflag)
						fwrite(full_frame_data, 1, recv_frame_size,
								channel_info->record_file);
				}
				if (channel_info->callback.channel_callback != NULL) //送解码
				{
					//LOGI("TcpRecvThread received a full flame!");
					channel_info->callback.channel_callback(
							(void *) full_frame_data, recv_frame_size,
							(void*) channel_info->callback.context);
				}
			} else {
				frame_t *pframe = (frame_t *) full_frame_data;
				if (pframe->frame_head.frame_type == VIDEO_FRAME) {
					if (difftime(now_time, oldtick) >= 2) {
						LOGI("---msg_force_keyframe---");
						net_msg_t net_msg;
						msg_force_keyframe_t msg_force_keyframe;

						int msg_sock = channel_info->msg_sock;
						time(&now_time);
						oldtick = now_time;

						msg_force_keyframe.channel = channel_info->channel_no;
						net_msg.msg_head.msg_type = MSG_FORCE_KEYFRAME;
						net_msg.msg_head.msg_subtype = 0;
						net_msg.msg_head.msg_size =
								sizeof(msg_force_keyframe_t);
						memcpy(net_msg.msg_data, (char *) &msg_force_keyframe,
								sizeof(msg_force_keyframe_t));
						if (PROTOCOL_TCP == g_protocol)
							sendLen = writen(msg_sock, (char*) &net_msg,
									sizeof(msg_head_t)
											+ sizeof(msg_force_keyframe_t));
						else
							sendLen = udtc_sendn(msg_sock, (char*) &net_msg,
									sizeof(msg_head_t)
											+ sizeof(msg_force_keyframe_t));

						if (sendLen
								!= (int) (sizeof(msg_head_t)
										+ sizeof(msg_force_keyframe_t)))
							LOGI("MSG_FORCE_KEYFRAME error!");
					}
				}
			}

			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}
	} //end while (channel_info->preview_flag)

	error: if (full_frame_data != NULL) {
		free(full_frame_data);
		full_frame_data = NULL;
		//LOGE("---error: no legal frame ---");
	}

	LOGE("Exit---->TcpRecvThread!");
	return (NULL);
} //end TcpReceiveThread

#if 0
static void *UdpReceiveThread(void *param)
{
	struct sockaddr_in RemoteAddr;
	int len;
	int Remotelen = sizeof(RemoteAddr);

	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char *full_frame_data = malloc(DATA_BUF_LEN);

	long oldtick = 0;
	time_t now_time;

	struct timeval tval;
	fd_set rset;
	net_pack_t net_pack;
	channel_info_t *channel_info = (channel_info_t*) param;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) channel_info->user_id;
	channel_info->preview_flag = 1;
	channel_info->protocol_type = PROTOCOL_UDP;
	while (channel_info->preview_flag)
	{
		//__android_log_print(ANDROID_LOG_DEBUG, "UdpReceiveThread: bit_rate = ", "----:%d",channel_info->bit_rate);
		FD_ZERO(&rset);
		FD_SET((unsigned int) channel_info->sock, &rset);
		tval.tv_sec = 10;// Date: 2014-1-8 Author: yms
		tval.tv_usec = 100 * 1000;
		if (select(channel_info->sock + 1, &rset, 0, 0, &tval) <= 0)
		{
			udtc_close(channel_info->sock);
			LOGI("------------udtc_close---------");
			pServerInfo->ichannelStatus[channel_info->channel_no] = 0;
			//pServerInfo->iLogin = 0;
			return NULL;
		}

		len = recvfrom(channel_info->sock, (char *) &net_pack, sizeof(net_pack_t), 0,
				(struct sockaddr *) ((void *) (&RemoteAddr)), (unsigned int *) &Remotelen);
		if (len < 0)
		{
			LOGE("UDP recvfrom error");
			udtc_close(channel_info->sock);
			LOGI("------------udtc_close---------");
			channel_info->preview_flag = 0;
			pServerInfo->ichannelStatus[channel_info->channel_no] = 0;
			continue;
		}

		if (first_frame == 0)
		{
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

		//LOGI("pack_no = %d, pack_count = %d, pack_size = %d", net_pack.pack_head.pack_no, net_pack.pack_head.pack_count, net_pack.pack_head.pack_size);

		//		LOGI("frame_no = %d", net_pack.pack_head.frame_no);

		//__android_log_print(ANDROID_LOG_DEBUG, "frame_no", "----:%d",frame_no);
		//__android_log_print(ANDROID_LOG_DEBUG, "net_pack.pack_head.frame_no", "----:%d",net_pack.pack_head.frame_no);
		//__android_log_print(ANDROID_LOG_DEBUG, "net_pack.pack_head.pack_no", "----:%d",net_pack.pack_head.pack_no);

		if (frame_no == net_pack.pack_head.frame_no)
		{
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			if (offset <= 512 * 1024)
			{
				memcpy(&full_frame_data[offset], (char*) &net_pack + sizeof(pack_head_t), net_pack.pack_head.pack_size);
				recv_frame_size += net_pack.pack_head.pack_size;
			}
		}
		else //满一帧
		{
			frame_t *pframe = (frame_t *) full_frame_data;
			if (full_frame_size + (int) sizeof(frame_head_t) == recv_frame_size)
			{
				//__android_log_print(ANDROID_LOG_DEBUG, "man yi zhen","man yi zhen");
				if (channel_info->callback.channel_callback != NULL)
				{
					LOGI("recv_frame_size----:%d",recv_frame_size);

					channel_info->callback.channel_callback((void *) full_frame_data, recv_frame_size,
							(void*) channel_info->callback.context);
				}
			}
			else
			{
				frame_t *pframe = (frame_t *) full_frame_data;
//				LOGI("frame_type = %d, frame_no = %d, full_frame_size = %d
//						, recv_frame_size = %d", pframe->frame_head.frame_type,
//                                         pframe->frame_head.frame_no, full_frame_size, recv_frame_size);
//				LOGI("pack_no = %d, pack_count = %d, pack_size = %d"
//					, net_pack.pack_head.pack_no, net_pack.pack_head.pack_count
//					, net_pack.pack_head.pack_size);
				if (pframe->frame_head.frame_type == 1)
				{
					if (difftime(now_time, oldtick) >= 2)
					{
						net_msg_t net_msg;
						msg_force_keyframe_t msg_force_keyframe;
						int iRet;

						int msg_sock = channel_info->msg_sock;
						time(&now_time);
						oldtick = now_time;

						msg_force_keyframe.channel = channel_info->channel_no;

						net_msg.msg_head.msg_type = MSG_FORCE_KEYFRAME;
						net_msg.msg_head.msg_subtype = 0;
						net_msg.msg_head.msg_size = sizeof(msg_force_keyframe_t);
						memcpy(net_msg.msg_data, (char *) &msg_force_keyframe, sizeof(msg_force_keyframe_t));
						iRet = udtc_sendn(msg_sock, (char *) &net_msg, sizeof(msg_head_t) + sizeof(msg_force_keyframe_t));
						if (iRet != (int) (sizeof(msg_head_t) + sizeof(msg_force_keyframe_t)))
						LOGI("MSG_FORCE_KEYFRAME error! 2");
					}
				}
			}

			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset], (char*) &net_pack + sizeof(pack_head_t), net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;

			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}

	}
	free(full_frame_data);
	return NULL;
}

static void *McastReceiveThread(void *param)
{
	struct sockaddr_in RemoteAddr;
	int len;
	int Remotelen = sizeof(RemoteAddr);

	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char full_frame_data[DATA_BUF_LEN];

	long oldtick = 0;
	time_t now_time;
	net_pack_t net_pack;
	channel_info_t *channel_info = (channel_info_t*) param;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) channel_info->user_id;
	channel_info->protocol_type = PROTOCOL_UDP;

	pthread_detach(pthread_self());
	while (channel_info->preview_flag)
	{

		struct timeval tval;
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET((unsigned int) channel_info->sock, &rset);
		tval.tv_sec = 10; // Date: 2014-1-8 Author: yms
		tval.tv_usec = 0;
		LOGI("McastReceiveThread sock :%d", channel_info->sock);
		if (select(channel_info->sock + 1, &rset, 0, 0, &tval) <= 0)
		{
			//__android_log_print(ANDROID_LOG_DEBUG, "deng dai chao shi--123", "this is my debug log:%s", __FUNCTION__);
			/*close(channel_info->sock);
			 pServerInfo->ichannelStatus[channel_info->channel_no] = 0;
			 //pServerInfo->iLogin = 0;
			 return NULL;*/
		}
		len = recvfrom(channel_info->sock, (char *) &net_pack, sizeof(net_pack_t), 0, (struct sockaddr *) &RemoteAddr,
				(unsigned int *) &Remotelen);
		//__android_log_print(ANDROID_LOG_INFO, "buf length :", "%d/n", len);
		if (len < 0)
		{
			udtc_close(channel_info->sock);
			LOGI("------------udtc_close---------");
			channel_info->preview_flag = 0;
			pServerInfo->ichannelStatus[channel_info->channel_no] = 0;
			continue;
		}

		//码流
#if 0
		if (channel_info->bBitRate == FALSE)
		{
			channel_info->frontsecond = getcurrentsecond();
			channel_info->bBitRate = TRUE;
		}
		if (channel_info->frontsecond == getcurrentsecond())
		{
			channel_info->tmp_bit_rate += (sizeof(pack_head_t) + len);
		}
		else
		{
			channel_info->bit_rate = (channel_info->tmp_bit_rate + sizeof(pack_head_t) + len) * 8;
			channel_info->tmp_bit_rate = 0;
			channel_info->bBitRate = FALSE;

		}
#endif

		if (first_frame == 0)
		{
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

		//		LOGI("pack_no = %d, pack_count = %d", net_pack.pack_head.pack_no, net_pack.pack_head.pack_count);

		if (frame_no == net_pack.pack_head.frame_no)
		{
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;

			memcpy(&full_frame_data[offset], (char*) &net_pack + sizeof(pack_head_t), net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
			//	LOGI("offset = %d, pack_size = %d", offset, net_pack.pack_head.pack_size);

		}
		else //满一帧
		{
			//			LOGI("full_frame_size = %d, recv_frame_size = %d", full_frame_size, recv_frame_size);
			if (full_frame_size + (int) sizeof(frame_head_t) == recv_frame_size)
			{
				if (channel_info->record_flag == TRUE)
				{
					frame_t *pframe = (frame_t *) full_frame_data;
					if (pframe->frame_head.frame_type == 1)
					channel_info->bIflag = TRUE;
					if (channel_info->bIflag == TRUE)
					fwrite(full_frame_data, 1, recv_frame_size, channel_info->record_file);
//						AVI_fwrite((void *)channel_info->record_file, full_frame_data);
				}

				if (channel_info->callback.channel_callback != NULL)
				{
//					fwrite(full_frame_data, 1, recv_frame_size, fp);
//					channel_info->callback.channel_callback(NULL
//					, channel_info->callback.channel,full_frame_data
//					, recv_frame_size, channel_info->callback.context);
					channel_info->callback.channel_callback((void *) full_frame_data, recv_frame_size,
							(void*) channel_info->callback.context);
				}
			}
			else
			{
				frame_t *pframe = (frame_t *) full_frame_data;
				if (pframe->frame_head.frame_type == 1)
				{
					if (difftime(now_time, oldtick) >= 2)
					{
						int iRet;
						net_msg_t net_msg;
						msg_force_keyframe_t msg_force_keyframe;

						int msg_sock = channel_info->msg_sock;
						time(&now_time);
						oldtick = now_time;

						msg_force_keyframe.channel = channel_info->channel_no;

						net_msg.msg_head.msg_type = MSG_FORCE_KEYFRAME;
						net_msg.msg_head.msg_subtype = 0;
						net_msg.msg_head.msg_size = sizeof(msg_force_keyframe_t);
						memcpy(net_msg.msg_data, (char *) &msg_force_keyframe, sizeof(msg_force_keyframe_t));

						iRet = udtc_sendn(msg_sock, (char *) &net_msg, sizeof(msg_head_t) + sizeof(msg_force_keyframe_t));
						if (iRet != (int) (sizeof(msg_head_t) + sizeof(msg_force_keyframe_t)))
						LOGI("MSG_FORCE_KEYFRAME error! 3");
					}
				}
			}

			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset], (char*) &net_pack + sizeof(pack_head_t), net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;

			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}

	}
	return NULL;
}
#endif

static void *HeartBeatThread(void *param) {
	LOGI(" ");
	LOGI("Create----->HeartBeatThread!");
	pthread_detach(pthread_self());

	tp_server_info_t *pServerInfo = (tp_server_info_t *) param;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t net_msg;
	int sendLen = 0;
	if (pConnect == NULL)
		return NULL;
	net_msg.msg_head.msg_type = MSG_HEARTBEAT;
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_size = 0;

	struct timeval tval;
	fd_set wtfds;

	int previewCnt = 100;
	int cntTimeout = 50 * 1000;

	while (pServerInfo->iMsgStatus) {
		//LOGI("---->Send MSG_HEARTBEAT to remote");
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&wtfds);
			FD_SET((unsigned int)pConnect->msg_sock, &wtfds);
			tval.tv_sec = 0;
			tval.tv_usec = cntTimeout;
			switch (select(pConnect->msg_sock + 1, NULL, &wtfds, NULL, &tval)) {
			case -1:
			case 0:
				LOGE("Heartbeat Select sock error! Msg = %s", strerror(errno));
				usleep(1000);
				continue;
			default:
				if (FD_ISSET(pConnect->msg_sock, &wtfds)) //测试sock是否可读，即是否网络上有数据
					sendLen = writen(pConnect->msg_sock, (char *) &net_msg,
							sizeof(msg_head_t));
				break;
			} // end switch
		} else
			sendLen = udtc_sendn(pConnect->msg_sock, (char *) &net_msg,
					sizeof(msg_head_t));

		//if(sendLen == sizeof(msg_head_t))
		//LOGI(">>>> send Heartbeat to remote succeed!");

		//************* send heartbeat every 5 Seconds ***************
		int cnt = previewCnt;
		while (cnt > 0) {
			if (!pServerInfo->iMsgStatus) {
				goto exit;
			}
			usleep(cntTimeout);
			cnt--;
		}
	}
	exit:
	LOGE("Exit---->HeartBeatThread!");
	return NULL;
}

/***********************************************************************************************************
 *
 * the following fucntions is export
 * added by LinZh107 at 2014-10-10	 to   fix the Reconnect
 *
 **********************************************************************************************************/

static void *ReconnectThread(void *param) {
	LOGI(" ");
	LOGI("Create----->ReconnectThread!");
	pthread_detach(pthread_self());

	unsigned long user_id = (unsigned long) param;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) param;
	server_connect_t *pConnect = pServerInfo->server_connect;
	channel_info_t *channel_info =
			(channel_info_t *) &pConnect->channel_info[pConnect->channel];

	int iRet = -1;
	while (pServerInfo->iLogin) {
		//********* check if the Login Statue is OK  every 15 Seconds **********
		int cnt;
		if (PROTOCOL_TCP == g_protocol)
			cnt = 400; // 400*50 = 20s, check if needed to reconnect by every 20 Second
		else
			cnt = 600; // 800*50 = 40s, check if needed to reconnect by every 30 Second
		while (cnt > 0) {
			if (!pServerInfo->iLogin) {
				goto exit;
			}
			usleep(50 * 1000);
			cnt--;
		}

		if (!pServerInfo->iLogin)
			goto exit;

		int channel = channel_info->channel_no;
		int channel_type = channel_info->channel_type;
		int stream_type = channel_info->stream_type;
		int protocol_type = channel_info->protocol_type;

		if (pConnect->bReloginFlag == TRUE) //检测通道打开状态，如果未打开就重新打开并取流
		{
			//LOGI("Reconnect - close and reopen channel!");
			st_net_userLogout(user_id, FALSE);
			pServerInfo->iLogin = 1;

			//wait for other thread stop completlly
			usleep(50 * 000);

			iRet = -1;
			iRet = st_net_userLogin(user_id);
			if (iRet == 0) //自动登录不成功，则过一段时间再登录
					{
				pConnect->bReloginFlag = FALSE;
				if (pConnect->bReopenFlag == TRUE) {
					iRet = -1;
					iRet = st_net_openChannelStream(user_id, channel,
							protocol_type, channel_type);
					if (iRet != 0) {
						LOGE("Reconnect - openChannelStream nRet = %d", iRet);
						continue;
					}
					st_net_registerChannelStreamCallback(user_id, channel,
							data_callback, NULL);
					iRet = -1;
					iRet = st_net_startChannelStream(user_id, channel,
							stream_type, channel_type);
					if (iRet != 0) {
						LOGE("Reconnect - startChannelStream nRet = %d", iRet);
						continue;
					} else
						pConnect->bReopenFlag = FALSE;
				}
				goto exit;
			} else {
				//LOGE("Reconnect - Relogin Error!");
				continue;
			}
		}
		if (pConnect->bReopenFlag == TRUE) {
			iRet = -1;
			iRet = st_net_openChannelStream(user_id, channel, protocol_type,
					channel_type); //最后的那个参数和关闭通道的一样，是通道的类型
			if (iRet != 0) {
				LOGE("Reconnect - openChannelStream nRet = %d", iRet);
				continue;
			}
			st_net_registerChannelStreamCallback(user_id, channel,
					data_callback, NULL);
			iRet = -1;
			iRet = st_net_startChannelStream(user_id, channel, stream_type,
					channel_type); //最后的参数为0表示复合流
			if (iRet != 0) {
				LOGE("Reconnect - startChannelStream nRet = %d", iRet);
				continue;
			} else
				pConnect->bReopenFlag = FALSE;
		}
	}

	exit:
	LOGE("Exit---->ReconnectThread!");
	return NULL;
}

long st_net_initServerInfo(char *ip_addr, int port, char *user_name,
		char *user_pwd, char *puid, int proto) {
//	LOGI("initServerInfo ip:%s, port:%d, name:%s, pwd:%s, proto:%d, pu_id:%s", ip_addr, port,
//			user_name, user_pwd, proto, puid);

	tp_server_info_t *pServerInfo = NULL;
	if ((pServerInfo = (tp_server_info_t*) malloc(sizeof(tp_server_info_t)))
			== NULL)
		return -1;

	memset(pServerInfo, 0, sizeof(tp_server_info_t));
	strcpy(pServerInfo->ip_addr, ip_addr);
	pServerInfo->port = port;
	strcpy(pServerInfo->user_name, user_name);
	strcpy(pServerInfo->user_pwd, user_pwd);
	//2014-7-25 
	strcpy(pServerInfo->pu_id, puid);
	pServerInfo->protocol = proto;
	if (PROTOCOL_TCP == proto)
		g_protocol = PROTOCOL_TCP;
	else if (PROTOCOL_UDT == proto) {
		g_protocol = PROTOCOL_UDT;
		udtc_initLib();
	} else {
		g_protocol = PROTOCOL_P2P;
		udtc_initLib();
	}

	// 2014-4-1
	pthread_mutex_init(&alarmInfoMutex, 0);

	// 2014-12-24  LinZh107
	pthread_mutex_init(&LogoutMutex, NULL);
	pthread_mutex_init(&CloseChannelMutex, NULL);

	// 2014-4-15
	memset(&g_AlarmInfoGroup, 0,
			MSG_ALARM_INFO_GROUP * sizeof(msg_alarm_info_t));

	LOGI("st_net_initServerInfo successed!");
	return ((long) pServerInfo);
}

int st_net_deinitServerInfo(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo != NULL) {
		if (PROTOCOL_TCP != g_protocol)
			udtc_deinitLib();
		free(pServerInfo);
		pServerInfo = NULL;
		user_id = (unsigned long) NULL;
	}
	// 2014-4-1
	pthread_mutex_destroy(&alarmInfoMutex);
	// 2014-12-24  LinZh107
	pthread_mutex_destroy(&LogoutMutex);
	pthread_mutex_destroy(&CloseChannelMutex);
	return 0;
}

// 2014-3-28
int st_net_setServerInfo(const unsigned long user_id, const char *ip_addr,
		const int port, const char *user_name, const char *user_pwd) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (NULL == pServerInfo)
		return -1;

	strcpy(pServerInfo->ip_addr, ip_addr);
	pServerInfo->port = port;
	strcpy(pServerInfo->user_name, user_name);
	strcpy(pServerInfo->user_pwd, user_pwd);

	return 0;
}

// add by LinZh107 for debug ice connect at 2015-2-14 10:15:00
int st_net_set_iceinfo(unsigned long user_id, const char* iceinfo, int ice_size) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL) {
		LOGE("Logout_pConnect == NULL");
		return -1;
	}

	remote_icelen = strlen(iceinfo);
	memcpy(remote_icedata, iceinfo, ice_size);
	if (ice_size == remote_icelen)
		LOGI("remote ice info Length = %d", remote_icelen);
	else
		LOGE("remote ice info Length = %d", remote_icelen);
	remote_icelen = ice_size;

	return ice_size;
}

int st_net_userLogin(unsigned long user_id) {
	LOGI("---->userLogin!");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL) {
		LOGE("Logout_pConnect == NULL");
		return -1;
	}
	server_connect_t *pConnect = NULL;
	pConnect = (server_connect_t *) malloc(sizeof(server_connect_t));
	if (!pConnect) {
		LOGE("error to malloc for pConnect!");
		return -1;
	}
	memset(pConnect, 0, sizeof(server_connect_t));

	int i = 0;
	int msg_sock = -1;
	int iRet = ACK_ERROR_OTHER;

	char ipaddr[32] = { 0 };
	struct sockaddr_in remote_addr;
	int TimeOut = OVER_TIME;

	net_msg_t net_msg;
	msg_login_t msg_login;
	msg_login_t recv_login;
	msg_session_id_t msg_session_id;

	time_t current_time;
	struct tm *curren_local_time;

	pthread_mutex_init(&pConnect->tcpmutex_id, NULL);

	for (; i < NETLIB_MAX_MSG_NUM; i++) {
		st_comm_semInit(&pConnect->sem_id[i], 0);
		pConnect->interval_valid[i] = FALSE;
		pConnect->p_net_msg[i] = NULL;
	}

	pConnect->channel = 0;
	//remove at 2015-2-13 , for pjproject instead of libnice.
	//pConnect->p2pParam = NULL;

	int szOptval = 1;

	//***************************************************************
	//add p2p protocol LinZh107 2014-7-24
	//TCP | UDT 协议通信
	if (PROTOCOL_TCP == g_protocol) {
		LOGI(
				"Login name:%s, pwd:%s, proto:TCP, ip:%s, port:%d", pServerInfo->user_name, pServerInfo->user_pwd, pServerInfo->ip_addr, pServerInfo->port);
#if 1
		if (inet_addr((pServerInfo->ip_addr)) == INADDR_NONE) {
			char *strIP;
			struct hostent *hp;
			if ((hp = gethostbyname(pServerInfo->ip_addr)) != NULL) //根据网络域名ip获取对应的IP地址
			{
				strIP = inet_ntoa(*(struct in_addr *) hp->h_addr_list[0]);
				if (strlen(strIP) == 0) {
					strIP = "Not available";
				}
				//my_log(strIP);
				sprintf(ipaddr, "%s", strIP);
			} else
				return iRet;
		} else {
			strcpy(ipaddr, pServerInfo->ip_addr);
		}
#endif

		msg_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (msg_sock <= 0) {
			return iRet;
		}

		//允许重用本地地址和端口
		if (setsockopt(msg_sock, SOL_SOCKET, SO_REUSEADDR, (void*) &szOptval,
				sizeof(szOptval)) == -1) {
			LOGE("Login Setsockopt Error 1! Msg = %s", strerror(errno));
			goto error;
		}

		remote_addr.sin_family = PF_INET;
		remote_addr.sin_addr.s_addr = inet_addr(ipaddr); //pServerInfo->ip_addr//ipaddr1
		remote_addr.sin_port = htons(pServerInfo->port);

		/***************************************************
		 * 非阻塞socket的连接
		 * 用select可以很好地解决这一问题
		 * 大致过程是这样的:
		 * 1.将打开的socket设为非阻塞的,可以用fcntl(socket, F_SETFL, O_NDELAY)完成(有的系统用FNEDLAY也可).
		 * 2.发connect调用,这时返回-1,但是errno被设为EINPROGRESS,意即connect仍旧在进行还没有完成.
		 * 3.将打开的socket设进被监视的可写(注意不是可读)文件集合用select进行监视,如果可写,用g
		 * etsockopt(socket, SOL_SOCKET, SO_ERROR, &error, sizeof(int));
		 * 来得到error的值,如果为零,则connect成功.
		 */
		LOGI("msg_sock = %d, TimeOut = %d", msg_sock, TimeOut);
		int ret = connect_nonb(msg_sock, (struct sockaddr*) &remote_addr,
				sizeof(struct sockaddr), TimeOut);
		if (ret != 0) {
			iRet = NET_EXCEPTION;
			goto error;
		}
	} else if (PROTOCOL_UDT == g_protocol) {
		//UDT 协议通信
		LOGI(
				"Login name:%s, pwd:%s, proto:UDT, ip:%s, port:%d", pServerInfo->user_name, pServerInfo->user_pwd, pServerInfo->ip_addr, pServerInfo->port);

		msg_sock = udtc_socket(PF_INET, SOCK_STREAM, 0);
		if (msg_sock <= 0) {
			LOGE("PROTOCOL_UDT userLogin error");
			return iRet;
		}

		iRet = udtc_setsockopt(msg_sock, SOL_SOCKET, SO_REUSEADDR,
				(void*) &szOptval, sizeof(szOptval));
		if (iRet == -1) {
			iRet = ACK_ERROR_OTHER;
			goto error;
		}

		remote_addr.sin_family = PF_INET;
		remote_addr.sin_addr.s_addr = inet_addr(pServerInfo->ip_addr); //pServerInfo->ip_addr <-- ipaddr1
		remote_addr.sin_port = htons(pServerInfo->port);

		if (0
				!= udtc_connect(msg_sock, (struct sockaddr*) &remote_addr,
						sizeof(struct sockaddr))) {
			iRet = NET_EXCEPTION;
			goto error;
		}
	} else if (PROTOCOL_P2P == g_protocol) {
		//P2P 协议通信
		LOGI(
				"Login name:%s, pwd:%s, proto:P2P, pu_id:%s", pServerInfo->user_name, pServerInfo->user_pwd, pServerInfo->pu_id);

		//remove at 2015-2-13 , for pjproject instead of libnice.

		msg_sock = tp_pjice_getsock(user_id);
		if (msg_sock <= 0) {
			LOGE("PROTOCOL_P2P Login error");
			return NET_EXCEPTION;
		}
		//**************** p2p_方案更换 ****************end
	}

	strcpy(pConnect->ip_addr, pServerInfo->ip_addr);
	pConnect->port = pServerInfo->port;
	memcpy(&pConnect->server_sockaddr, &remote_addr,
			sizeof(struct sockaddr_in));
	memset(&msg_login, 0, sizeof(msg_login_t));
	strcpy(msg_login.user_name, pServerInfo->user_name);
	strcpy(msg_login.user_pwd, pServerInfo->user_pwd);
	msg_login.local_right = 0;
	msg_login.remote_right = 0;

	time(&current_time);
	curren_local_time = localtime(&current_time);
	msg_login.local_time.year = curren_local_time->tm_year + 1900;
	msg_login.local_time.month = curren_local_time->tm_mon + 1;
	msg_login.local_time.day = curren_local_time->tm_mday;
	msg_login.local_time.hour = curren_local_time->tm_hour;
	msg_login.local_time.minute = curren_local_time->tm_min;
	msg_login.local_time.second = curren_local_time->tm_sec;

	pConnect->interval_valid[INDEX_MSG_LOGIN] = TRUE;
	pConnect->interval_valid[INDEX_MSG_SESSION_ID] = TRUE;
	pConnect->msg_sock = msg_sock;

	/******************************请求登录********************************/
	//请求登录
	iRet = SendNetMsg(pConnect->msg_sock, MSG_LOGIN, 0, (char *) &msg_login,
			sizeof(msg_login_t));
	if (0 != iRet) {
		iRet = ACK_ERROR_OTHER;
		goto error;
	}
	pConnect->bPortOffset = TRUE; //add by fengzx 2011-07-05

	pthread_create(&pConnect->msg_thread_id, NULL, MsgThread, (void*) pConnect);

	iRet = 0; //LinZh107 movie here
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_LOGIN], OVER_TIME)
			== 0) {
		if (GetMsg(pConnect, INDEX_MSG_LOGIN, &net_msg) < 0) {
			iRet = ACK_ERROR_OTHER;
			LOGE("GetMsg MSG_LOGIN error!");
			goto error;
		}
		if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
			if (net_msg.msg_head.ack_flag == ACK_ERROR_NAME) {
				iRet = ACK_ERROR_NAME;
				LOGE("GetMsg MSG_LOGIN == ACK_ERROR_NAME");
				goto error;
			} else if (net_msg.msg_head.ack_flag == ACK_ERROR_PWD) {
				iRet = ACK_ERROR_PWD;
				LOGE("GetMsg MSG_LOGIN == ACK_ERROR_PWD");
				goto error;
			} else {
				iRet = ACK_ERROR_OTHER;
				LOGE("GetMsg MSG_LOGIN = ACK_ERROR_OTHER");
				goto error;
			}
		}
		memcpy(&recv_login, net_msg.msg_data, sizeof(msg_login_t));
		pConnect->device_type = recv_login.device_type;
		pConnect->user_right = recv_login.remote_right;
	} else {
		iRet = ACK_ERROR_OTHER;
		LOGE("semTrywait MSG_LOGIN timeout");
		goto error;
	}

	/******************************请求登录********************************/

	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_SESSION_ID],
	OVER_TIME) == 0) {
		if (GetMsg(pConnect, INDEX_MSG_SESSION_ID, &net_msg) < 0) {
			iRet = ACK_ERROR_OTHER;
			LOGE("GetMsg MSG_SESSION_ID = ACK_ERROR_OTHER");
			goto error;
		}
		if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
			iRet = ACK_ERROR_OTHER;
			LOGE("GetMsg MSG_SESSION_ID = ACK_SUCCESS");
			goto error;
		}
		memcpy(&msg_session_id, net_msg.msg_data, sizeof(msg_session_id_t));
		pConnect->session_id = msg_session_id.session_id;
		pServerInfo->server_connect = pConnect;
	} else {
		LOGE("semTrywait MSG_SESSION_ID timeout");
		iRet = ACK_ERROR_OTHER;
		goto error;
	}

	pConnect->interval_valid[INDEX_MSG_HEARTBEAT] = TRUE;
	iRet = SendNetMsg(pConnect->msg_sock, MSG_HEARTBEAT, 0, NULL, 0);
	if (iRet != 0) {
		LOGE("SendNetMsg MSG_HEARTBEAT error");
		iRet = ACK_ERROR_OTHER;
		goto error;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		iRet = ACK_ERROR_OTHER;
		LOGE("GetMsg MSG_HEARTBEAT != ACK_SUCCESS");
		goto error;
	}
	pServerInfo->server_connect = pConnect;
	pServerInfo->iLogin = 1;

	//********** start heartbeat_thread **************
	pServerInfo->iMsgStatus = 1;

	//add by fengzx 2011-07-05
	if (net_msg.msg_head.msg_subtype == FLAG_PORT_OFFSET) {
		pConnect->bPortOffset = FALSE;
	}
	//end add
	// 定制告警信息   Date: 2014-4-1 Author: yms
	msg_alarm_control_t msg_alarm_control;
	msg_alarm_control.status = 1;

	LOGI("alarmControl status = 1");
	if (st_net_alarmControl(user_id, msg_alarm_control) < 0) {
		LOGE("st_net_alarmControl failed");
		goto error;
	}

	// 注册告警回调函数   Date: 2014-4-1 Author: yms
	st_net_registerServerAlarmInfoCallback(user_id, alarmInfoCallback, NULL);

	pthread_create(&pConnect->heartbeat_thread_id, NULL, HeartBeatThread,
			(void*) pServerInfo);

	pthread_create(&pConnect->reconnect_thread_id, NULL, ReconnectThread,
			(void*) pServerInfo);

	return 0;

	error: if (pConnect != NULL) {
		pthread_mutex_destroy(&pConnect->tcpmutex_id);
		for (i = 0; i < NETLIB_MAX_MSG_NUM; i++)
			st_comm_semDestroy(&pConnect->sem_id[i]);

		if (pConnect->msg_flag == 1) {
			pConnect->msg_flag = 0;
			pServerInfo->iMsgStatus = 0;
			usleep(100 * 1000);
			if (PROTOCOL_TCP == g_protocol)
				close(pConnect->msg_sock);
			else
				udtc_close(pConnect->msg_sock);

			LOGE("Destroy All Login thread!");
		}
		//remove at 2015-2-13 , for pjproject instead of libnice.

		free(pConnect);
		pConnect = NULL;
		pServerInfo->server_connect = NULL;
	} else if (PROTOCOL_TCP == g_protocol)
		close(msg_sock);
	else {
		udtc_close(msg_sock);
	}

	return iRet;
}

int st_net_userLogout(unsigned long user_id, int really_logout) {
	LOGI("---->userLogout!");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL) {
		LOGE("Logout_pServerInfo == NULL");
		return -1;
	}
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pConnect == NULL) {
		LOGE("Logout_pConnect == NULL");
		return -1;
	}
	pthread_mutex_lock(&LogoutMutex);

//	********** stop msg_thread ************
	pConnect->msg_flag = 0;
//	pthread_cancel(pConnect->msg_thread_id);

//  ********** stop heartbeat_thread ************
	pServerInfo->iMsgStatus = 0;
//  pthread_cancel(pConnect->heartbeat_thread_id);

	int i = 0;
	while (i < NETLIB_MAX_CHANNEL_NUM) {
		//********** stop preview_thread ************
		pConnect->channel_info[i].preview_flag = 0;
		i++;
	}

	LOGI("send logout msg to pu");
	SendNetMsg(pConnect->msg_sock, MSG_LOGOUT, 0, NULL, 0); //不需要回馈

	//等待其他线程退出
	usleep(100 * 1000);
//    pthread_join(pConnect->heartbeat_thread_id, NULL);
//    pthread_join(pConnect->msg_thread_id, NULL);

	pthread_mutex_destroy(&pConnect->tcpmutex_id);

	if (pConnect->msg_sock != -1) {
		if (PROTOCOL_TCP == g_protocol)
			close(pConnect->msg_sock);
		else
			udtc_close(pConnect->msg_sock);
		pConnect->msg_sock = -1;
	}

	// assert if the logout is called by Reconnect  LinZh107
	//remove at 2015-2-13 , for pjproject instead of libnice.
//	if(pConnect->p2pParam)
//	{
//		my_nice_free(pConnect->p2pParam);
//		free(pConnect->p2pParam);
//		pConnect->p2pParam = NULL;
//	}

	if (really_logout) {
		for (i = 0; i < NETLIB_MAX_MSG_NUM; i++) {
			st_comm_semDestroy(&pConnect->sem_id[i]);
		}

		free(pConnect);
		pServerInfo->server_connect = NULL;
	}

	pServerInfo->iLogin = 0;

	pthread_mutex_unlock(&LogoutMutex);

	LOGI("---->end userLogout!");
	return 0;
}

int st_net_getUserRight(unsigned long user_id) {
	tp_server_info_t *pServerInfo = NULL;
	if ((pServerInfo = (tp_server_info_t*) user_id) == NULL
			|| pServerInfo->server_connect == NULL)
		return -1;
	return (pServerInfo->server_connect->user_right);
}

int st_net_openChannelStream(long user_id, int channel, int protocol_type,
		int channel_type) {
	LOGI("---->openChannelStream!");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pConnect == NULL)
		return -1;

	int iLen = 0;
	int TimeOut = OVER_TIME;

	net_msg_t net_msg;
	msg_open_channel_t msg_open_channel;
	msg_preview_t msg_preview;
	msg_session_id_t msg_session_id;

	struct sockaddr_in remote_addr;
	struct timeval tval;
	fd_set rset;

	memset(&msg_open_channel, 0, sizeof(msg_open_channel_t));
	LOGI("openChannelStream protocol_type = %d", protocol_type);

	//remove at 2015-2-13 , for pjproject instead of libnice.
	//pConnect->channel_info[channel].p2pParam = NULL;
	pConnect->channel_info[channel].protocol_type = protocol_type;
	pConnect->channel_info[channel].record_flag = FALSE;
	pConnect->channel_info[channel].bIflag = FALSE;
	pConnect->channel_info[channel].msg_sock = pConnect->msg_sock;
	pConnect->channel_info[channel].bit_rate = 0;
	pConnect->channel_info[channel].bBitRate = FALSE;
	pConnect->channel_info[channel].frontsecond = 0;
	pConnect->channel_info[channel].tmp_bit_rate = 0;
	pConnect->channel_info[channel].user_id = user_id;

	//add by LinZh107  一下都是断线重连的维护所需参数 2014-10-10
	pConnect->channel_info[channel].channel_type = channel_type;
	pConnect->channel = channel;

	net_msg.msg_head.msg_type = TCP_TYPE_PREVIEW;
	net_msg.msg_head.msg_subtype = 0;
	msg_open_channel.channel_no = channel;
	msg_open_channel.protocol_type = 0;
	msg_open_channel.channel_type = channel_type;
	memcpy(net_msg.msg_data, &msg_open_channel, sizeof(msg_open_channel_t));
	//LOGI("openChannel = %d",channel);

	//TCP | UDT 协议通信
	if (PROTOCOL_TCP == g_protocol) {
		if ((pConnect->channel_info[channel].sock = socket(AF_INET, SOCK_STREAM,
				0)) == -1)
			return -1;
		iLen = DATA_BUF_LEN;
		if (setsockopt(pConnect->channel_info[channel].sock, SOL_SOCKET,
				SO_RCVBUF, (void*) &iLen, sizeof(iLen)) == -1) {
			LOGE("err2 OpenchannelStream PROTOCOL_TCP");
			goto error;
		}

		/*7.14号*/
		char ipaddr1[16];
		if (inet_addr((pConnect->ip_addr)) == INADDR_NONE) {
			char * strIP;
			struct hostent * hp;
			if ((hp = gethostbyname(pConnect->ip_addr)) != NULL) //根据网络域名ip获取对应的IP地址
			{
				strIP = inet_ntoa(*(struct in_addr *) hp->h_addr_list[0]);
				if (strlen(strIP) == 0) {
					strIP = "Not available";
				}
				//my_log(strIP);
				sprintf(ipaddr1, "%s", strIP);
			} else
				return -1;
		} else {
			strcpy(ipaddr1, pConnect->ip_addr);
		}
		/*7.14号*/

		remote_addr.sin_family = AF_INET;
		remote_addr.sin_addr.s_addr = inet_addr(ipaddr1);
		//add by fengzx 2011-07-05
		int nPortOffset = 0;
		if (pConnect->bPortOffset) {
			nPortOffset = PORT_OFFSET_TCP;
		}
		remote_addr.sin_port = htons(pConnect->port + nPortOffset);
		if (connect_nonb(pConnect->channel_info[channel].sock,
				(struct sockaddr*) &remote_addr, sizeof(remote_addr), TimeOut)
				!= 0) {
			LOGE("Not Connect Enc Close Socket");
			goto error;
		}
	}
	// st_net_openChannelStream
	else if (PROTOCOL_UDT == g_protocol) {
		if ((pConnect->channel_info[channel].sock = udtc_socket(AF_INET,
				SOCK_STREAM, 0)) == -1)
			return (-1);
		iLen = DATA_BUF_LEN;
		if (udtc_setsockopt(pConnect->channel_info[channel].sock, SOL_SOCKET, /*SO_RCVBUF*/
		6, (void*) &iLen, sizeof(iLen)) == -1) {
			LOGE("err2 OpenchannelStream PROTOCOL_UDT");
			goto error;
		}

		/*7.14号*/
		char ipaddr1[16];
		if (inet_addr((pConnect->ip_addr)) == INADDR_NONE) {
			char * strIP;
			struct hostent * hp;
			if ((hp = gethostbyname(pConnect->ip_addr)) != NULL) //根据网络域名ip获取对应的IP地址
			{
				strIP = inet_ntoa(*(struct in_addr *) hp->h_addr_list[0]);
				if (strlen(strIP) == 0) {
					strIP = "Not available";
				}
				//my_log(strIP);
				sprintf(ipaddr1, "%s", strIP);
			} else
				return -1;
		} else {
			strcpy(ipaddr1, pConnect->ip_addr);
		}
		/*7.14号*/

		remote_addr.sin_family = AF_INET;
		remote_addr.sin_addr.s_addr = inet_addr(ipaddr1);
		//add by fengzx 2011-07-05
		int nPortOffset = 0;
		if (pConnect->bPortOffset) {
			nPortOffset = PORT_OFFSET_TCP;
		}
		remote_addr.sin_port = htons(pConnect->port + nPortOffset);
		if (0
				!= udtc_connect(pConnect->channel_info[channel].sock,
						(struct sockaddr*) &remote_addr, sizeof(remote_addr))) {
			LOGE("Not Connect Enc Close Socket");
			goto error;
		}
		LOGI("Ok to Connect Enc Close Socket");
	}
	// P2P 协议通信
	else if (PROTOCOL_P2P == g_protocol) {
		int msg_sock = tp_pjice_getsock(user_id);
		if (msg_sock <= 0) {
			LOGE("PROTOCOL_P2P OpenChannel failed.");
			return -1;
		}
		pConnect->channel_info[channel].sock = msg_sock;
	}

	msg_open_channel.channel_no = channel;
	msg_open_channel.protocol_type = protocol_type;
	msg_open_channel.channel_type = channel_type;
	pConnect->interval_valid[INDEX_MSG_OPEN_CHANNEL] = TRUE;

	SendNetMsg(pConnect->msg_sock, MSG_OPEN_CHANNEL, 0,
			(char *) &msg_open_channel, sizeof(msg_open_channel_t));

	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_OPEN_CHANNEL],
	OVER_TIME) == 0) //modify by LinZh107
			{
		if (GetMsg(pConnect, INDEX_MSG_OPEN_CHANNEL, &net_msg) < 0) {
			LOGE("GetMsg MSG_OPEN_CHANNEL error!");
			goto error;
		}
		if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
			LOGE("GetMsg MSG_OPEN_CHANNEL ack_flag != ACK_SUCCESS");
			goto error;
		}
		pConnect->channel_info[channel].channel_no = channel;
	} else {
		LOGE("semTrywait MSG_OPEN_CHANNEL timeout");
		goto error;
	}

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(pConnect->channel_info[channel].sock, MSG_SESSION_ID, 0,
			(char *) &msg_session_id, sizeof(msg_session_id_t));

	int recvlen = 0;
	if (PROTOCOL_TCP == g_protocol) {
		FD_ZERO(&rset);
		FD_SET((unsigned int) pConnect->channel_info[channel].sock, &rset);

		tval.tv_sec = 4;
		tval.tv_usec = 0;
		//	LOGI("in -->Open_channel select");
		if (select(pConnect->channel_info[channel].sock + 1, &rset, 0, 0, &tval)
				> 0)
			recvlen = readn(pConnect->channel_info[channel].sock,
					(char *) &net_msg, sizeof(msg_head_t));
	} else
		//UDT  P2P
		recvlen = udtc_recvn(pConnect->channel_info[channel].sock,
				(char *) &net_msg, sizeof(msg_head_t));

	if (recvlen != sizeof(msg_head_t)) {
		LOGE("OpenChannelStream receive MSG_SESSION_ID error");
		goto error;
	}
	net_msg.msg_head.msg_type = TCP_TYPE_PREVIEW;
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_size = sizeof(msg_preview_t);
	msg_preview.channel = channel;
	msg_preview.channel_type = channel_type;
	memcpy(net_msg.msg_data, &msg_preview, sizeof(msg_preview_t));

	if (PROTOCOL_TCP == g_protocol) {
		FD_ZERO(&rset);
		FD_SET((unsigned int) pConnect->channel_info[channel].sock, &rset);

		tval.tv_sec = 1;
		tval.tv_usec = 0;
		//	LOGI("in -->Open_channel select");
		if (select(pConnect->channel_info[channel].sock + 1, 0, &rset, 0, &tval)
				> 0)
			recvlen = writen(pConnect->channel_info[channel].sock,
					(char *) &net_msg,
					sizeof(msg_head_t) + sizeof(msg_preview_t));
	} else
		recvlen = udtc_sendn(pConnect->channel_info[channel].sock,
				(char *) &net_msg, sizeof(msg_head_t) + sizeof(msg_preview_t));

	if (recvlen != sizeof(msg_head_t) + sizeof(msg_preview_t)) {
		LOGE("OpenChannelStream send TCP_TYPE_PREVIEW error ");
		goto error;
	}

	if (PROTOCOL_TCP == g_protocol) {
		FD_ZERO(&rset);
		FD_SET((unsigned int) pConnect->channel_info[channel].sock, &rset);
		tval.tv_sec = 4;
		tval.tv_usec = 0;
		if (select(pConnect->channel_info[channel].sock + 1, &rset, 0, 0, &tval)
				> 0)
			recvlen = readn(pConnect->channel_info[channel].sock,
					(char *) &net_msg, sizeof(msg_head_t));
	} else
		recvlen = udtc_recvn(pConnect->channel_info[channel].sock,
				(char *) &net_msg, sizeof(msg_head_t));
	if (recvlen != sizeof(msg_head_t)) {
		LOGE("openChannelStream receive TCP_TYPE_PREVIEW error1");
		goto error;
	}

	if (PROTOCOL_TCP == g_protocol) {
		recvlen = readn(pConnect->channel_info[channel].sock,
				(char *) &net_msg + sizeof(msg_head_t),
				net_msg.msg_head.msg_size);
	} else
		recvlen = udtc_recvn(pConnect->channel_info[channel].sock,
				(char *) &net_msg + sizeof(msg_head_t),
				net_msg.msg_head.msg_size);
	if (recvlen != net_msg.msg_head.msg_size) {
		LOGE("openChannelStream receive TCP_TYPE_PREVIEW error2");
		goto error;
	}
	LOGI("openChannelStream receive TCP_TYPE_PREVIEW OK");

	pConnect->channel_info[channel].channel_no = channel;
	pServerInfo->ichannelStatus[channel] = 1;

	//********** start preview_thread **************
	pConnect->channel_info[channel].preview_flag = 1;

	pthread_create(&pConnect->channel_info[channel].preview_thread_id, NULL,
			TcpReceiveThread, &(pConnect->channel_info[channel]));

	return 0;

	error: if (pConnect->channel_info[channel].sock > 0) {
		if (PROTOCOL_TCP == g_protocol)
			close(pConnect->channel_info[channel].sock);
		else
			udtc_close(pConnect->channel_info[channel].sock);

		LOGE("openChannelStream error --> close");

		//remove at 2015-2-13 , for pjproject instead of libnice.
//		if(PROTOCOL_P2P == g_protocol)
//			if(pConnect->channel_info[channel].p2pParam)
//			{
//				my_nice_free(pConnect->channel_info[channel].p2pParam);
//				free(pConnect->channel_info[channel].p2pParam);
//				pConnect->channel_info[channel].p2pParam = NULL;
//			}
	}

	pConnect->channel_info[channel].sock = -1;
	pConnect->interval_valid[INDEX_MSG_OPEN_CHANNEL] = FALSE;
	return (-1);
} //end st_net_openChannelStream()

int st_net_closeChannelStream(unsigned long user_id, int channel,
		int channel_type) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (!pServerInfo)
		return -1;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pthread_mutex_lock(&CloseChannelMutex);
	int msg_sock = pConnect->msg_sock;

	msg_close_channel_t msg_close_channel;
	memset(&msg_close_channel, 0, sizeof(msg_close_channel));

	msg_close_channel.channel_no = channel;
	msg_close_channel.channel_type = channel_type;

	//********** stop preview_thread **************
	pConnect->channel_info[channel].preview_flag = 0;
//	usleep(100*1000);

//	pthread_cancel(pConnect->channel_info[channel].preview_thread_id);
//	pthread_join(pConnect->channel_info[channel].preview_thread_id, NULL);

	pServerInfo->ichannelStatus[channel] = 0;

	SendNetMsg(msg_sock, MSG_CLOSE_CHANNEL, channel, (char*) &msg_close_channel,
			sizeof(msg_close_channel_t));

	if (pConnect->channel_info[channel].sock > 0) {
		if (PROTOCOL_TCP == g_protocol)
			close(pConnect->channel_info[channel].sock);
		else
			udtc_close(pConnect->channel_info[channel].sock);

//		//remove at 2015-2-13 , for pjproject instead of libnice.
//		if(pConnect->channel_info[channel].p2pParam)
//		{
//			my_nice_free(pConnect->channel_info[channel].p2pParam);
//			free(pConnect->channel_info[channel].p2pParam);
//			pConnect->channel_info[channel].p2pParam = NULL;
//		}
		pConnect->channel_info[channel].sock = -1;
	}

	pthread_mutex_unlock(&CloseChannelMutex);

	LOGI("---->closeChannelStream succeed");
	return 0;
} //end st_net_closeChannelStream()

int st_net_startChannelStream(unsigned long user_id, int channel,
		int stream_type, int channel_type) {
	LOGI("----->startChannelStream!");
	int msg_sock;
	msg_send_data_t msg_send_data;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;

	//add by LinZh107  一下都是断线重连的维护所需参数 2014-10-10
	pConnect->channel_info[channel].stream_type = stream_type;

	if (!pConnect)
		return -1;
	msg_sock = pConnect->msg_sock;
	msg_send_data.channel = channel;
	msg_send_data.data_type = stream_type;
	msg_send_data.channel_type = channel_type;

	if (SendNetMsg(msg_sock, MSG_START_SEND_DATA, 0, (char*) &msg_send_data,
			sizeof(msg_send_data_t)) == -1) {
		LOGE("send MSG_START_SEND_DATA  fail");
		return -1;
	} else
		pConnect->channel_info[channel].recv_flag = 1;
//		LOGI("st_net_startChannelStream ...");
	return 0;
}

int st_net_stopChannelStream(unsigned long user_id, int channel,
		int stream_type, int channel_type) {
	LOGI("---->stopChannelStream!");
	int msg_sock;
	int iRet;
	msg_send_data_t msg_send_data;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (!pServerInfo) {
		LOGE("st_net_stopChannelStream pServerInfo = NULL");
		return -1;
	}
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect) {
		LOGE("st_net_stopChannelStream pConnect = NULL");
		return -1;
	}
	msg_sock = pConnect->msg_sock;
	msg_send_data.channel = channel;
	msg_send_data.data_type = stream_type;
	msg_send_data.channel_type = channel_type;
	if ((iRet = SendNetMsg(msg_sock, MSG_STOP_SEND_DATA, 0,
			(char*) &msg_send_data, sizeof(msg_send_data_t))) == -1)
		return -1;
	else if (pConnect)
		pConnect->channel_info[channel].recv_flag = 0;
	return 0;
}

int st_net_forceKeyFrame(unsigned long user_id, int channel) {
	int msg_sock;
	msg_force_keyframe_t msg_force_keyframe;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	msg_sock = pConnect->msg_sock;
	msg_force_keyframe.channel = channel;
	return SendNetMsg(msg_sock, MSG_FORCE_KEYFRAME, 0,
			(char *) &msg_force_keyframe, sizeof(msg_force_keyframe_t));
}
int st_net_sendMsgToServer(unsigned long user_id, int msg_type, int msg_subtype,
		void *buf, int len) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL || pServerInfo->server_connect == NULL)
		return -1;
	return SendNetMsg(pServerInfo->server_connect->msg_sock, msg_type,
			msg_subtype, (char*) buf, len);
}

//注册回调.
int st_net_registerChannelStreamCallback(unsigned long user_id, int channel,
		CHANNEL_STREAM_CALLBACK data_callback, void *context) {
	LOGI("user_id____:%d", user_id);
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pConnect == NULL)
		return -1;
	pConnect->channel_info[channel].callback.channel = channel;
	pConnect->channel_info[channel].callback.channel_callback = data_callback;
	pConnect->channel_info[channel].callback.context = context;
	return 0;
}
int st_net_getChannelStatus(unsigned long user_id, int ichannel) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	return pServerInfo->ichannelStatus[ichannel];
}

int st_net_getMsgStatus(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	return pServerInfo->iMsgStatus;
}

int st_net_clearMsgStatus(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	pServerInfo->iMsgStatus = 0;
	return pServerInfo->iMsgStatus;
}

int st_net_ptzControl(unsigned long user_id, int type, msg_ptz_control_t info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SetServInfo(user_id, MSG_PTZ_CONTROL, type, INDEX_MSG_PTZ_CONTROL,
			(void*) &info, sizeof(msg_ptz_control_t));
}

int st_net_GetIPAddress(char *name, char *ip) {
	char **pptr;
	char str[16] = { 0 };
	struct hostent *hptr;
	int ret = 0;

	if ((hptr = gethostbyname(name)) == NULL) {
		LOGE("gethostbyname error for host %s", name);
		return -1;
	}

	LOGI("official hostname %s ", hptr->h_name);

	switch (hptr->h_addrtype) {
	case AF_INET:
		pptr = hptr->h_addr_list;
		for (; *pptr != NULL; pptr++) {
			inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));

			strcpy(ip, str);
			LOGI("taddress: %s", ip);
		}
		break;
	default:
		LOGE("unknown address type");
		ret = -1;
		break;
	}
	return ret;
}
int st_net_modifyServerInfo(unsigned long user_id, char *ip_addr, int port,
		char *user_name, char *user_pwd) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (ip_addr != NULL) {
		strcpy(pServerInfo->ip_addr, ip_addr);
	}
	if (port != 0) {
		pServerInfo->port = port;
	}
	if (user_name != NULL) {
		strcpy(pServerInfo->user_name, user_name);
	}
	if (user_pwd != NULL) {
		strcpy(pServerInfo->user_pwd, user_pwd);
	}
	LOGI("rebuit_pServerInfo->ip_addr==%s", pServerInfo->ip_addr);
	return 0;
}

//语音对讲
static void *DualRevThread(void *param) {
	LOGI(" ");
	LOGI("start DualRevThread !");

	pthread_detach(pthread_self()); //线程分离.

	dual_talk_t *dual_talk = (dual_talk_t*) param;
	if (dual_talk == NULL)
		return -1;

	int CntTimeout = 50 * 1000;
	//************* get/set socket opction **************** LinZh107
	if (PROTOCOL_TCP != g_protocol) {
		int rcvTimeout = CntTimeout / 1000; //100 msec
		int iRet = udtc_setsockopt(dual_talk->talk_sock, 0, UDT_RCVTIMEO,
				(char*) &rcvTimeout, sizeof(int));
		LOGI("setsockopt return %d", iRet);
	}
	//************* end **************** LinZh107

	net_pack_t net_pack;
	int tmpLen = 0;
	struct timeval timeout;
	fd_set rset1;
	fd_set rset2;

	while (dual_talk->iDualRevFalg) {
		//******* /下面开始接收有效帧头
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&rset1);
			FD_SET((unsigned int) dual_talk->talk_sock, &rset1);
			timeout.tv_sec = 0;
			timeout.tv_usec = CntTimeout; //100 ms
			if (select(dual_talk->talk_sock + 1, &rset1, NULL, NULL, &timeout)
					<= 0)
				continue;
			tmpLen = readn(dual_talk->talk_sock, (char*) &net_pack,
					sizeof(pack_head_t));
		} else
			tmpLen = udtc_recvn(dual_talk->talk_sock, (char*) &net_pack,
					sizeof(pack_head_t));

		if (tmpLen != sizeof(pack_head_t))
			continue;

		//******* /下面开始接收有效帧数据
		if (PROTOCOL_TCP == g_protocol) {
			FD_ZERO(&rset2);
			FD_SET((unsigned int) dual_talk->talk_sock, &rset2);
			timeout.tv_sec = 0;
			timeout.tv_usec = CntTimeout; //100ms
			if (select(dual_talk->talk_sock + 1, &rset2, NULL, NULL, &timeout)
					<= 0)
				continue;
			tmpLen = readn(dual_talk->talk_sock,
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
		} else
			tmpLen = udtc_recvn(dual_talk->talk_sock,
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);

		//LOGI("DualRecvThread out talk_callback!");
		if (tmpLen == net_pack.pack_head.pack_size) {
			if (dual_talk->talk_callback != NULL) {
				//LOGI("DualRecvThread in talk_callback!");
				dual_talk->talk_callback(NULL,
						(char*) &net_pack + sizeof(pack_head_t),
						net_pack.pack_head.pack_size, dual_talk->context);
			}
		}
	} //end while (dual_talk->iDualRevFalg)

	if (PROTOCOL_TCP == g_protocol)
		close(dual_talk->talk_sock);
	else
		udtc_close(dual_talk->talk_sock);
	LOGE("Exit---->DualRevThread");
	dual_talk->talk_sock = -1;
	return NULL;
}

//INDEX_MSG_START_TALK
int st_net_startDualTalk(unsigned long user_id) {
	LOGI(" ");
	LOGI("---->startDualTalk!");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL)
		return -1;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (NULL == pConnect)
		return -1;

	int talk_sock = -1;
	int tmpLen = 0;
	int recvLen = 0;
	int sendLen = 0;
	int TimeOut = 6000;

	net_msg_t net_msg;
	memset(&net_msg, 0, sizeof(net_msg_t));
	net_msg.msg_head.msg_type = TCP_TYPE_START_DUAL_TALK;
	net_msg.msg_head.msg_size = 0;

	pConnect->dual_talk.talk_sock = -1;

	struct sockaddr_in remote_talk_addr;
	memset(&remote_talk_addr, 0, sizeof(remote_talk_addr));

	remote_talk_addr.sin_family = PF_INET;
	remote_talk_addr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);

	//add by fengzx 2011-07-05
	int nPortOffset = 0; // 0==>PORT_OFFSET_TALK
	if (pConnect->bPortOffset) {
		LOGE("startDualTalk set offset!");
		nPortOffset = PORT_OFFSET_TALK;
	}
	remote_talk_addr.sin_port = htons(pConnect->port + nPortOffset);
	//end add

	//add  UDT and P2P  by LinZh107 2014-9-15 
	//************** TCP 协议通信 *************
	if (PROTOCOL_TCP == g_protocol) {
		talk_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (talk_sock <= 0)
			return -2;

		tmpLen = connect_nonb(talk_sock, (struct sockaddr *) &remote_talk_addr,
				sizeof(struct sockaddr), TimeOut);
		if (tmpLen != 0) {
			close(talk_sock);
			talk_sock = -1;
			return -3;
		}
	}
	//*************** UDT 协议通信 ******************
	else if (PROTOCOL_UDT == g_protocol) {
		talk_sock = udtc_socket(PF_INET, SOCK_STREAM, 0);
		if (talk_sock <= 0)
			return -2;

		tmpLen = udtc_connect(talk_sock, (struct sockaddr *) &remote_talk_addr,
				sizeof(struct sockaddr));
		if (tmpLen != 0) {
			udtc_close(talk_sock);
			talk_sock = -1;
			return -3;
		}
	}
	//P2P 协议通信
	else if (PROTOCOL_P2P == g_protocol) {
#if 0
		talk_sock = p2p_getSocket(pServerInfo->pu_id);
		//**************** p2p_方案更换 ****************begin
//	#else //remove at 2015-2-13 , for pjproject instead of libnice.
		pConnect->p2pParam = malloc(sizeof(p2p_thorough_param_t));
		memset(pConnect->p2pParam, 0, sizeof(p2p_thorough_param_t));
		BOOL ret = my_nice_create (pConnect->p2pParam);
		LOGI("Exit---->P2pCrossThread %d",ret);

		talk_sock = pConnect->p2pParam->niceSocket;
		if (talk_sock <= 0 )
		{
			LOGE("PROTOCOL_P2P startDualTalk error");
			return -3;
		}
#else
		talk_sock = tp_pjice_getsock(user_id);
		if (talk_sock <= 0) {
			LOGE("PROTOCOL_P2P startDualTalk failed.");
			return -3;
		}

#endif
		//**************** p2p_方案更换 ****************end
	}

	if (PROTOCOL_TCP == g_protocol) {
		//发送请求
		sendLen = writen(talk_sock, (char *) &net_msg, sizeof(msg_head_t));
		if (sendLen != sizeof(msg_head_t)) {
			close(talk_sock);
			talk_sock = -1;
			return -4;
		}

		struct timeval tv1;
		fd_set rset1;
		FD_ZERO(&rset1);
		FD_SET((unsigned int) talk_sock, &rset1);
		tv1.tv_sec = 4;
		tv1.tv_usec = 0;
		if (select(talk_sock + 1, &rset1, NULL, NULL, &tv1) <= 0)
			return -5;

		recvLen = readn(talk_sock, (char *) &net_msg, sizeof(msg_head_t));
		if (recvLen != sizeof(msg_head_t)) {
			LOGE("start dualTalk ack error recvLen=%d", recvLen);
			close(talk_sock);
			talk_sock = -1;
			return -6;
		}
		if (net_msg.msg_head.ack_flag == 1) {
			close(talk_sock);
			talk_sock = -1;
			return -7;
		} else if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
			close(talk_sock);
			talk_sock = -1;
			return -8;
		}
	} // end if (PROTOCOL_TCP == g_protocol)
	else {
		sendLen = udtc_sendn(talk_sock, (char *) &net_msg, sizeof(msg_head_t));
		if (sendLen != sizeof(msg_head_t)) {
			udtc_close(talk_sock);
			talk_sock = -1;
			return -4;
		}
		//接收应答
		recvLen = udtc_recvn(talk_sock, (char *) &net_msg, sizeof(msg_head_t));
		if (recvLen != sizeof(msg_head_t)) {
			udtc_close(talk_sock);
			talk_sock = -1;
			return -5;
		}
		if (net_msg.msg_head.ack_flag == 1) {
			udtc_close(talk_sock);
			talk_sock = -1;
			return -6;
		} else if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
			udtc_close(talk_sock);
			talk_sock = -1;
			return -7;
		}
	}

	pConnect->dual_talk.talk_sock = talk_sock;
	pConnect->dual_talk.context = NULL;
//	pConnect->dual_talk.talk_callback = NULL; //已经在打开视频通道的时候注册了
	pConnect->dual_talk.iDualRevFalg = 1;
	pthread_create(&pConnect->dual_talk.TalkThread_id, NULL, DualRevThread,
			(void*) &pConnect->dual_talk);
	return 0;
}

int st_net_stopDualTalk(unsigned long user_id) {
	LOGI("---->stopDualTalk!");
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL)
		return -1;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pConnect == NULL)
		return -1;

	net_msg_t net_msg;
	memset(&net_msg, 0, sizeof(net_msg_t));
	net_msg.msg_head.msg_type = TCP_TYPE_STOP_DUAL_TALK;
	net_msg.msg_head.msg_size = 0;

	pConnect->dual_talk.iDualRevFalg = 0;

//	pthread_cancel(pConnect->dual_talk.TalkThread_id);
//	pthread_join(&pConnect->dual_talk.TalkThread_id, NULL);
	pConnect->dual_talk.TalkThread_id = 0;

	int sendLen = 0;
	if (PROTOCOL_TCP == g_protocol) {
		sendLen = writen(pConnect->dual_talk.talk_sock, (char *) &net_msg,
				sizeof(msg_head_t));
		close(pConnect->dual_talk.talk_sock);
	} else {
		sendLen = udtc_sendn(pConnect->dual_talk.talk_sock, (char *) &net_msg,
				sizeof(msg_head_t));
		udtc_close(pConnect->dual_talk.talk_sock);
	}

	pConnect->dual_talk.talk_sock = -1;

	if (sendLen == sizeof(msg_head_t))
		return 0;
	else
		return -1;

}

int st_net_sendAudioData(unsigned long user_id, char *buf, int len) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo == NULL)
		return -1;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (pConnect == NULL)
		return -2;
	if (pConnect->dual_talk.talk_sock < 0)
		return -3;
	int sendLen = 0;
	while (sendLen == 0) {
		if (PROTOCOL_TCP == g_protocol) {
			struct timeval tval1;
			fd_set wset1;
			FD_ZERO(&wset1);
			FD_SET((unsigned int) pConnect->dual_talk.talk_sock, &wset1);
			tval1.tv_sec = 0;
			tval1.tv_usec = 100 * 1000;
			if (select(pConnect->dual_talk.talk_sock + 1, NULL, &wset1, NULL,
					&tval1) > 0) {
				//LOGI("Writen Audiodata to talk_socket len=%d",len);
				sendLen = writen(pConnect->dual_talk.talk_sock, buf, len);
			} else
				LOGE("Writen Audiodata to talk_socket error");
		} else
			sendLen = udtc_sendn(pConnect->dual_talk.talk_sock, buf, len);
	}
	return sendLen;
}

int st_net_getVersionInfo(unsigned long user_id, msg_version_info_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return GetServInfo(user_id, MSG_VERSION_INFO, 0, INDEX_MSG_VERSION_INFO,
			(void*) info, sizeof(msg_version_info_t));
}
int st_net_getSystemStatus(unsigned long user_id, msg_system_status_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return GetServInfo(user_id, MSG_SYSTEM_STATUS, 0, INDEX_MSG_SYSTEM_STATUS,
			(void*) info, sizeof(msg_system_status_t));
}

int st_net_getSystemTime(unsigned long user_id, msg_time_t *tm) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return GetServInfo(user_id, MSG_GET_TIME, 0, INDEX_MSG_GET_TIME_INFO, tm,
			sizeof(msg_time_t));
}

int st_net_setSystemTime(unsigned long user_id, msg_time_t tm) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_SET_TIME, 0, INDEX_MSG_SET_TIME_INFO,
			(void*) &tm, sizeof(msg_time_t));
}

int st_net_remote_start_hand_record(unsigned long user_id,
		msg_remote_hand_record_t msg_remote_hand_record) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_REMOTE_START_HAND_RECORD, 0,
			INDEX_MSG_REMOTE_START_HAND_RECORD, (void*) &msg_remote_hand_record,
			sizeof(msg_remote_hand_record_t));
}

int st_net_remote_stop_hand_record(unsigned long user_id,
		msg_remote_hand_record_t msg_remote_hand_record) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_REMOTE_STOP_HAND_RECORD, 0,
			INDEX_MSG_REMOTE_STOP_HAND_RECORD, (void*) &msg_remote_hand_record,
			sizeof(msg_remote_hand_record_t));
}

int st_net_getParam(unsigned long user_id, int type, void *param, int size) {
	//LOGI("st_net_getParam");
	int iRet = 0;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;

	switch (type) {
	case PARAM_HANDIO_CONTROL:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HANDIO_CONTROL,
				INDEX_MSG_PARAM_HANDIO_CONTROL, param, size);
		break;
	case PARAM_TIMEIO_CONTROL:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TIMEIO_CONTROL,
				INDEX_MSG_PARAM_TIMEIO_CONTROL, param, size);
		break;
	case PARAM_AUTO_POWEROFF:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_AUTO_POWEROFF,
				INDEX_MSG_PARAM_AUTO_POWEROFF, param, size);
		break;
	case PARAM_AUTO_POWERON:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_AUTO_POWERON,
				INDEX_MSG_PARAM_AUTO_POWERON, param, size);
		break;
	case PARAM_USER_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_USER_INFO,
				INDEX_MSG_PARAM_USER_INFO, param, size);
		break;
	case PARAM_HAND_RECORD:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HAND_RECORD,
				INDEX_MSG_PARAM_HAND_RECORD, param, size);
		break;
	case PARAM_NETWORK_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_NETWORK_CONFIG,
				INDEX_MSG_PARAM_NETWORK_CONFIG, param, size);
		break;
	case PARAM_MISC_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_MISC_CONFIG,
				INDEX_MSG_PARAM_MISC_CONFIG, param, size);
		break;
	case PARAM_PROBER_ALRAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_PROBER_ALRAM,
				INDEX_MSG_PARAM_PROBER_ALRAM, param, size);
		break;
	case PARAM_RECORD_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_RECORD_PARAM,
				INDEX_MSG_PARAM_RECORD_PARAM, param, size);
		break;
	case PARAM_TERM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TERM_CONFIG,
				INDEX_MSG_PARAM_TERM_CONFIG, param, size);
		break;
	case PARAM_TIMER_RECORD:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TIMER_RECORD,
				INDEX_MSG_PARAM_TIMER_RECORD, param, size);
		break;
	case PARAM_VIDEO_LOSE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_VIDEO_LOSE,
				INDEX_MSG_PARAM_VIDEO_LOSE, param, size);
		break;
	case PARAM_VIDEO_MOVE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_VIDEO_MOVE,
				INDEX_MSG_PARAM_VIDEO_MOVE, param, size);
		break;
	case PARAM_VIDEO_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_VIDEO_CONFIG,
				INDEX_MSG_PARAM_VIDEO_CONFIG, param, size);
		break;
	case PARAM_VIDEO_ENCODE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_VIDEO_ENCODE,
				INDEX_MSG_PARAM_VIDEO_ENCODE, param, size);
		break;
	case PARAM_COM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_COM_CONFIG,
				INDEX_MSG_PARAM_COM_CONFIG, param, size);
		break;
	case PARAM_OSD_SET:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_OSD_SET,
				INDEX_MSG_PARAM_OSDSET, param, size);
		break;
	case PARAM_OVERLAY_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_OVERLAY_INFO,
				INDEX_MSG_PARAM_OVERLAY, param, size);
		break;
	case PARAM_MOTION_ZOOM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_MOTION_ZOOM,
				INDEX_MSG_PARAM_MOTION_ZOOM, param, size);
		break;
	case PARAM_PPPOE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_PPPOE,
				INDEX_MSG_PARAM_PPPOE, param, size);
		break;
	case PARAM_RECORD_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_RECORD_INFO,
				INDEX_MSG_PARAM_RECORD_INFO, param, size);
		break;
	case PARAM_WIRELESS_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_WIRELESS_CONFIG,
				INDEX_MSG_PARAM_WIRELESS_CONFIG, param, size);
		break;
	case PARAM_MAIL_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_MAIL_CONFIG,
				INDEX_MSG_PARAM_MAIL_CONFIG, param, size);
		break;
	case PARAM_FTP_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_FTP_CONFIG,
				INDEX_MSG_PARAM_FTP_CONFIG, param, size);
		break;
	case PARAM_SERIAL_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_SERIAL_INFO,
				INDEX_MSG_PARAM_SERIAL_INFO, param, size);
		break;
	case PARAM_TRANSPARENCE_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TRANSPARENCE_INFO,
				INDEX_MSG_PARAM_TRANSPARENCE_INFO, param, size);
		break;
	case PARAM_SLAVE_ENCODE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_SLAVE_ENCODE,
				INDEX_MSG_PARAM_SLAVE_INFO, param, size);
		break;
	case PARAM_DEVICE_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DEVICE_INFO,
				INDEX_MSG_PARAM_DEVICE_INFO, param, size);
		break;
	case PARAM_AUDIO_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_AUDIO_INFO,
				INDEX_MSG_PARAM_AUDIO_INFO, param, size);
		break;
	case PARAM_MEMBER_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_MEMBER_INFO,
				INDEX_MSG_PARAM_MEMBER_INFO, param, size);
		break;
	case PARAM_CAMERA_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CAMERA_INFO,
				INDEX_MSG_PARAM_CAMERA_INFO, param, size);
		break;
	case PARAM_NAS_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_NAS_INFO,
				INDEX_MSG_PARAM_NAS_INFO, param, size);
		break;
	case PARAM_ITU_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_ITU_INFO,
				INDEX_MSG_PARAM_ITU_INFO, param, size);
		break;
	case PARAM_TIMER_COLOR2GREY:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TIMER_COLOR2GREY,
				INDEX_MSG_PARAM_COLOR_INFO, param, size);
		break;
	case PARAM_NA208_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_NA208_INFO,
				INDEX_MSG_PARAM_NA208_INFO, param, size);
		break;
	case PARAM_MATRIX_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_MATRIX_INFO,
				INDEX_MSG_PAPAM_MATRIX_INFO, param, size);
		break;
	case PARAM_TIMER_CAPTURE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TIMER_CAPTURE,
				INDEX_MSG_TIMER_CAPTURE, param, size);
		break;
	case PARAM_TIMER_RESTART:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_TIMER_RESTART,
				INDEX_MSG_PARAM_TIMER_RESTART, param, size);
		break;
	case PARMA_PTZ_CURISE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARMA_PTZ_CURISE,
				INDEX_PARMA_PTZ_CURISE, param, size);
		break;
	case PARAM_USB_TYPE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_USB_TYPE,
				INDEX_PARAM_USB_TYPE, param, size);
		break;
	case PARAM_CDMA_TIME:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CDMA_TIME,
				INDEX_MSG_CDMA_TIME, param, size);
		break;

	case PARAM_CDMA_ALARM_ENABLE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CDMA_ALARM_ENABLE,
				INDEX_MSG_CDMA_ALARM_ENABLE, param, size);
		break;

	case PARAM_CDMA_NONEGO_ENABLE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CDMA_NONEGO_ENABLE,
				INDEX_MSG_CDMA_NONEGO_ENABLE, param, size);
		break;
	case PARAM_CDMA_STREAM_ENABLE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CDMA_STREAM_ENABLE,
				INDEX_MSG_CDMA_STREAM_ENABLE, param, size);
		break;
	case PARAM_CDMA_ALARM_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CDMA_ALARM_INFO,
				INDEX_MSG_CDMA_ALARM_INFO, param, size);
		break;

	case PARMA_IMMEDIATE_CAPTURE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARMA_IMMEDIATE_CAPTURE,
				INDEX_MSG_IMMEDIATE_CAPTURE, param, size);
		break;
	case PARAM_3511_SYSINFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_3511_SYSINFO,
				INDEX_3511_PARAM_SYSINFO, param, size);
		break;
	case PARAM_SZ_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_SZ_INFO,
				INDEX_MSG_PARAM_SZ_PARAM, param, size);
		break;
	case PARAM_ZXLW_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_ZXLW_PARAM,
				INDEX_MSG_PARAM_ZXLW_PARAM, param, size);
		break;
	case PARAM_CHECK_TIME:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CHECK_TIME,
				INDEX_MSG_PARAM_CHECK_TIME, param, size);
		break;
	case PARAM_ZTE_STREAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_ZTE_STREAM,
				INDEX_MSG_PARAM_ZTE_STREAM, param, size);
		break;
	case PARAM_ZTE_ONLINE:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_ZTE_ONLINE,
				INDEX_MSG_PARAM_ZTE_ONLINE, param, size);
		break;
	case PARAM_YIXUN_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_YIXUN_PARAM,
				INDEX_MSG_PARAM_YIXUN_PARAM, param, size);
		break;

	case PARAM_DEFAULT_ENCODER:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DEFAULT_ENCODER,
				INDEX_MSG_DEFAULT_ENCODER, param, size);
		break;
	case PARAM_DECODE_OSD:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DECODE_OSD,
				INDEX_MSG_PARAM_DECODE_OSD, param, size);
		break;
	case PARAM_DECODER_BUF_TIME:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DECODER_BUF_TIME,
				INDEX_MSG_PARAM_DECODER_BUF_TIME, param, size);
		break;
	case PARAM_DECODER_BUF_FRAME_COUNT:
		iRet = GetServInfo(user_id, MSG_GET_PARAM,
				PARAM_DECODER_BUF_FRAME_COUNT,
				INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT, param, size);
		break;
	case PARAM_CENTER_SERVER_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_CENTER_SERVER_CONFIG,
				INDEX_MSG_PARAM_CENTER_SERVER_CONFIG, param, size);
		break;
	case PARAM_LENS_INFO_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_LENS_INFO_CONFIG,
				INDEX_MSG_PARAM_LENS_INFO_CONFIG, param, size);
		break;
	case PARAM_H3C_PLATFORM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_H3C_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_H3C_NETCOM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_H3C_NETCOM_CONFIG,
				INDEX_MSG_PARAM_H3C_NETCOM_CONFIG, param, size);
		break;
	case PARAM_LPS_CONFIG_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_LPS_CONFIG_INFO,
				INDEX_MSG_PARAM_LPS_CONFIG_INFO, param, size);
		break;
	case PARAM_H3C_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_H3C_INFO,
				INDEX_H3C_INFO, param, size);
		break;
	case PARAM_KEDA_PLATFORM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEDA_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_BELL_PLATFORM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_BELL_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_ZTE_PLATFORM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_ZTE_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_UDP_NAT_CLINTINFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_UDP_NAT_CLINTINFO,
				INDEX_MSG_PARAM_UDP_NAT_CLINTINFO, param, size);
		break;
	case PARAM_HXHT_PLATFORM_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HXHT_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_HXHT_ALARM_SCHEME:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HXHT_ALARM_SCHEME,
				INDEX_MSG_PARAM_HXHT_ALARM_SCHEME, param, size);
		break;
	case PARAM_HXHT_NAT:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HXHT_NAT,
				INDEX_MSG_PARAM_HXHT_NAT, param, size);
		break;
	case PARAM_FENGH_NAT:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_FENGH_NAT,
				INDEX_MSG_PARAM_FENGH_NAT, param, size);
		break;
	case PARAM_GONGX_NAT:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_GONGX_NAT,
				INDEX_MSG_PARAM_GONGX_NAT, param, size);
		break;
	case PARAM_HAIDO_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HAIDO_PARAM,
				INDEX_MSG_PARAM_HAIDOU_NAT, param, size);
		break;
	case PARAM_HAIDO_NAS:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_HAIDO_NAS,
				INDEX_MSG_PARAM_HAIDOU_NAS, param, size);
		break;
	case PARAM_LANGTAO_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_LANGTAO_PARAM,
				INDEX_MSG_PARAM_LANGTAO_NAT, param, size);
		break;
	case PARAM_UT_PARAM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_UT_PARAM,
				INDEX_MSG_PARAM_UT_NAT, param, size);
		break;
	case PARAM_NETSTAR_CONFIG:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_NETSTAR_CONFIG,
				INDEX_DECORD_PARAM_NETSTAR, param, size);
		break;
	case PARAM_DECODE_NUM:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DECODE_NUM,
				INDEX_DECORD_PARAM_NUM, param, size);
		break;
	case PARAM_DECODE_STAT:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_DECODE_STAT,
				INDEX_DECORD_PARAM_STAR, param, size);
		break;
	case PARAM_KEY_WEB_PWD:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_WEB_PWD,
				INDEX_MSG_PARAM_KEY_WEB_PWD, param, size);
		break;
	case PARAM_KEY_USER_PWD:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_USER_PWD,
				INDEX_MSG_PARAM_KEY_USER_PWD, param, size);
		break;
	case PARAM_KEY_SERVER_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_SERVER_INFO,
				INDEX_MSG_PARAM_KEY_SERVER_INFO, param, size);
		break;
	case PARAM_KEY_ENC_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_ENC_INFO,
				INDEX_MSG_PARAM_KEY_ENC_INFO, param, size);
		break;
	case PARAM_KEY_ENC_CHANNEL_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_ENC_CHANNEL_INFO,
				INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO, param, size);
		break;
	case PARAM_KEY_DEC_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_DEC_INFO,
				INDEX_MSG_PARAM_KEY_DEC_INFO, param, size);
		break;
	case PARAM_KEY_TOUR_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_TOUR_INFO,
				INDEX_MSG_PARAM_KEY_TOUR_INFO, param, size);
		break;
	case PARAM_KEY_TOUR_STEP_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_TOUR_STEP_INFO,
				INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO, param, size);
		break;
	case PARAM_KEY_GROUP_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_GROUP_INFO,
				INDEX_MSG_PARAM_KEY_GROUP_INFO, param, size);
		break;
	case PARAM_KEY_GROUP_STEP_INFO:
		iRet = GetServInfo(user_id, MSG_GET_PARAM, PARAM_KEY_GROUP_STEP_INFO,
				INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO, param, size);
		break;
	default:
		break;
	}

	return iRet;
}
int st_net_setParam(unsigned long user_id, int type, void *param, int size) {
	int iRet = 0;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;

	switch (type) {
	case PARAM_HANDIO_CONTROL:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HANDIO_CONTROL,
				INDEX_MSG_PARAM_HANDIO_CONTROL, param, size);
		break;
	case PARAM_TIMEIO_CONTROL:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TIMEIO_CONTROL,
				INDEX_MSG_PARAM_TIMEIO_CONTROL, param, size);
		break;
	case PARAM_AUTO_POWEROFF:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_AUTO_POWEROFF,
				INDEX_MSG_PARAM_AUTO_POWEROFF, param, size);
		break;
	case PARAM_AUTO_POWERON:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_AUTO_POWERON,
				INDEX_MSG_PARAM_AUTO_POWERON, param, size);
		break;
	case PARAM_USER_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_USER_INFO,
				INDEX_MSG_PARAM_USER_INFO, param, size);
		break;
	case PARAM_HAND_RECORD:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HAND_RECORD,
				INDEX_MSG_PARAM_HAND_RECORD, param, size);
		break;
	case PARAM_NETWORK_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_NETWORK_CONFIG,
				INDEX_MSG_PARAM_NETWORK_CONFIG, param, size);
		break;
	case PARAM_MISC_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_MISC_CONFIG,
				INDEX_MSG_PARAM_MISC_CONFIG, param, size);
		break;
	case PARAM_PROBER_ALRAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_PROBER_ALRAM,
				INDEX_MSG_PARAM_PROBER_ALRAM, param, size);
		break;
	case PARAM_RECORD_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_RECORD_PARAM,
				INDEX_MSG_PARAM_RECORD_PARAM, param, size);
		break;
	case PARAM_TERM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TERM_CONFIG,
				INDEX_MSG_PARAM_TERM_CONFIG, param, size);
		break;
	case PARAM_TIMER_RECORD:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TIMER_RECORD,
				INDEX_MSG_PARAM_TIMER_RECORD, param, size);
		break;
	case PARAM_VIDEO_LOSE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_VIDEO_LOSE,
				INDEX_MSG_PARAM_VIDEO_LOSE, param, size);
		break;
	case PARAM_VIDEO_MOVE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_VIDEO_MOVE,
				INDEX_MSG_PARAM_VIDEO_MOVE, param, size);
		break;
	case PARAM_VIDEO_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_VIDEO_CONFIG,
				INDEX_MSG_PARAM_VIDEO_CONFIG, param, size);
		break;
	case PARAM_VIDEO_ENCODE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_VIDEO_ENCODE,
				INDEX_MSG_PARAM_VIDEO_ENCODE, param, size);
		break;
	case PARAM_COM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_COM_CONFIG,
				INDEX_MSG_PARAM_COM_CONFIG, param, size);
		break;
	case PARAM_OSD_SET:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_OSD_SET,
				INDEX_MSG_PARAM_OSDSET, param, size);
		break;
	case PARAM_OVERLAY_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_OVERLAY_INFO,
				INDEX_MSG_PARAM_OVERLAY, param, size);
		break;
	case PARAM_MOTION_ZOOM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_MOTION_ZOOM,
				INDEX_MSG_PARAM_MOTION_ZOOM, param, size);
		break;
	case PARAM_PPPOE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_PPPOE,
				INDEX_MSG_PARAM_PPPOE, param, size);
		break;
	case PARAM_RECORD_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_RECORD_INFO,
				INDEX_MSG_PARAM_RECORD_INFO, param, size);
		break;
	case PARAM_WIRELESS_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_WIRELESS_CONFIG,
				INDEX_MSG_PARAM_WIRELESS_CONFIG, param, size);
		break;
	case PARAM_MAIL_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_MAIL_CONFIG,
				INDEX_MSG_PARAM_MAIL_CONFIG, param, size);
		break;
	case PARAM_FTP_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_FTP_CONFIG,
				INDEX_MSG_PARAM_FTP_CONFIG, param, size);
		break;
	case PARAM_SERIAL_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_SERIAL_INFO,
				INDEX_MSG_PARAM_SERIAL_INFO, param, size);
		break;
	case PARAM_TRANSPARENCE_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TRANSPARENCE_INFO,
				INDEX_MSG_PARAM_TRANSPARENCE_INFO, param, size);
		break;
	case PARAM_SLAVE_ENCODE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_SLAVE_ENCODE,
				INDEX_MSG_PARAM_SLAVE_INFO, param, size);
		break;
	case PARAM_DEVICE_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DEVICE_INFO,
				INDEX_MSG_PARAM_DEVICE_INFO, param, size);
		break;
	case PARAM_AUDIO_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_AUDIO_INFO,
				INDEX_MSG_PARAM_AUDIO_INFO, param, size);
		break;
	case PARAM_MEMBER_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_MEMBER_INFO,
				INDEX_MSG_PARAM_MEMBER_INFO, param, size);
		break;
	case PARAM_CAMERA_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CAMERA_INFO,
				INDEX_MSG_PARAM_CAMERA_INFO, param, size);
		break;
	case PARAM_NAS_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_NAS_INFO,
				INDEX_MSG_PARAM_NAS_INFO, param, size);
		break;
	case PARAM_ITU_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_ITU_INFO,
				INDEX_MSG_PARAM_ITU_INFO, param, size);
		break;
	case PARAM_TIMER_COLOR2GREY:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TIMER_COLOR2GREY,
				INDEX_MSG_PARAM_COLOR_INFO, param, size);
		break;
	case PARAM_NA208_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_NA208_INFO,
				INDEX_MSG_PARAM_NA208_INFO, param, size);
		break;
	case PARAM_MATRIX_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_MATRIX_INFO,
				INDEX_MSG_PAPAM_MATRIX_INFO, param, size);
		break;
	case PARAM_TIMER_CAPTURE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TIMER_CAPTURE,
				INDEX_MSG_TIMER_CAPTURE, param, size);
		break;
	case PARAM_TIMER_RESTART:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_TIMER_RESTART,
				INDEX_MSG_PARAM_TIMER_RESTART, param, size);
		break;
	case PARMA_PTZ_CURISE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARMA_PTZ_CURISE,
				INDEX_PARMA_PTZ_CURISE, param, size);
		break;
	case PARAM_USB_TYPE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_USB_TYPE,
				INDEX_PARAM_USB_TYPE, param, size);
		break;
	case PARAM_CDMA_TIME:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CDMA_TIME,
				INDEX_MSG_CDMA_TIME, param, size);
		break;
	case PARAM_CDMA_ALARM_ENABLE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CDMA_ALARM_ENABLE,
				INDEX_MSG_CDMA_ALARM_ENABLE, param, size);
		break;
	case PARAM_CDMA_NONEGO_ENABLE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CDMA_NONEGO_ENABLE,
				INDEX_MSG_CDMA_NONEGO_ENABLE, param, size);
		break;
	case PARAM_CDMA_STREAM_ENABLE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CDMA_STREAM_ENABLE,
				INDEX_MSG_CDMA_STREAM_ENABLE, param, size);
		break;
	case PARAM_CDMA_ALARM_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CDMA_ALARM_INFO,
				INDEX_MSG_CDMA_ALARM_INFO, param, size);
		break;
	case PARMA_IMMEDIATE_CAPTURE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARMA_IMMEDIATE_CAPTURE,
				INDEX_MSG_IMMEDIATE_CAPTURE, param, size);
		break;
	case PARAM_3511_SYSINFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_3511_SYSINFO,
				INDEX_3511_PARAM_SYSINFO, param, size);
		break;
	case PARAM_SZ_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_SZ_INFO,
				INDEX_MSG_PARAM_SZ_PARAM, param, size);
		break;
	case PARAM_ZXLW_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_ZXLW_PARAM,
				INDEX_MSG_PARAM_ZXLW_PARAM, param, size);
		break;
	case PARAM_CHECK_TIME:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CHECK_TIME,
				INDEX_MSG_PARAM_CHECK_TIME, param, size);
		break;
	case PARAM_ZTE_STREAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_ZTE_STREAM,
				INDEX_MSG_PARAM_ZTE_STREAM, param, size);
		break;
	case PARAM_ZTE_ONLINE:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_ZTE_ONLINE,
				INDEX_MSG_PARAM_ZTE_ONLINE, param, size);
		break;
	case PARAM_YIXUN_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_YIXUN_PARAM,
				INDEX_MSG_PARAM_YIXUN_PARAM, param, size);
		break;
	case PARAM_DEFAULT_ENCODER:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DEFAULT_ENCODER,
				INDEX_MSG_DEFAULT_ENCODER, param, size);
		break;
	case PARAM_DECODE_OSD:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DECODE_OSD,
				INDEX_MSG_PARAM_DECODE_OSD, param, size);
		break;
	case PARAM_DECODER_BUF_TIME:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DECODER_BUF_TIME,
				INDEX_MSG_PARAM_DECODER_BUF_TIME, param, size);
		break;
	case PARAM_DECODER_BUF_FRAME_COUNT:
		iRet = SetServInfo(user_id, MSG_SET_PARAM,
				PARAM_DECODER_BUF_FRAME_COUNT,
				INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT, param, size);
		break;
	case PARAM_CENTER_SERVER_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_CENTER_SERVER_CONFIG,
				INDEX_MSG_PARAM_CENTER_SERVER_CONFIG, param, size);
		break;
	case PARAM_LENS_INFO_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_LENS_INFO_CONFIG,
				INDEX_MSG_PARAM_LENS_INFO_CONFIG, param, size);
		break;
	case PARAM_H3C_PLATFORM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_H3C_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_H3C_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_H3C_INFO,
				INDEX_H3C_INFO, param, size);
		break;
	case PARAM_H3C_NETCOM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_H3C_NETCOM_CONFIG,
				INDEX_MSG_PARAM_H3C_NETCOM_CONFIG, param, size);
		break;
	case PARAM_LPS_CONFIG_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_LPS_CONFIG_INFO,
				INDEX_MSG_PARAM_LPS_CONFIG_INFO, param, size);
		break;
	case PARAM_KEDA_PLATFORM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEDA_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_BELL_PLATFORM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_BELL_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_ZTE_PLATFORM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_ZTE_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_HXHT_PLATFORM_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HXHT_PLATFORM_CONFIG,
				INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG, param, size);
		break;
	case PARAM_UDP_NAT_CLINTINFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_UDP_NAT_CLINTINFO,
				INDEX_MSG_PARAM_UDP_NAT_CLINTINFO, param, size);
		break;
	case PARAM_HXHT_ALARM_SCHEME:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HXHT_ALARM_SCHEME,
				INDEX_MSG_PARAM_HXHT_ALARM_SCHEME, param, size);
		break;
	case PARAM_HXHT_NAT:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HXHT_NAT,
				INDEX_MSG_PARAM_HXHT_NAT, param, size);
		break;
	case PARAM_FENGH_NAT:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_FENGH_NAT,
				INDEX_MSG_PARAM_FENGH_NAT, param, size);
		break;
	case PARAM_GONGX_NAT:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_GONGX_NAT,
				INDEX_MSG_PARAM_GONGX_NAT, param, size);
		break;
	case PARAM_HAIDO_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HAIDO_PARAM,
				INDEX_MSG_PARAM_HAIDOU_NAT, param, size);
		break;
	case PARAM_HAIDO_NAS:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_HAIDO_NAS,
				INDEX_MSG_PARAM_HAIDOU_NAS, param, size);
		break;
	case PARAM_LANGTAO_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_LANGTAO_PARAM,
				INDEX_MSG_PARAM_LANGTAO_NAT, param, size);
		break;
	case PARAM_UT_PARAM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_UT_PARAM,
				INDEX_MSG_PARAM_UT_NAT, param, size);
		break;
	case PARAM_NETSTAR_CONFIG:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_NETSTAR_CONFIG,
				INDEX_DECORD_PARAM_NETSTAR, param, size);
		break;
	case PARAM_DECODE_NUM:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DECODE_NUM,
				INDEX_DECORD_PARAM_NUM, param, size);
		break;
	case PARAM_DECODE_STAT:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_DECODE_STAT,
				INDEX_DECORD_PARAM_STAR, param, size);
		break;
	case PARAM_KEY_WEB_PWD:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_WEB_PWD,
				INDEX_MSG_PARAM_KEY_WEB_PWD, param, size);
		break;
	case PARAM_KEY_USER_PWD:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_USER_PWD,
				INDEX_MSG_PARAM_KEY_USER_PWD, param, size);
		break;
	case PARAM_KEY_SERVER_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_SERVER_INFO,
				INDEX_MSG_PARAM_KEY_SERVER_INFO, param, size);
		break;
	case PARAM_KEY_ENC_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_ENC_INFO,
				INDEX_MSG_PARAM_KEY_ENC_INFO, param, size);
		break;
	case PARAM_KEY_ENC_CHANNEL_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_ENC_CHANNEL_INFO,
				INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO, param, size);
		break;
	case PARAM_KEY_DEC_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_DEC_INFO,
				INDEX_MSG_PARAM_KEY_DEC_INFO, param, size);
		break;
	case PARAM_KEY_TOUR_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_TOUR_INFO,
				INDEX_MSG_PARAM_KEY_TOUR_INFO, param, size);
		break;
	case PARAM_KEY_TOUR_STEP_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_TOUR_STEP_INFO,
				INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO, param, size);
		break;
	case PARAM_KEY_GROUP_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_GROUP_INFO,
				INDEX_MSG_PARAM_KEY_GROUP_INFO, param, size);
		break;
	case PARAM_KEY_GROUP_STEP_INFO:
		iRet = SetServInfo(user_id, MSG_SET_PARAM, PARAM_KEY_GROUP_STEP_INFO,
				INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO, param, size);
		break;
	default:
		break;
	}
	return iRet;
}
int st_net_stopDownloadFile(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	pConnect->iDownloadFileSize = 0;
	if (pConnect->downloadfile_tcp_sock != 0) {
		if (PROTOCOL_TCP == g_protocol)
			close(pConnect->downloadfile_tcp_sock);
		else
			udtc_close(pConnect->downloadfile_tcp_sock);
		usleep(500 * 1000);
	}
	if (pConnect->downloadfile) {
		fclose((void *) pConnect->downloadfile);
//		AVI_fclose((void *)pConnect->downloadfile);
		pConnect->downloadfile = NULL;
	}
	return 0;
}

int st_net_stopDownloadFileByTime(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	pConnect->iDownloadFileSize = 0;
	if (pConnect->downloadfile_tcp_sock != 0) {
		if (PROTOCOL_TCP == g_protocol)
			close(pConnect->downloadfile_tcp_sock);
		else
			udtc_close(pConnect->downloadfile_tcp_sock);
		usleep(500 * 1000);
	}
	if (pConnect->downloadfile) {
		fclose((void *) pConnect->downloadfile);
//		AVI_fclose((void *)pConnect->downloadfile);
		pConnect->downloadfile = NULL;
	}
	return 0;
}

int st_net_startRecord(unsigned long user_id, int channel, char *filename) //add by yangll
{
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	if (st_net_forceKeyFrame(user_id, channel) != 0)
		return (-1);
	strcpy(pConnect->channel_info[channel].record_name, filename);
	pConnect->channel_info[channel].record_file = fopen(filename, "wb+");
//	pConnect->channel_info[channel].record_file = (FILE *)AVI_fopen(filename, AVI_WRITE);//20090819
	if (pConnect->channel_info[channel].record_file == NULL)
		return -1;
	pConnect->channel_info[channel].record_flag = TRUE;
	return 0;
}

int st_net_stopRecord(unsigned long user_id, int channel) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	pConnect->channel_info[channel].record_flag = FALSE;
	usleep(500 * 1000);
//	AVI_fclose((void *)pConnect->channel_info[channel].record_file);
	fclose(pConnect->channel_info[channel].record_file);
	return 0;
}

int st_net_queryLog(unsigned long user_id, msg_search_log_t log_param,
		msg_log_info_t *p_msg_log_info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int len = 0;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_GET_LOG, 0, (char *) &log_param,
			sizeof(msg_search_log_t)) < 0)
		return -1;

	while (1) {
		if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_GET_LOG_INFO],
		OVER_TIME) == 0) {
			int num;
			if (GetMsg(pConnect, INDEX_MSG_GET_LOG_INFO, &cache_net_msg) < 0)
				break;

			memcpy(&num, cache_net_msg.msg_data, sizeof(int));
			len = num * sizeof(msg_log_info_t);
			memcpy(p_msg_log_info, (char*) cache_net_msg.msg_data + sizeof(int),
					len);
			return num;
		} else {
			pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO] = FALSE;
			return -1;
		}
	}
	return 0;
}
int st_net_deleteLog(unsigned long user_id, msg_search_log_t msg_serch_log) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_DELETE_LOG_INFO] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_DELETE_LOG, 0,
			(char*) &msg_serch_log, sizeof(msg_search_log_t)) < 0) {
		pConnect->interval_valid[INDEX_MSG_DELETE_LOG_INFO] = FALSE;
		return -1;
	}
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_DELETE_LOG_INFO],
	OVER_TIME) == 0) {
		if (GetMsg(pConnect, INDEX_MSG_DELETE_LOG_INFO, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
	} else {
		pConnect->interval_valid[INDEX_MSG_DELETE_LOG_INFO] = FALSE;
		return -1;
	}
	return 0;
}
int st_net_queryUserInfo(unsigned long user_id, msg_user_info_t *p_user) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int len = 0;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_GET_USERINFO] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_GET_USERINFO, 0, NULL, 0) < 0)
		return -1;

	while (1) {
		if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_GET_USERINFO],
		OVER_TIME) == 0) {
			int num;
			if (GetMsg(pConnect, INDEX_MSG_GET_USERINFO, &cache_net_msg) < 0)
				break;

			memcpy(&num, cache_net_msg.msg_data, sizeof(int));
			len = num * sizeof(msg_log_info_t);
			memcpy(p_user, (char*) cache_net_msg.msg_data + sizeof(int), len);
			return num;
		} else {
			pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO] = FALSE;
			return -1;
		}
	}
	return 0;
}

int st_net_addUser(unsigned long user_id, msg_user_info_t user) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_ADD_USER, 0, INDEX_MSG_ADD_USER,
			(void*) &user, sizeof(msg_user_info_t));
}

int st_net_deleteUser(unsigned long user_id, msg_user_info_t user) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_DELETE_USER, 0, INDEX_MSG_DELETE_USER,
			(void*) &user, sizeof(msg_user_info_t));
}

int st_net_changeUser(unsigned long user_id, msg_user_info_t user) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (pServerInfo->server_connect == NULL)
		return -1;
	return SetServInfo(user_id, MSG_MODIFY_USER, 0, INDEX_MSG_CHANGE_USER,
			(void*) &user, sizeof(msg_user_info_t));
}

int st_net_searchWirelessDevice(unsigned long user_id,
		msg_wireless_info_t *p_wireless_info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_WIRDLESS_SEARCH] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_GET_AP_INFO, 0, 0, 0) < 0)
		return -1;

	while (1) {
		if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_WIRDLESS_SEARCH],
				20 * 1000) == 0) {
			int num = 0;
			if (GetMsg(pConnect, INDEX_MSG_WIRDLESS_SEARCH, &cache_net_msg) < 0)
				break;

			memcpy(&num, cache_net_msg.msg_data, sizeof(int));
			memcpy(p_wireless_info,
					(char*) cache_net_msg.msg_data + sizeof(int),
					num * sizeof(msg_wireless_info_t));
			return num;
		} else {
			pConnect->interval_valid[INDEX_MSG_WIRDLESS_SEARCH] = FALSE;
			return -1;
		}
	}
	return 0;
}

int st_net_queryFileList(unsigned long user_id, msg_search_file_t record_param,
		msg_file_info_t *p_record_info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_GET_RECORD_LIST] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_GET_RECORD_LIST, 0,
			(char*) &record_param, sizeof(msg_search_file_t)) < 0)
		return -1;

	while (1) {
		if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_GET_RECORD_LIST],
				20 * 1000) == 0) {
			int num;
			if (GetMsg(pConnect, INDEX_MSG_GET_RECORD_LIST, &cache_net_msg) < 0)
				break;

			memcpy(&num, cache_net_msg.msg_data, sizeof(int));
			memcpy(p_record_info, (char*) cache_net_msg.msg_data + sizeof(int),
					num * sizeof(msg_file_info_t));
			return num;
		} else {
			pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO] = FALSE;
			return -1;
		}
	}
	return 0;
}

int st_net_queryFileListByTime(unsigned long user_id,
		msg_search_file_t record_param, msg_fragment_t *p_record_info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_MSG_GET_RECORD_LIST] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_GET_RECORD_LIST, 0,
			(char*) &record_param, sizeof(msg_search_file_t)) < 0)
		return -1;

	while (1) {
		if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_GET_RECORD_LIST],
				20 * 1000) == 0) {
			int num;
			if (GetMsg(pConnect, INDEX_MSG_GET_RECORD_LIST, &cache_net_msg) < 0)
				break;

			memcpy(&num, cache_net_msg.msg_data, sizeof(int));
			if (num > 36) {
				num = 36;
			}
			memcpy(p_record_info, (char*) cache_net_msg.msg_data + sizeof(int),
					num * sizeof(msg_fragment_t));

			int i;
			for (i = 0; i < num; i++) {
				LOGI("result:%s", p_record_info[i].file_name);
			}
			record_info = (msg_fragment_t *) malloc(
					1000 * sizeof(msg_fragment_t));
			memcpy(record_info, p_record_info, num * sizeof(msg_fragment_t));
			return num;
		} else {
			pConnect->interval_valid[INDEX_MSG_GET_LOG_INFO] = FALSE;
			return -1;
		}
	}
	return 0;
}

int record_status_callback(void *context) {
	return 1;
}

int record_decode_callback(void *buf, int len, void *context) {
	st_inputFrameListTail((char *) buf, len - 32);
	return 0;
}

int st_net_startrecord(long user_id, unsigned int index) {

	msg_fragment_t f_info = record_info[index];
	LOGI("user_id__:%d", user_id);
	LOGI("index__:%d", index);
	LOGI("file_name____:%s", f_info.file_name);
	msg_recordplay_t msg_recordplay;
	memset(&msg_recordplay, 0, sizeof(msg_recordplay_t));
	stop = 0;
	int remote_play_id = st_net_startRemotePlayEx(user_id, record_info[index],
			&msg_recordplay);
//	unsigned long remote_play_id = st_net_startRemotePlayByTime(user_id,
//			record_info[index], msg_recordplay);
	LOGI("remote_play_id__:%lu", remote_play_id);
//	st_destroyFrameList();
//	if (remote_play_id < 0) {
//		return -1;
//	}
	//unsigned long remote_play_id,
	//PLAYRECORD_STATUS_CALLBACK //callback,
	//void *context
//	st_net_registerServerPlayrecordStatusCallback(remote_play_id);
	st_net_registerServerPlayrecordStatusCallback(remote_play_id,
			record_status_callback, NULL);
	st_net_registerServerPlayrecordDecodeCallback(remote_play_id,
			record_decode_callback, NULL);
	return remote_play_id;
}

int st_net_downloadRecord(unsigned long user_id, unsigned int index,
		const char* file) {
	if (record_info == NULL) {
		LOGI("record_info == NULL");
		return -1;
	}
	msg_fragment_t f_info = record_info[index];
	//char *savefilename,
	LOGI("错误1");
	msg_file_down_Status* file_down_Status = (msg_file_down_Status*) malloc(
			sizeof(msg_file_down_Status));
	LOGI("错误2");
//	st_net_downloadFileByTime(user_id,f_info,file,file_down_Status);
	stopdownload = 0;
	int result = st_net_startDownloadFileEx(user_id, f_info, file,
			file_down_Status);
//	st_net_dow
	LOGI(
			"st_net_startDownloadFileEx__result__:%.2f", (float)file_down_Status->iTransfersSize/(float)file_down_Status->iFileSize);
}
int st_net_deleteRecord(long user_id, unsigned int index) {
	if (record_info == NULL) {
		LOGI("record_info == NULL");
		return -1;
	}
	msg_fragment_t f_info = record_info[index];
	del_file_info_t del_file_info;
	del_file_info.ch = 0;
	del_file_info.year = f_info.start_time.year;
	del_file_info.month = f_info.start_time.month;
	del_file_info.day = f_info.start_time.day;

	del_file_info.start_hour = f_info.start_time.hour;
	del_file_info.start_minute = f_info.start_time.minute;
	del_file_info.start_second = f_info.start_time.second;

	del_file_info.end_hour = f_info.end_time.hour;
	del_file_info.end_minute = f_info.end_time.minute;
	del_file_info.end_second = f_info.end_time.second;
	msg_delete_file_t msg_delete_record;
	memset(&msg_delete_record, 0, sizeof(msg_delete_record));
	memcpy(msg_delete_record.filename, &del_file_info, sizeof(del_file_info_t));
	int ret = st_net_deleteFile(user_id, msg_delete_record);
	return ret;
}
int st_net_deleteFile(unsigned long user_id,
		msg_delete_file_t msg_delete_record) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SetServInfo(user_id, MSG_DELETE_RECORD_FILE, 0,
			INDEX_MSG_DELETE_RECORD_FILE, (void*) &msg_delete_record,
			sizeof(msg_delete_file_t));
}

int st_net_getHarddiskInfo(unsigned long user_id, msg_harddisk_data_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return GetServInfo(user_id, MSG_HARDDISK_INFO, 0, INDEX_MSG_HARDDISK_INFO,
			(void*) info, sizeof(msg_harddisk_data_t));
}

int st_net_alarmControl(unsigned long user_id, msg_alarm_control_t param) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (NULL == pConnect)
		return -1;
	return SetServInfo(user_id, MSG_ALARM_CONTROL, 0, INDEX_MSG_ALARM_CONTROL,
			(void*) &param, sizeof(msg_alarm_control_t));
}

int st_net_matrixControl_loginout(unsigned long user_id, int type) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SetServInfo(user_id, MSG_PTZ_CONTROL, type, INDEX_MSG_PTZ_CONTROL,
			NULL, 0);
}

int st_net_matrixControl_login(unsigned long user_id, int type,
		msg_matrix_login_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int datasize = (info == NULL) ? 0 : sizeof(msg_matrix_login_t);

	if (!pConnect)
		return -1;

	return SetServInfo(user_id, MSG_PTZ_CONTROL, type, INDEX_MSG_PTZ_CONTROL,
			(void*) info, datasize);
}
int st_net_matrixControl(unsigned long user_id, int type,
		msg_ptz_control_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	int datasize = (info == NULL) ? 0 : sizeof(msg_ptz_control_t);
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	return SetServInfo(user_id, MSG_PTZ_CONTROL, type, INDEX_MSG_PTZ_CONTROL,
			(void*) info, datasize);
}

int st_net_transparentCmd(unsigned long user_id, msg_transparent_cmd_t cmd) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SetServInfo(user_id, MSG_TRANSPARENT_CMD, 0, INDEX_MSG_TRANSPARENT,
			(void*) &cmd, sizeof(msg_transparent_cmd_t));
}

int st_net_getUpgradeStatus(unsigned long user_id, msg_upgrade_status_t *info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return GetServInfo(user_id, MSG_UPGRADE_STATUS, 0, INDEX_MSG_UPGRADE_STATUS,
			(void*) &info, sizeof(msg_upgrade_status_t));
}

int st_net_getHeartbeatStatus(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;
	net_msg_t net_msg;
	int sendLen = 0;
	if (!pConnect)
		return -1;

	net_msg.msg_head.msg_type = MSG_HEARTBEAT;
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_size = 0;
	net_msg.msg_head.ack_flag = ACK_SUCCESS;
	pConnect->interval_valid[INDEX_MSG_HEARTBEAT] = TRUE;

	if (PROTOCOL_TCP == g_protocol)
		sendLen = writen(pConnect->msg_sock, (char *) &net_msg,
				sizeof(msg_head_t));
	else
		sendLen = udtc_sendn(pConnect->msg_sock, (char *) &net_msg,
				sizeof(msg_head_t));
	if (sendLen != sizeof(msg_head_t))
		pServerInfo->iMsgStatus = 0;

	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_HEARTBEAT], 6 * 1000)
			== 0) {
		if (GetMsg(pConnect, INDEX_MSG_HEARTBEAT, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			pServerInfo->iMsgStatus = 0;
		else
			pServerInfo->iMsgStatus = 1;
	} else
		pServerInfo->iMsgStatus = 0;
	return pServerInfo->iMsgStatus;
}
int st_net_getDeviceType(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return (pConnect->device_type);
}

int st_net_beginplayback(unsigned long user_id) {
	return SetServInfo(user_id, MSG_START_BACK, 0, INDEX_MSG_START_BACK, NULL,
			0);
}

int st_net_stopplayback(unsigned long user_id) {
	return SetServInfo(user_id, MSG_STOP_BACK, 0, INDEX_MSG_STOP_BACK, NULL, 0);
}

int st_net_defaultSetting(unsigned long user_id) {
	return SetServInfo(user_id, MSG_DEFAULT_SETTING, 0,
			INDEX_MSG_DEFAULT_SETTING, NULL, 0);
}

int st_net_defOutPutSetting(unsigned long user_id) {
	return SetServInfo(user_id, MSG_DEFAULTOUTPUT_SETING, 0,
			INDEX_MSG_PARAM_RECOVERDEF_CONFIG, NULL, 0);
}

int st_net_defaultVidioParame(unsigned long user_id, unsigned char channel_no) {
	return SetServInfo(user_id, MSG_RESET_VIDEO_PARAM, 0, INDEX_MSG_RESET_VIDIO,
			&channel_no, 1);
}

int st_net_defaultCcdParame(unsigned long user_id) {
	return SetServInfo(user_id, MSG_CCD_DEFAULT, 0, INDEX_MSG_PARAM_CCD_DEFAULT,
			NULL, 0);
}

int st_net_defaultItuParame(unsigned long user_id) {
	return SetServInfo(user_id, MSG_ITU_DEFAULT, 0, INDEX_MSG_PARAM_ITU_DEFAULT,
			NULL, 0);
}
int st_net_setVersion(unsigned long user_id, msg_version_info_t info) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SetServInfo(user_id, MSG_VERSION_SETING, 0, INDEX_MSG_SETVERSION,
			(void*) &info, sizeof(msg_version_info_t));
}
int st_net_restartDevice(unsigned long user_id) {
	return SetServInfo(user_id, MSG_HAND_RESTART, 0, INDEX_MSG_HAND_RESTART,
			NULL, 0);
}
int st_net_harddiskPartition(unsigned long user_id,
		msg_hard_fdisk_t msg_hard_fdisk) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return SendNetMsg(pConnect->msg_sock, MSG_PARTITION, 0,
			(char*) &msg_hard_fdisk, sizeof(msg_hard_fdisk_t));
}
int st_net_fdiskStatus(unsigned long user_id,
		msg_hard_fdisk_status_t msg_hard_fdisk_status) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;
	msg_hard_fdisk_status_t recv_fdisk_status;
	pConnect->interval_valid[INDEX_MSG_FDISK_STATUS] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_FDISK_STATUS, 0,
			(char*) &msg_hard_fdisk_status, sizeof(msg_hard_fdisk_status_t))
			== -1)
		return -1;
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_FDISK_STATUS],
	OVER_TIME) == 0) {
		if (GetMsg(pConnect, INDEX_MSG_FDISK_STATUS, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
		else {
			memcpy((char *) &recv_fdisk_status, cache_net_msg.msg_data,
					sizeof(msg_hard_fdisk_status_t));
			return recv_fdisk_status.fdisk_status;
		}
	} else {
		pConnect->interval_valid[INDEX_MSG_FDISK_STATUS] = FALSE;
		return -1;
	}
	return 0;
}

//格式化
int st_net_harddiskFormat(unsigned long user_id,
		msg_part_format_t msg_part_format) {
	return SetServInfo(user_id, MSG_FORMAT, 0, INDEX_MSG_FORMAT,
			(void*) &msg_part_format, sizeof(msg_part_format_t));
}
int st_net_harddiskFormatStatus(unsigned long user_id,
		msg_hard_format_status_t msg_hard_format_status) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;
	msg_hard_format_status_t recv_format_status;

	pConnect->interval_valid[INDEX_MSG_FORMAT_STATUS] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_FORMAT_STATUS, 0,
			(char*) &msg_hard_format_status, sizeof(msg_hard_format_status_t))
			== -1)
		return -1;
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_FORMAT_STATUS],
	OVER_TIME) == 0) {
		if (GetMsg(pConnect, INDEX_MSG_FORMAT_STATUS, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
		else {
			memcpy((char *) &recv_format_status, cache_net_msg.msg_data,
					sizeof(msg_hard_format_status_t));
			return recv_format_status.format_status;
		}
	} else {
		pConnect->interval_valid[INDEX_MSG_FORMAT_STATUS] = FALSE;
		return -1;
	}
	return 0;
}

int st_net_getBitRate(unsigned long user_id, int channel) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;
	return pConnect->channel_info[channel].bit_rate;
}
int st_net_searchDevice(msg_broadcast_t *msg_broadcast, int iMaxCount) {
	int opt_val = 1;
	char ip_addr[16];
	unsigned int sockfd;
	net_msg_t send_net_msg;
	net_msg_t rev_net_msg;
	struct sockaddr_in RemoteAddr;
	struct sockaddr_in LocalAddr;
	int device_num = 0;

	sockfd = udtc_socket(AF_INET, SOCK_DGRAM, 0);
	memset(&RemoteAddr, 0, sizeof(RemoteAddr));
	strcpy(ip_addr, "255.255.255.255");
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(ip_addr);
	RemoteAddr.sin_port = htons(SEARCH_PORT);

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	LocalAddr.sin_port = htons(PC_SEARCH_PORT);

	if (udtc_bind(sockfd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr))
			< 0) {
		LOGE("udtc_recv udtc_bind fail");
		perror("udtc_bind fail");
		return -1;
	}
	udtc_setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &opt_val,
			sizeof(opt_val));

	send_net_msg.msg_head.msg_type = MSG_BROADCAST_DEVICE;
	send_net_msg.msg_head.msg_subtype = 0;
	send_net_msg.msg_head.ack_flag = ACK_SUCCESS;
	send_net_msg.msg_head.msg_size = 0;
	sendto(sockfd, (char*) &send_net_msg, sizeof(msg_head_t), 0,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr));

	while (1) {
		unsigned int len;
		struct timeval tval;
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		tval.tv_sec = 3;
		tval.tv_usec = 0;
		if (select(sockfd + 1, &rset, 0, 0, &tval) <= 0)
			break;
		len = recvfrom(sockfd, (char *) &rev_net_msg, sizeof(net_msg_t), 0,
				NULL, NULL);
		if (len != sizeof(msg_head_t) + rev_net_msg.msg_head.msg_size)
			break;
		memcpy(msg_broadcast + device_num, (char *) rev_net_msg.msg_data,
				sizeof(msg_broadcast_t));
		device_num++;
		if (device_num >= iMaxCount)
			break;
	}
	udtc_close(sockfd);
	return device_num;
}
int st_net_broadcastSetIp(msg_broadcast_setnetinfo_t msg_broadcast_setnetinfo) {
	int opt_val = 1;
	char ip_addr[16];
	int sockfd;
	net_msg_t send_net_msg;
	net_msg_t rev_net_msg;
	struct sockaddr_in RemoteAddr;
	struct sockaddr_in LocalAddr;
	struct timeval tval;
	fd_set rset;
	unsigned int len;

	sockfd = udtc_socket(AF_INET, SOCK_DGRAM, 0);
	memset(&RemoteAddr, 0, sizeof(RemoteAddr));
	strcpy(ip_addr, "255.255.255.255");
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(ip_addr);
	RemoteAddr.sin_port = htons(SEARCH_PORT);

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	LocalAddr.sin_port = htons(PC_SEARCH_PORT);

	if (udtc_bind(sockfd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr))
			< 0) {
		LOGE("udtc_recv udtc_bind fail");
		perror("udtc_bind fail");
		return -1;
	}
	udtc_setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &opt_val,
			sizeof(opt_val));

	send_net_msg.msg_head.msg_type = MSG_BROADCAST_IP;
	send_net_msg.msg_head.msg_subtype = 0;
	send_net_msg.msg_head.ack_flag = ACK_SUCCESS;
	send_net_msg.msg_head.msg_size = sizeof(msg_broadcast_setnetinfo_t);
	memcpy(send_net_msg.msg_data, &msg_broadcast_setnetinfo,
			sizeof(msg_broadcast_setnetinfo_t));
	sendto(sockfd, (char*) &send_net_msg,
			sizeof(msg_head_t) + sizeof(msg_broadcast_setnetinfo_t), 0,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr));

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tval.tv_sec = 0;
	tval.tv_usec = 200 * 1000;
	if (select(sockfd + 1, &rset, 0, 0, &tval) <= 0) {
		udtc_close(sockfd);
		return -1;
	}
	len = recvfrom(sockfd, (char *) &rev_net_msg, sizeof(net_msg_t), 0, NULL,
			NULL);
	if (len != sizeof(msg_head_t) + rev_net_msg.msg_head.msg_size) {
		//	close(sockfd);
		udtc_close(sockfd);
		return -1;
	}
	if (rev_net_msg.msg_head.msg_type != MSG_BROADCAST_IP
			|| rev_net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		udtc_close(sockfd);
		return -1;
	}
	udtc_close(sockfd);
	return 0;
}
//广播恢复参数设置
int st_net_broadcastDefaultSetting(
		msg_broadcast_default_t msg_broadcast_default) {
	int opt_val = 1;
	char ip_addr[16];
	int sockfd;
	net_msg_t send_net_msg;
	net_msg_t rev_net_msg;
	struct sockaddr_in RemoteAddr;
	struct sockaddr_in LocalAddr;
	struct timeval tval;
	fd_set rset;

	sockfd = udtc_socket(AF_INET, SOCK_DGRAM, 0);
	memset(&RemoteAddr, 0, sizeof(RemoteAddr));
	strcpy(ip_addr, "255.255.255.255");
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(ip_addr);
	RemoteAddr.sin_port = htons(SEARCH_PORT);

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	LocalAddr.sin_port = htons(PC_SEARCH_PORT);

	if (udtc_bind(sockfd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr))
			< 0) {
		LOGE("udtc_recv udtc_bind fail");
		perror("udtc_bind fail");
		return -1;
	}
	udtc_setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &opt_val,
			sizeof(opt_val));

	send_net_msg.msg_head.msg_type = MSG_DEFAULT_SETTING;
	send_net_msg.msg_head.msg_subtype = 0;
	send_net_msg.msg_head.ack_flag = ACK_SUCCESS;
	send_net_msg.msg_head.msg_size = sizeof(msg_broadcast_default_t);
	memcpy(send_net_msg.msg_data, &msg_broadcast_default,
			sizeof(msg_broadcast_default_t));
	if (sendto(sockfd, (char*) &send_net_msg,
			sizeof(msg_head_t) + sizeof(msg_broadcast_default_t), 0,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr))
			!= sizeof(msg_head_t) + sizeof(msg_broadcast_default_t)) {
		udtc_close(sockfd);
		return -1;
	}

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tval.tv_sec = 0;
	tval.tv_usec = 200 * 1000;
	if (select(sockfd + 1, &rset, 0, 0, &tval) <= 0) {
		udtc_close(sockfd);
		return -1;
	}
	if (recvfrom(sockfd, (char *) &rev_net_msg, sizeof(net_msg_t), 0, NULL,
			NULL) != sizeof(msg_head_t) + rev_net_msg.msg_head.msg_size) {
		udtc_close(sockfd);
		return -1;
	}
	if (rev_net_msg.msg_head.msg_type != MSG_DEFAULT_SETTING
			|| rev_net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		udtc_close(sockfd);
		return -1;
	}

	udtc_close(sockfd);
	return 0;
}
//广播恢复出厂设置
int st_net_broadcastFactorySetting(
		msg_broadcast_default_t msg_broadcast_default) {
	int opt_val = 1;
	char ip_addr[16];
	int sockfd;
	net_msg_t send_net_msg;
	net_msg_t rev_net_msg;
	struct sockaddr_in RemoteAddr;
	struct sockaddr_in LocalAddr;
	int send_len;
	struct timeval tval;
	fd_set rset;
	unsigned int len;

	sockfd = udtc_socket(AF_INET, SOCK_DGRAM, 0);
	memset(&RemoteAddr, 0, sizeof(RemoteAddr));
	strcpy(ip_addr, "255.255.255.255");
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(ip_addr);
	RemoteAddr.sin_port = htons(SEARCH_PORT);

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	LocalAddr.sin_port = htons(PC_SEARCH_PORT);

	if (udtc_bind(sockfd, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr))
			< 0) {
		LOGE("udtc_recv udtc_bind fail");
		perror("udtc_bind fail");
		return -1;
	}
	udtc_setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &opt_val,
			sizeof(opt_val));

	send_net_msg.msg_head.msg_type = MSG_DEFAULTOUTPUT_SETING;
	send_net_msg.msg_head.msg_subtype = 0;
	send_net_msg.msg_head.ack_flag = ACK_SUCCESS;
	send_net_msg.msg_head.msg_size = sizeof(msg_broadcast_default_t);
	memcpy(send_net_msg.msg_data, &msg_broadcast_default,
			sizeof(msg_broadcast_default_t));
	send_len = sendto(sockfd, (char*) &send_net_msg,
			sizeof(msg_head_t) + sizeof(msg_broadcast_default_t), 0,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr));
	if (send_len != sizeof(msg_head_t) + sizeof(msg_broadcast_default_t)) {
		udtc_close(sockfd);
		return -1;
	}

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tval.tv_sec = 0;
	tval.tv_usec = 200 * 1000;
	if (select(sockfd + 1, &rset, 0, 0, &tval) <= 0) {
		udtc_close(sockfd);
		return -1;
	}
	len = recvfrom(sockfd, (char *) &rev_net_msg, sizeof(net_msg_t), 0, NULL,
			NULL);
	if (len != sizeof(msg_head_t) + rev_net_msg.msg_head.msg_size) {
		udtc_close(sockfd);
		return -1;
	}
	if (rev_net_msg.msg_head.msg_type != MSG_DEFAULTOUTPUT_SETING
			|| rev_net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		udtc_close(sockfd);
		return -1;
	}

	udtc_close(sockfd);
	return 0;
}
//双向透明传输
int st_net_startBidiTransparentCmd(unsigned long user_id,
		msg_transparent_start_t msg_transparent_start) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t cache_net_msg;

	if (!pConnect)
		return -1;

	memcpy(&pConnect->bidi_transparent.transparent_start.searial_info,
			&msg_transparent_start, sizeof(msg_transparent_start_t));
	if (SendNetMsg(pConnect->msg_sock, MSG_START_TRANSPARENT, 0,
			(char *) &pConnect->bidi_transparent.transparent_start,
			sizeof(transparent_start_t)) == -1)
		return -1;

	pConnect->interval_valid[INDEX_MSG_START_TRANSPARENT] = TRUE;
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_START_TRANSPARENT],
			5 * 1000) == 0) {
		if (GetMsg(pConnect, INDEX_MSG_START_TRANSPARENT, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
		memcpy(&pConnect->bidi_transparent.transparent_start,
				cache_net_msg.msg_data, sizeof(transparent_start_t));
	} else {
		pConnect->interval_valid[INDEX_MSG_START_TRANSPARENT] = FALSE;
		return -1;
	}

	return 0;
}
int st_net_stopBidiTransparentCmd(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->bidi_transparent.bidi_ransparent_callback = NULL;
	return SendNetMsg(pConnect->msg_sock, MSG_STOP_TRANSPARENT, 0, NULL, 0);
}
int st_net_sendBidiTransparentCmd(unsigned long user_id,
		msg_transparent_send_t msg_transparent_send) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	return SetServInfo(user_id, MSG_SEND_TRANSPARENT, 0,
			INDEX_MSG_SEND_TRANSPARENT, (void*) &msg_transparent_send,
			sizeof(msg_transparent_send_t));
}
//////////////////////////////////////////////////////////////////////////
int st_net_registerServerMsgCallback(unsigned long user_id,
		SERVER_MSG_CALLBACK callback, void *context) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->msg_callback = callback;
	pConnect->msg_callback_context = context;
	return 0;
}
int st_net_registerServerAlarmInfoCallback(unsigned long user_id,
		ALARM_INFO_CALLBACK callback, void *context) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if (NULL == pServerInfo)
		return -1;

	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->alarm_info_callback = callback;
	pConnect->alarm_info_callback_context = context;
	//__android_log_print(ANDROID_LOG_DEBUG, "--------MSG_ALARM_INFO callback------- register",__FUNCTION__);
	return 0;
}
int st_net_registerUpdateStatusCallback(unsigned long user_id,
		UPDATE_STATUS_CALLBACK callback, void *context) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->update_status_callback = callback;
	pConnect->update_status_callback_context = context;
	return 0;
}

int st_net_registerTalkStreamCallback(unsigned long user_id,
		TALK_STREAM_CALLBACK callback, void *context) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->dual_talk.talk_callback = callback;
	pConnect->dual_talk.context = context;
	return 0;
}

int st_net_registerTransparentCmdCallback(unsigned long user_id,
		BIDI_TRANSPARENT_CALLBACK callback, void *context) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!pConnect)
		return -1;

	pConnect->bidi_transparent.bidi_ransparent_callback = callback;
	pConnect->bidi_transparent.context = context;
	return 0;
}
int st_net_getLoginStatus(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	return pServerInfo->iLogin;
}
int zte_start_pu_handlerecord() {
	return 0;
}
int zte_stop_pu_handlerecord() {
	return 0;
}
static int st_net_getLocaltime(msg_time_t *msg_time) {
	time_t tv;
	struct tm *sTime;

	tv = time(NULL);
	sTime = localtime(&tv);
	msg_time->year = sTime->tm_year + 1900;
	msg_time->month = sTime->tm_mon + 1;
	msg_time->day = sTime->tm_mday;
	msg_time->hour = sTime->tm_hour;
	msg_time->minute = sTime->tm_min;
	msg_time->second = sTime->tm_sec;
	return 0;
}
int st_net_inputFileCmd(unsigned long user_id, char *file_name) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t net_msg;
	FILE *fp = NULL;
	param_file_head_t param_file_head;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_INPUTFILE_CONFIG] = TRUE;
	if ((fp = fopen(file_name, "rb+")) == NULL)
		return -1;
	if (fread(&param_file_head, 1, sizeof(param_file_head_t), fp)
			!= sizeof(param_file_head_t))
		return -1;
	fread(net_msg.msg_data, 1, param_file_head.file_size, fp);
	SendNetMsg(pConnect->msg_sock, MSG_INPUTFILE_CONFIG, 0, net_msg.msg_data,
			param_file_head.file_size);
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_INPUTFILE_CONFIG],
	OVER_TIME) == 0) {
		if (GetMsg(pConnect, INDEX_INPUTFILE_CONFIG, &net_msg) < 0)
			return -1;
		if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
			return -1;
	} else {
		pConnect->interval_valid[INDEX_INPUTFILE_CONFIG] = FALSE;
		return -1;
	}
	return 0;
}
int st_net_outputFileCmd(unsigned long user_id, char *file_name) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	net_msg_t net_msg;
	FILE *fp = NULL;

	if (!pConnect)
		return -1;

	pConnect->interval_valid[INDEX_OUTPUTFILE_CONFIG] = TRUE;
	if (SendNetMsg(pConnect->msg_sock, MSG_OUTPUTFILE_CONFIG, 0, NULL, 0) < 0)
		return -1;
	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_OUTPUTFILE_CONFIG],
	OVER_TIME) == 0) {
		param_file_head_t param_file_head;

		if (GetMsg(pConnect, INDEX_OUTPUTFILE_CONFIG, &net_msg) < 0)
			return -1;
		if ((fp = fopen(file_name, "wb+")) == NULL)
			return -1;

		param_file_head.dev_type = pConnect->device_type;
		param_file_head.file_size = net_msg.msg_head.msg_size;
		param_file_head.file_type = net_msg.msg_head.msg_type;
		fwrite(&param_file_head, 1, sizeof(param_file_head), fp);
		fwrite(net_msg.msg_data, 1, net_msg.msg_head.msg_size, fp);
		fclose(fp);
	} else {
		pConnect->interval_valid[INDEX_OUTPUTFILE_CONFIG] = FALSE;
		return -1;
	}
	return 0;
}
void *RecvServInfoThread(void *param) {
	net_msg_t net_msg;
	search_device_param_t *p_search_device_param =
			(search_device_param_t*) param;
	p_search_device_param->device_num = 0;

	pthread_detach(pthread_self()); //线程分离.

	while (1) {
		struct timeval tval;
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(p_search_device_param->sockfd, &rset);
		tval.tv_sec = 2;
		tval.tv_usec = 0;
		if (select(p_search_device_param->sockfd + 1, &rset, 0, 0, &tval) <= 0)
			return NULL;
		if (recvfrom(p_search_device_param->sockfd, (char *) &net_msg,
				sizeof(net_msg_t), 0, NULL, NULL) != sizeof(net_msg_t))
			continue;
		memcpy(
				(msg_broadcast_t*) (p_search_device_param->pData)
						+ p_search_device_param->device_num,
				(char *) net_msg.msg_data, sizeof(msg_broadcast_t));
		p_search_device_param->device_num++;
	}
	return NULL;
}
int st_net_registerZTERegisterCallback(REGISTER_CALLBACK callback,
		void *context) {
	g_register_callback = callback;
	return 0;
}

int st_net_stopRemotePlay(unsigned long remote_play_id) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;

	if (remote_play == NULL)
		return -1;

//	remote_play->run_flag = 0;
//	Sleep(500);
//	WaitForSingleObject(remote_play->hRomtePlayThread,1000);
//	int iRet = CloseHandle(remote_play->hRomtePlayThread);
//	if (iRet == 0)
//	{
//		AfxMessageBox("关闭点播线程失败！");
//		return -1;
//	}
	remote_play->run_flag = 0;
	if (PROTOCOL_TCP == g_protocol)
		close(remote_play->remote_play_sock);
	else
		udtc_close(remote_play->remote_play_sock);
	if (NULL != remote_play) {
		free(remote_play);
		remote_play = NULL;
	}
	return 0;
}

static void *remotePlayThread(void *param) {
	remote_play_t *remote_play = (remote_play_t *) param;
	unsigned long user_id = remote_play->user_id;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	msg_remote_play_position_t msg_remote_play_position;
	net_pack_t net_pack;
	net_msg_t net_msg;
	LOGI("跑跑跑");
	int frame_num;
	int tcp_sock;
	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char full_frame_data[DATA_BUF_LEN];
	struct timeval tval;
	fd_set rset;
	int status;

	pthread_detach(pthread_self()); //线程分离.

	if (!pConnect)
		return NULL;

	tcp_sock = remote_play->remote_play_sock;
	frame_num = remote_play->frame_num;

	msg_remote_play_position.flag = 0;
	msg_remote_play_position.play_position = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);
	status = 1;
	isfull = 0;
	while (remote_play->run_flag) {
//		if(frame_num <= 0) //暂时去掉
//		{
//			TRACE("frame num < 0 ");
//			if (pConnect->playrecord_decode_callback != NULL)
//			{
//				pConnect->playrecord_decode_callback(NULL, 0, pConnect->playrecord_status_callback_context);
//			}
//			break;
//		}
//		LOGI("ISFULL:__%d",isfull);
//		while(isfull)
//		{
////			LOGI("满了");
//		}
//		LOGI("没满");
		if(stop==1)
		{
			LOGI("退出了");
			break;
		}
		if (status == 1) {
			struct timeval tval1;
			fd_set wset1;
			FD_ZERO(&wset1);
			FD_SET((unsigned int) tcp_sock, &wset1);
			tval1.tv_sec = 0;
			tval1.tv_usec = 100 * 1000;
			if (select((unsigned int) tcp_sock + 1, 0, &wset1, 0, &tval1)
					<= 0) {
				if (remote_play->playrecord_decode_callback != NULL)
					remote_play->playrecord_decode_callback(NULL, 0,
							remote_play->playrecord_status_callback_context);
				LOGE("remote play time out!");
				remote_play->run_flag = 0;
				continue;
			}
//			LOGI("跑跑跑2");
			if (Send(tcp_sock, (char*) &net_msg,
					sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
					!= sizeof(msg_head_t)
							+ sizeof(msg_remote_play_position_t)) {
				if (remote_play->playrecord_decode_callback != NULL)
					remote_play->playrecord_decode_callback(NULL, 0,
							remote_play->playrecord_decode_callback_context);
				remote_play->run_flag = 0;
				continue;
			}
			LOGI("request data seuuess!");
			//frame_num = frame_num - 100;
			status = 0;
		}

		FD_ZERO(&rset);
		FD_SET((unsigned int) tcp_sock, &rset);
		tval.tv_sec = 0;
		tval.tv_usec = 500 * 1000;
//		LOGI("跑跑跑3");
//		LOGI("tcp_sock__:%d",tcp_sock);
		if (select((unsigned int) tcp_sock + 1, &rset, 0, 0, &tval) <= 0) {
//			void *context = pConnect->playrecord_status_callback_context;
			if (remote_play->playrecord_status_callback != NULL)
				status = remote_play->playrecord_status_callback(
						remote_play->playrecord_status_callback_context);
			LOGE("data not enough-----!");
			continue;
		}
		if (Recv(tcp_sock, (char*) &net_pack, sizeof(pack_head_t), 0)
				!= sizeof(pack_head_t)) {
			if (remote_play->playrecord_decode_callback != NULL)
				remote_play->playrecord_decode_callback(NULL, 0,
						remote_play->playrecord_decode_callback_context);
			LOGE("remote play udtc_recv head err!");
			remote_play->run_flag = 0;
			continue;
		}
		if (first_frame == 0) {
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}
		//TRACE("!!!!!!!!!!!!!!!!!!!!!!!!!!! frame size %d, pack size %d, pack no %d", net_pack.pack_head.frame_size, net_pack.pack_head.pack_size, net_pack.pack_head.pack_no);
		if (Recv(tcp_sock, (char*) &net_pack + sizeof(pack_head_t),
				net_pack.pack_head.pack_size, 0)
				!= (int) net_pack.pack_head.pack_size) {
			if (remote_play->playrecord_decode_callback != NULL)
				remote_play->playrecord_decode_callback(NULL, 0,
						remote_play->playrecord_decode_callback_context);
			LOGE("remote play udtc_recv data err!");
			remote_play->run_flag = 0;
			continue;
		}
		if (frame_no == net_pack.pack_head.frame_no) {
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
		} else //满一帧
		{
//			LOGI("得到一帧数");
			if (full_frame_size + (int) sizeof(frame_head_t) == recv_frame_size)
				if (remote_play->playrecord_decode_callback != NULL)
					remote_play->playrecord_decode_callback(full_frame_data,
							recv_frame_size,
							remote_play->playrecord_decode_callback_context);
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}
	}
	LOGI("remote play thread end!");

	close(tcp_sock);
	LOGI("------------udtc_close---------");

	return NULL;
}

unsigned long st_net_startRemotePlay(unsigned long user_id,
		msg_play_record_t msg_play_record, msg_recordplay_t *msg_recordplay) {
	remote_play_t *remote_play = NULL;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	struct sockaddr_in RemoteAddr;
	msg_session_id_t msg_session_id;
	net_msg_t net_msg;
	int len;
	int tcp_sock = -1;
	int frame_num;
	char file_name[MSG_RECORD_FILENAME_LEN];

	if (!pConnect)
		return -1;
	if ((remote_play = (remote_play_t*) malloc(sizeof(remote_play_t))) == NULL)
		return (-1);
	remote_play->playrecord_decode_callback = NULL;
	remote_play->playrecord_status_callback = NULL;

	strcpy(file_name, msg_play_record.file_name);
	if (pConnect == NULL || file_name == NULL)
		goto error;

	if ((tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		goto error;

	if (tcp_sock <= 0) {
		LOGE("err1 st_net_startRemotePlay connect");
		goto error;
	}
	len = DATA_BUF_LEN;
	if (udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1)
		goto error;
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//add by fengzx 2011-07-05
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);

	if (udtc_connect(tcp_sock, (struct sockaddr*) &RemoteAddr,
			sizeof(RemoteAddr)) < 0) {
		LOGE("udtc_connect-startRemotePlay-");
		goto error;
	}
	remote_play->remote_play_sock = tcp_sock;

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	strcpy(net_msg.msg_data, file_name);
	net_msg.msg_head.msg_size = sizeof(msg_play_record_t);
	LOGI("startRemotePlay send 2");
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_play_record_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_play_record_t))
		goto error;
	//接收应答
	if (udtc_recv(tcp_sock, (char*) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(tcp_sock, (char*) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
	frame_num = msg_recordplay->total_frame_num;
	remote_play->user_id = user_id;
	remote_play->msg_play_record = msg_play_record;
	remote_play->run_flag = 1;
	remote_play->remote_play_sock = tcp_sock;
	remote_play->frame_num = frame_num;
	if (frame_num <= 0)
		goto error;
	pthread_create(&remote_play->hRomtePlayThread_id, NULL, remotePlayThread,
			(void*) remote_play);
	return (unsigned long) remote_play;

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
	}
	tcp_sock = -1;
	if (remote_play != NULL)
		free(remote_play);
	remote_play = NULL;
	return (-1);
}
unsigned long st_net_startRemotePlayByTime(unsigned long user_id,
		msg_fragment_t msg_play_record, msg_recordplay_t *msg_recordplay) {
	remote_play_t *remote_play = NULL;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int len;
	int tcp_sock = -1;
	net_msg_t net_msg;
	struct sockaddr_in RemoteAddr;
	msg_session_id_t msg_session_id;
	int frame_num;

	if (!pConnect)
		return -1;
	if ((remote_play = (remote_play_t *) malloc(sizeof(remote_play_t))) == NULL)
		return -1;
	remote_play->playrecord_decode_callback = NULL;
	remote_play->playrecord_status_callback = NULL;

	if (pConnect == NULL)
		goto error;

	if ((tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		goto error;

	if (tcp_sock <= 0) {
		LOGE("err1 st_net_startRemotePlayByTime connect");
		goto error;
	}
	len = DATA_BUF_LEN;

	if (udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1)
		goto error;

	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	RemoteAddr.sin_port = htons(pConnect->port + 1);
	LOGI("错误");
	if (udtc_connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr)) < 0) {
		LOGE("startRemotePlay--udtc_connect-2");
		goto error;
	}
	remote_play->remote_play_sock = tcp_sock;

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(net_msg.msg_data, &msg_play_record, sizeof(msg_fragment_t));
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_fragment_t))
		goto error;
	//接收应答
	if (udtc_recv(tcp_sock, (char*) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(tcp_sock, (char*) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
	frame_num = msg_recordplay->total_frame_num;
	remote_play->user_id = user_id;
	remote_play->run_flag = 1;
	remote_play->remote_play_sock = tcp_sock;
	remote_play->frame_num = frame_num;
	if (frame_num <= 0)
		goto error;
	pthread_create(&remote_play->hRomtePlayThread_id, NULL, remotePlayThread,
			(void*) remote_play);
	return (unsigned long) remote_play;

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
	}
	tcp_sock = -1;
	if (remote_play != NULL)
		free(remote_play);
	remote_play = NULL;
	return (-1);
}

int st_net_startRemotePlayEx(long user_id, msg_fragment_t msg_play_record,
		msg_recordplay_t *msg_recordplay) {
	remote_play_t *remote_play = NULL;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	remote_play = (remote_play_t *) malloc(sizeof(remote_play_t));
	if (remote_play == NULL) {
		return -1;
	}
	int len;
	int tcp_sock;
	net_msg_t net_msg;
	struct sockaddr_in RemoteAddr;
	msg_session_id_t msg_session_id;
	int frame_num;

	if (!pConnect)
		return -1;

//	remote_play->hRomtePlayThread_id = NULL;
	remote_play->playrecord_decode_callback = NULL;
	remote_play->playrecord_status_callback = NULL;

	if (pConnect == NULL)
		goto error;

	tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	LOGI("真机走到这里了");
	if (tcp_sock <= 0) {
		LOGE("err1 st_net_startRemotePlayByTime connect");
		goto error;
	}
	len = DATA_BUF_LEN;

	//setsockopt()
	if (setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1)
		goto error;

	RemoteAddr.sin_family = AF_INET; //2
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
//	LOGI("错误1");
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	LOGI("s_addr__:%s", pConnect->ip_addr);
	LOGI("sin_port:%d", RemoteAddr.sin_port);
	LOGI("tcp_sock:__%d", tcp_sock);
	remote_play->remote_play_sock = tcp_sock;
	LOGI("remote_play_sock__:%d", remote_play->remote_play_sock);
	int ret = connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr));
	LOGI("ret:__%d", ret);
	if (ret < 0) {
		LOGE("startRemotePlay--udtc_connect-2");
		goto error;
	}
//	LOGI("错误1.3");

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
//	LOGI("错误1.5");
	if (Recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;

//	LOGI("错误2");
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;

	memcpy(net_msg.msg_data, &msg_play_record, sizeof(msg_fragment_t));
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	if (Send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_fragment_t))
		goto error;
	//接收应答

//	LOGI("错误3");
	if (Recv(tcp_sock, (char*) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (recv(tcp_sock, (char*) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

//	LOGI("错误4");
	LOGI("msg_data__:%s", net_msg.msg_data);
	memcpy(msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
//	LOGI("错误4.1");
	frame_num = msg_recordplay->total_frame_num;
//	LOGI("错误4.2");
	remote_play->user_id = user_id;
//	LOGI("错误4.3");
	remote_play->run_flag = 1;
//	remote_play->remote_play_sock = tcp_sock;

	remote_play->frame_num = frame_num;
	if (frame_num <= 0)
		goto error;
//	LOGI("错误5");
	pthread_create(&remote_play->hRomtePlayThread_id, NULL, remotePlayThread,
			(void*) remote_play);
//	LOGI("错误6");
	LOGI("remote_play__:%d", remote_play);
	LOGI("remote_play->frame_num:__%d", remote_play->frame_num);
	return remote_play;

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
	}
	tcp_sock = -1;
	if (remote_play != NULL)
		free(remote_play);
	remote_play = NULL;
	return (-1);
}

int Send(int fd, char *vptr, int n, int flag) {
	int nleft;
	int nwritten;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = send(fd, ptr, nleft, flag)) <= 0) {
			if (errno == 10004)
				nwritten = 0;
			else
				return (-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft);
}

int st_net_stopRemotePlayByTime(unsigned long remote_play_id) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	if (remote_play == NULL)
		return -1;

	remote_play->run_flag = 0;
	stop = 1;
	close(remote_play->remote_play_sock);
	if (NULL != remote_play) {
		free(remote_play);
		remote_play = NULL;
	}
	return 0;
}
int st_net_RemotePlayMode(unsigned long remote_play_id, int current_frameno,
		int flag) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	net_msg_t net_msg;
	msg_remote_play_position_t msg_remote_play_position;
	int tcp_sock;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) remote_play->user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	if (!remote_play)
		return -1;
	if (!pConnect)
		return -1;

	tcp_sock = remote_play->remote_play_sock;
	net_msg.msg_head.ack_flag = ACK_SUCCESS;
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	msg_remote_play_position.flag = flag;
	msg_remote_play_position.play_position = current_frameno;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_remote_play_position_t))
		return -1;
	return 0;
}
int st_net_RemotePlayPosition(unsigned long remote_play_id, int current_playpos) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) remote_play->user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int tcp_sock;
	net_msg_t net_msg;
	msg_remote_play_position_t msg_remote_play_position;

	if (remote_play == NULL)
		return -1;

	if (!pConnect)
		return -1;

	tcp_sock = remote_play->remote_play_sock;
	net_msg.msg_head.ack_flag = ACK_SUCCESS;
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	msg_remote_play_position.flag = 1;
	msg_remote_play_position.play_position = current_playpos;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	LOGI("current_playpos %d 1", current_playpos);
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_remote_play_position_t))
		return -1;
	return 0;
}
int st_net_RemotePlayPositionByTime(unsigned long remote_play_id,
		int current_playpos) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	tp_server_info_t *pServerInfo = (tp_server_info_t *) remote_play->user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	int tcp_sock;
	net_msg_t net_msg;
	msg_remote_play_position_t msg_remote_play_position;

	if (remote_play == NULL)
		return -1;

	if (!pConnect)
		return -1;

	tcp_sock = remote_play->remote_play_sock;
	net_msg.msg_head.ack_flag = ACK_SUCCESS;
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	msg_remote_play_position.flag = 1;
	msg_remote_play_position.play_position = current_playpos;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	LOGI("current_playpos %d 2", current_playpos);
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_remote_play_position_t))
		return -1;
	return 0;
}

int st_net_startDownloadFileEx(unsigned long user_id, msg_fragment_t info,
		char *savefilename, msg_file_down_Status *file_down_Status) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;

	int len;
	int iRet;

	net_pack_t net_pack;
	net_msg_t net_msg;
	struct timeval tval;
	fd_set rset;

	if (pConnect == NULL || savefilename == NULL) {
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	memset(file_down_Status, 0, sizeof(msg_file_down_Status));

	struct sockaddr_in RemoteAddr;
	pConnect->downloadfile_tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	len = DATA_BUF_LEN;
	if (setsockopt(pConnect->downloadfile_tcp_sock, SOL_SOCKET, SO_RCVBUF,
			(const char *) &len, sizeof(len)) == -1) {
		return -1;
	}

	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);

	file_down_Status->iSocket = pConnect->downloadfile_tcp_sock;

	iRet = connect(pConnect->downloadfile_tcp_sock,
			(struct sockaddr *) &RemoteAddr, sizeof(RemoteAddr));
	if (iRet != 0) {
		return -1;
	}
	tcp_sock_info_t *tcp_sock_info;
	tcp_sock_info = (tcp_sock_info_t*) malloc(sizeof(tcp_sock_info_t));
	tcp_sock_info->sock_fd = pConnect->downloadfile_tcp_sock;
	tcp_sock_info->next = NULL;

	addTcpSockInfo(pConnect, tcp_sock_info);

	msg_session_id_t msg_session_id;
	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(pConnect->downloadfile_tcp_sock, MSG_SESSION_ID, 0,
			(char *) &msg_session_id, sizeof(msg_session_id_t));
	iRet = recv(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t), 0);
	if (iRet != sizeof(msg_head_t)) {
		delTcpSockInfo(pConnect, tcp_sock_info);
		return -1;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		delTcpSockInfo(pConnect, tcp_sock_info);
		return -1;
	}
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_DOWNLOAD_RECORD;
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	memcpy(net_msg.msg_data, &info, sizeof(msg_fragment_t));
	iRet = send(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0);
	if (iRet != sizeof(msg_head_t) + sizeof(msg_fragment_t)) {
		delTcpSockInfo(pConnect, tcp_sock_info);
		return -1;
	}
	//接收应答
	iRet = recv(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t), 0);
	if (iRet != sizeof(msg_head_t)) {
		delTcpSockInfo(pConnect, tcp_sock_info);
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		delTcpSockInfo(pConnect, tcp_sock_info);
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	iRet = recv(pConnect->downloadfile_tcp_sock,
			(char *) &net_msg + sizeof(msg_head_t), net_msg.msg_head.msg_size,
			0);
	if (iRet != (int) net_msg.msg_head.msg_size) {
//		delTcpSockInfo(pConnect,tcp_sock_info);
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	msg_filesize_t msg_filesize;
	memcpy(&msg_filesize, net_msg.msg_data, sizeof(msg_filesize_t));
	//	unsigned long iFileSize;
	pConnect->iDownloadFileSize = msg_filesize.filesize;
	file_down_Status->iFileSize = msg_filesize.filesize;
	msg_progress = (msg_file_down_Status *) malloc(
			sizeof(msg_file_down_Status));
	memset(msg_progress, 0, sizeof(msg_file_down_Status));
	msg_progress[0].iFileSize = msg_filesize.filesize;
	LOGI("********* download file size %lu", msg_filesize.filesize);

	//取得文件大小后，关闭download socket
	close(pConnect->downloadfile_tcp_sock);
	delTcpSockInfo(pConnect, tcp_sock_info);

	//按远程点播来发命令
	int tcp_sock;
	char file_name[MSG_RECORD_FILENAME_LEN];

	strcpy(file_name, info.file_name);
	if (file_name == NULL) {
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	len = 512 * 1024;
	if (setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	int nPortOffset1 = 0;
	if (pConnect->bPortOffset) {
		nPortOffset1 = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset1);

	iRet = connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr));
	if (iRet < 0) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	iRet = recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0);
	if (iRet != sizeof(msg_head_t)) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(net_msg.msg_data, &info, sizeof(msg_fragment_t));
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	iRet = send(tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0);
	if (iRet != sizeof(msg_head_t) + sizeof(msg_fragment_t)) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}

	//接收应答
	iRet = recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0);
	if (iRet != sizeof(msg_head_t)) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	iRet = recv(tcp_sock, (char *) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0);
	if (iRet != (int) net_msg.msg_head.msg_size) {
		close(tcp_sock);
		pConnect->downloadfile_tcp_sock = 0;
//		SetEvent(pConnect->hLogoutEvent);
		return -1;
	}
	msg_recordplay_t msg_recordplay;
	memcpy(&msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
	int frame_num = msg_recordplay.total_frame_num;
//	int first_frame_no = msg_recordplay.first_frame_no;
	pConnect->downloadfile_tcp_sock = tcp_sock; //20090817
	if (frame_num <= 0) {

		return -1;
	}
	if ((pConnect->downloadfile = (FILE *) fopen(savefilename, "wb+")) == NULL) {

		return -1;
	}

	unsigned long i = 0;
	msg_remote_play_position_t msg_remote_play_position;

	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char full_frame_data[512 * 1024];

	msg_remote_play_position.flag = 0;
	msg_remote_play_position.play_position = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);

	int status = 1;
	//接收数据
	LOGI("pConnect->iDownloadFileSize__:%d", pConnect->iDownloadFileSize);
	while (pConnect->iDownloadFileSize > 100) {
		msg_progress[0].iTransfersSize = pConnect->iDownloadFileSize;
		if(stopdownload)
		{
			break;
		}
		if (status == 1) {
			struct timeval tval1;
			fd_set wset1;
			FD_ZERO(&wset1);
			FD_SET((unsigned int) tcp_sock, &wset1);
			tval1.tv_sec = 0;
			tval1.tv_usec = 200 * 1000; //100*1000
			if (select((unsigned int) tcp_sock + 1, 0, &wset1, 0, &tval1) <= 0)
				continue;
			LOGI("downloadFileByTime send 7");
			if (send(tcp_sock, (char *) &net_msg,
					sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
					!= sizeof(msg_head_t)
							+ sizeof(msg_remote_play_position_t)) {
				LOGE(" --- send error: ------");
				continue;
			}
			status = 0;
		}

		FD_ZERO(&rset);
		FD_SET((unsigned int) tcp_sock, &rset);
		tval.tv_sec = 0;
		tval.tv_usec = 200 * 1000; //100*1000
		if (select((unsigned int) tcp_sock + 1, &rset, 0, 0, &tval) <= 0) {
			status = 1;
			continue;
		}

		if (Recv(tcp_sock, (char*) &net_pack, sizeof(pack_head_t), 0)
				!= sizeof(pack_head_t)) {
			LOGI(" ----udtc_recv error: -------");
			break;
		}

		if (first_frame == 0) {
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

		//		TRACE("!!!!!!!!!!!!!!!!!!!!!!!!!!! frame size %d, pack size %d, pack no %d", net_pack.pack_head.frame_size, net_pack.pack_head.pack_size, net_pack.pack_head.pack_no);

		if (Recv(tcp_sock, (char*) &net_pack + sizeof(pack_head_t),
				net_pack.pack_head.pack_size, 0)
				!= (int) net_pack.pack_head.pack_size) {
//			LOGI("不一样");
		}
		if (frame_no == net_pack.pack_head.frame_no) {
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
		} else //满一帧
		{
			//TRACE("************************************************** udtc_recv size %d", recv_frame_size);
			if (full_frame_size + (int) sizeof(frame_head_t)
					== recv_frame_size) {
				fwrite(full_frame_data, 1, recv_frame_size,
						(void *) pConnect->downloadfile);
				//				AVI_fwrite((void *)pConnect->downloadfile, full_frame_data);
			}

			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;

			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}

		file_down_Status->iTransfersSize += net_pack.pack_head.pack_size;
		pConnect->iDownloadFileSize -= net_pack.pack_head.pack_size;
		//		TRACE("================ download size:%d =============", pConnect->iDownloadFileSize);
		//////////////////////////////////////////////////////////////////////////
	}
	LOGI("!!!!!!!!!!!!!!!!!!!!!!!!!!!end");
	if (pConnect->downloadfile) {
		fclose((void *) pConnect->downloadfile);
		//		AVI_fclose((void *)pConnect->downloadfile);
		pConnect->downloadfile = NULL;
	}
	close(tcp_sock);
	pConnect->downloadfile_tcp_sock = -1;
	return 0;

}
int st_net_stopdownrecord(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	if(pServerInfo == NULL)
	{
		return -1;
	}
	server_connect_t *pConnect = pServerInfo->server_connect;
	if(pConnect == NULL)
	{
		return -1;
	}
	int tcp_sock;
	stopdownload = 1;
	tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	close(tcp_sock);
	pConnect->downloadfile_tcp_sock = -1;
	return 0;
}
int st_net_getprogress(msg_file_down_Status *down_progress) {
	if (msg_progress == NULL) {
		return -1;
	}
	memcpy(down_progress, msg_progress, sizeof(msg_file_down_Status));
	return 0;
}
int Recv(int fd, char *vptr, int n, int flag) {
	int nleft, nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = recv(fd, ptr, nleft, flag)) < 0) {
			if (errno == 10004)
				nread = 0;
			else
				return (-1);
		} else if (nread == 0) {
			break; //EOF
		}
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);
}
int st_net_downloadFileByTime(unsigned long user_id, msg_fragment_t info,
		char *savefilename, msg_file_down_Status* file_down_Status) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	LOGI("错误3");
	int len;
	net_pack_t net_pack;
	net_msg_t net_msg;
	struct sockaddr_in RemoteAddr;
	tcp_sock_info_t *tcp_sock_info = NULL;
	msg_session_id_t msg_session_id;
	msg_filesize_t msg_filesize;
	int frame_num;
	msg_recordplay_t msg_recordplay;
	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char full_frame_data[DATA_BUF_LEN];
	msg_remote_play_position_t msg_remote_play_position;
	int status;
	struct timeval tval;
	fd_set rset;

	int tcp_sock = -1;
	char file_name[MSG_RECORD_FILENAME_LEN];

	if (!pConnect)
		return -1;
	if (pConnect == NULL || savefilename == NULL)
		return -1;
	memset(file_down_Status, 0, sizeof(msg_file_down_Status));
	LOGI("错误4");
	if ((pConnect->downloadfile_tcp_sock = udtc_socket(AF_INET, SOCK_STREAM,
			IPPROTO_TCP)) == 1)
		return (-1);
	LOGI("错误5");
	len = DATA_BUF_LEN;
	if (udtc_setsockopt(pConnect->downloadfile_tcp_sock, SOL_SOCKET, SO_RCVBUF,
			(const char*) &len, sizeof(len)) == -1)
		goto error;
	LOGI("错误6");
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	//end add
	file_down_Status->iSocket = pConnect->downloadfile_tcp_sock;
	LOGI("错误7");
	if (udtc_connect(pConnect->downloadfile_tcp_sock,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr)) != 0) {
		LOGE("---udtc_connect--downloadfile-");
		goto error;
	}
	LOGI("错误8");
	tcp_sock_info = (tcp_sock_info_t*) malloc(sizeof(tcp_sock_info_t));
	tcp_sock_info->sock_fd = pConnect->downloadfile_tcp_sock;
	tcp_sock_info->next = NULL;
	addTcpSockInfo(pConnect, tcp_sock_info);

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(pConnect->downloadfile_tcp_sock, MSG_SESSION_ID, 0,
			(char *) &msg_session_id, sizeof(msg_session_id_t));
	LOGI("错误8");
	if (udtc_recv(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t), 0) != sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	LOGI("错误10");
	net_msg.msg_head.msg_type = TCP_TYPE_DOWNLOAD_RECORD;
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	memcpy(net_msg.msg_data, &info, sizeof(msg_fragment_t));
	LOGI("downloadFileByTime send 5");
	if (udtc_send(pConnect->downloadfile_tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_fragment_t))
		goto error;
	//接收应答
	LOGI("错误11");
	if (udtc_recv(pConnect->downloadfile_tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t), 0) != sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;

	LOGI("错误12");
	if (udtc_recv(pConnect->downloadfile_tcp_sock,
			(char*) &net_msg + sizeof(msg_head_t), net_msg.msg_head.msg_size, 0)
			!= (int) net_msg.msg_head.msg_size)
		goto error;

	LOGI("错误13");
	memcpy(&msg_filesize, net_msg.msg_data, sizeof(msg_filesize_t));
	pConnect->iDownloadFileSize = msg_filesize.filesize;
	file_down_Status->iFileSize = msg_filesize.filesize;
	LOGI("********* download file size %lu", msg_filesize.filesize);

	//取得文件大小后，关闭download udtc_socket
	udtc_close(pConnect->downloadfile_tcp_sock);
	LOGI("----udtc_close---");

	pConnect->downloadfile_tcp_sock = -1;
	delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;

	//按远程点播来发命令

	strcpy(file_name, info.file_name);
	if (file_name == NULL)
		goto error;

	if ((tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		goto error;
	len = DATA_BUF_LEN;

	if (udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1)
		goto error;
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	//end add

	if (udtc_connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr)) < 0) {
		LOGE("------------udtc_connect---downloadfile2------");
		goto error;
	}
	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;

	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;

	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
//strcpy(net_msg.msg_data,file_name);
	memcpy(net_msg.msg_data, &info, sizeof(msg_fragment_t));
//memcpy(net_msg.msg_data, &msg_play_record, sizeof(msg_fragment_t));
	net_msg.msg_head.msg_size = sizeof(msg_fragment_t);
	if (udtc_send(tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_fragment_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_fragment_t))
		goto error;

	//接收应答
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(tcp_sock, (char *) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(&msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
	frame_num = msg_recordplay.total_frame_num;
	pConnect->downloadfile_tcp_sock = tcp_sock; //20090817
	if (frame_num <= 0)
		goto error;
//	if ((pConnect->downloadfile = (FILE *)AVI_fopen(savefilename, AVI_WRITE)) == NULL)
	if ((pConnect->downloadfile = (FILE *) fopen(savefilename, "wb+")) == NULL)
		goto error;

	msg_remote_play_position.flag = 0;
	msg_remote_play_position.play_position = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);

	status = 1;
	//接收数据
	while (pConnect->iDownloadFileSize > 0) {
		if (status == 1) {
			struct timeval tval1;
			fd_set wset1;
			FD_ZERO(&wset1);
			FD_SET((unsigned int) tcp_sock, &wset1);
			tval1.tv_sec = 0;
			tval1.tv_usec = 200 * 1000; //100*1000
			if (select((unsigned int) tcp_sock + 1, 0, &wset1, 0, &tval1) <= 0)
				continue;
//			LOGI("downloadFileByTime send 7");
			if (udtc_send(tcp_sock, (char *) &net_msg,
					sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
					!= sizeof(msg_head_t)
							+ sizeof(msg_remote_play_position_t)) {
				LOGE(" --- send error: ------");
				continue;
			}
			status = 0;
		}

		FD_ZERO(&rset);
		FD_SET((unsigned int) tcp_sock, &rset);
		tval.tv_sec = 0;
		tval.tv_usec = 200 * 1000; //100*1000
		if (select((unsigned int) tcp_sock + 1, &rset, 0, 0, &tval) <= 0) {
			status = 1;
			continue;
		}

		if (udtc_recv(tcp_sock, (char*) &net_pack, sizeof(pack_head_t), 0)
				!= sizeof(pack_head_t)) {
			LOGI(" ----udtc_recv error: -------");
			break;
		}

		if (first_frame == 0) {
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

//		TRACE("!!!!!!!!!!!!!!!!!!!!!!!!!!! frame size %d, pack size %d, pack no %d", net_pack.pack_head.frame_size, net_pack.pack_head.pack_size, net_pack.pack_head.pack_no);
		if (udtc_recv(tcp_sock, (char*) &net_pack + sizeof(pack_head_t),
				net_pack.pack_head.pack_size, 0)
				!= (int) net_pack.pack_head.pack_size)
			break;

		if (frame_no == net_pack.pack_head.frame_no) {
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
		} else //满一帧
		{
			//TRACE("************************************************** udtc_recv size %d", recv_frame_size);
			if (full_frame_size + (int) sizeof(frame_head_t)
					== recv_frame_size) {
				fwrite(full_frame_data, 1, recv_frame_size,
						(void *) pConnect->downloadfile);
//				AVI_fwrite((void *)pConnect->downloadfile, full_frame_data);
			}

			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;

			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}

		file_down_Status->iTransfersSize += net_pack.pack_head.pack_size;
		pConnect->iDownloadFileSize -= net_pack.pack_head.pack_size;
//		TRACE("================ download size:%d =============", pConnect->iDownloadFileSize);
		//////////////////////////////////////////////////////////////////////////
	}
	LOGI("!!!!!!!!!!!!!!!!!!!!!!!!!!!end");
	if (pConnect->downloadfile) {
		fclose((void *) pConnect->downloadfile);
//		AVI_fclose((void *)pConnect->downloadfile);
		pConnect->downloadfile = NULL;
	}
	udtc_close(tcp_sock);
	pConnect->downloadfile_tcp_sock = -1;
	return 0;

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
	}
	tcp_sock = -1;
	if (pConnect->downloadfile_tcp_sock > 0) {
		close(pConnect->downloadfile_tcp_sock);
	}

	pConnect->downloadfile_tcp_sock = -1;
	if (pConnect->downloadfile != NULL)
//		AVI_fclose((void *)pConnect->downloadfile);
		fclose((void *) pConnect->downloadfile);
	pConnect->downloadfile = NULL;
	if (tcp_sock_info != NULL)
		delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;
	return (-1);
}
int st_net_downloadFile(unsigned long user_id, msg_download_t info,
		char *savefilename, msg_file_down_Status *file_down_Status) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;

	int len;
	net_pack_t net_pack;
	net_msg_t net_msg;
	struct sockaddr_in RemoteAddr;

	msg_recordplay_t msg_recordplay;
	tcp_sock_info_t *tcp_sock_info = NULL;
	msg_session_id_t msg_session_id;
	msg_filesize_t msg_filesize;
	int tcp_sock = -1;
	char file_name[MSG_RECORD_FILENAME_LEN];
	int frame_num;

	unsigned int frame_no = 0;
	int recv_frame_size = 0;
	int first_frame = 0;
	int full_frame_size = 0;
	int offset = 0;
	char full_frame_data[DATA_BUF_LEN];
	msg_remote_play_position_t msg_remote_play_position;

	int status;

	struct timeval tval;
	fd_set rset;

	if (!pConnect)
		return -1;

	if (pConnect == NULL || savefilename == NULL)
		return -1;
	memset(file_down_Status, 0, sizeof(msg_file_down_Status));
	if ((pConnect->downloadfile_tcp_sock = udtc_socket(AF_INET, SOCK_STREAM,
			IPPROTO_TCP)) == -1)
		return (-1);
	len = DATA_BUF_LEN;
	if (udtc_setsockopt(pConnect->downloadfile_tcp_sock, SOL_SOCKET, SO_RCVBUF,
			(const char *) &len, sizeof(len)) == -1)
		goto error;
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	//end add
	file_down_Status->iSocket = pConnect->downloadfile_tcp_sock;
	if (udtc_connect(pConnect->downloadfile_tcp_sock,
			(struct sockaddr*) &RemoteAddr, sizeof(RemoteAddr)) != 0) {
		LOGE("---udtc_connect--downloadfile3---");
		goto error;
	}

	if ((tcp_sock_info = (tcp_sock_info_t*) malloc(sizeof(tcp_sock_info_t)))
			== NULL)
		goto error;
	tcp_sock_info->sock_fd = pConnect->downloadfile_tcp_sock;
	tcp_sock_info->next = NULL;
	addTcpSockInfo(pConnect, tcp_sock_info);

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(pConnect->downloadfile_tcp_sock, MSG_SESSION_ID, 0,
			(char *) &msg_session_id, sizeof(msg_session_id_t));
	if (udtc_recv(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t), 0) != sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_DOWNLOAD_RECORD;
	net_msg.msg_head.msg_size = sizeof(msg_download_t);
	memcpy(net_msg.msg_data, &info, sizeof(msg_download_t));
	if (udtc_send(pConnect->downloadfile_tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_download_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_download_t))
		goto error;
	//接收应答
	if (udtc_recv(pConnect->downloadfile_tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t), 0) != sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(pConnect->downloadfile_tcp_sock,
			(char*) &net_msg + sizeof(msg_head_t), net_msg.msg_head.msg_size, 0)
			!= (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(&msg_filesize, net_msg.msg_data, sizeof(msg_filesize_t));
	pConnect->iDownloadFileSize = msg_filesize.filesize;
	file_down_Status->iFileSize = msg_filesize.filesize;
	LOGI("********* download file size %lu", msg_filesize.filesize);

	//取得文件大小后，关闭download udtc_socket
	udtc_close(pConnect->downloadfile_tcp_sock);
	pConnect->downloadfile_tcp_sock = -1;
	delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;

	//按远程点播来发命令

	strcpy(file_name, info.filename);
	if (file_name == NULL)
		goto error;

	if ((tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		goto error;
	len = DATA_BUF_LEN;
	if (udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1)
		goto error;
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	if (udtc_connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr)) < 0) {
		LOGE("------------udtc_connect--downloadfile5-------");
		goto error;
	}

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	strcpy(net_msg.msg_data, file_name);
	net_msg.msg_head.msg_size = sizeof(msg_play_record_t);
	LOGI("downloadFile send 9");
	if (udtc_send(tcp_sock, (char *) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_play_record_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_play_record_t))
		goto error;
	//接收应答
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(tcp_sock, (char *) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(&msg_recordplay, net_msg.msg_data, sizeof(msg_recordplay_t));
	frame_num = msg_recordplay.total_frame_num;
	pConnect->downloadfile_tcp_sock = tcp_sock; //20090817
	LOGI("10 !!!!!!!tcp_sock=%d", tcp_sock);
	if (frame_num <= 0)
		goto error;
//	if ((pConnect->downloadfile = (FILE *)AVI_fopen(savefilename, AVI_WRITE)) == NULL)
	if ((pConnect->downloadfile = (FILE *) fopen(savefilename, "wb+")) == NULL)
		goto error;

	msg_remote_play_position.flag = 0;
	msg_remote_play_position.play_position = 0;
	net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
	memcpy(&net_msg.msg_data, &msg_remote_play_position,
			sizeof(msg_remote_play_position_t));
	net_msg.msg_head.msg_size = sizeof(msg_remote_play_position_t);

	status = 1;
	//接收数据
	while (pConnect->iDownloadFileSize > 0) {
		if (status == 1) {
			struct timeval tval1;
			fd_set wset1;
			FD_ZERO(&wset1);
			FD_SET((unsigned int) tcp_sock, &wset1);
			tval1.tv_sec = 0;
			tval1.tv_usec = 10 * 1000; //100*1000
			if (select((unsigned int) tcp_sock + 1, 0, &wset1, 0, &tval1) <= 0)
				continue;
			if (udtc_send(tcp_sock, (char *) &net_msg,
					sizeof(msg_head_t) + sizeof(msg_remote_play_position_t), 0)
					!= sizeof(msg_head_t)
							+ sizeof(msg_remote_play_position_t)) {
				LOGE(" ---------- send error: ----------------");
				continue;
			}
			status = 0;
		}

		FD_ZERO(&rset);
		FD_SET((unsigned int) tcp_sock, &rset);
		tval.tv_sec = 0;
		tval.tv_usec = 10 * 1000; //100*1000
		if (select((unsigned int) tcp_sock + 1, &rset, 0, 0, &tval) <= 0) {
			status = 1;
			continue;
		}

		if (udtc_recv(tcp_sock, (char*) &net_pack, sizeof(pack_head_t), 0)
				!= sizeof(pack_head_t)) {
			LOGE(" ---recv error: -------");
			break;
		}

		if (first_frame == 0) {
			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
			first_frame = 1;
		}

		LOGI(
				"!!!frame no %lu,frame size %lu, pack size %lu, pack no %d", net_pack.pack_head.frame_no, net_pack.pack_head.frame_size, net_pack.pack_head.pack_size, net_pack.pack_head.pack_no);
		if (udtc_recv(tcp_sock, (char*) &net_pack + sizeof(pack_head_t),
				net_pack.pack_head.pack_size, 0)
				!= (int) net_pack.pack_head.pack_size)
			break;

		if (frame_no == net_pack.pack_head.frame_no) {
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size += net_pack.pack_head.pack_size;
		} else //满一帧
		{
			//TRACE("************************************************** udtc_recv size %d", recv_frame_size);
			if (full_frame_size + (int) sizeof(frame_head_t) == recv_frame_size)
//				AVI_fwrite((void *)pConnect->downloadfile, full_frame_data);
				fwrite(full_frame_data, 1, recv_frame_size,
						(void *) pConnect->downloadfile);
			offset = net_pack.pack_head.pack_no * MAX_NET_PACK_SIZE;
			memcpy(&full_frame_data[offset],
					(char*) &net_pack + sizeof(pack_head_t),
					net_pack.pack_head.pack_size);
			recv_frame_size = net_pack.pack_head.pack_size;

			full_frame_size = net_pack.pack_head.frame_size;
			frame_no = net_pack.pack_head.frame_no;
		}

		file_down_Status->iTransfersSize += net_pack.pack_head.pack_size;
		pConnect->iDownloadFileSize -= net_pack.pack_head.pack_size;
		LOGI(
				"================ download size:%lu =============", pConnect->iDownloadFileSize);
		//////////////////////////////////////////////////////////////////////////
	}
	LOGI("!!!!!!!!!!!!!!!!!!!!!!!!!!!end");
	if (pConnect->downloadfile) {
//		AVI_fclose((void *)pConnect->downloadfile);
		fclose((void *) pConnect->downloadfile);
		pConnect->downloadfile = NULL;
	}
	udtc_close(tcp_sock);
	LOGI("------------udtc_close---------");
	pConnect->downloadfile_tcp_sock = -1;
	return 0;

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
		LOGI("------------udtc_close---------");
	}
	tcp_sock = -1;
	if (pConnect->downloadfile_tcp_sock > 0) {
		udtc_close(pConnect->downloadfile_tcp_sock);
		LOGI("------------udtc_close---------");
	}
	pConnect->downloadfile_tcp_sock = -1;
	if (pConnect->downloadfile != NULL)
//		AVI_fclose((void *)pConnect->downloadfile);
		fclose((void *) pConnect->downloadfile);
	pConnect->downloadfile = NULL;
	if (tcp_sock_info != NULL)
		delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;
	return (-1);
}
int st_net_downloadPicture(unsigned long user_id, msg_download_pic_t info,
		char *pic_name) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;

	tcp_sock_info_t *tcp_sock_info = NULL;
	msg_session_id_t msg_session_id;
	msg_download_pic_t msg_download_pic;
	unsigned long iPicSize;
	int pack_size;

	FILE *fp = NULL;
	int tcp_sock = -1;
	int len;
	int iRet;
	net_pack_t net_pack;
	net_msg_t net_msg;
	struct sockaddr_in RemoteAddr;

	if (!pConnect)
		return -1;

	if (pConnect == NULL || pic_name == NULL)
		return -1;
	if ((tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return (-1);
	len = DATA_BUF_LEN;
	if (udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char*) &len,
			sizeof(len)) == -1)
		goto error;
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	//end add
	if (udtc_connect(tcp_sock, (struct sockaddr *) &RemoteAddr,
			sizeof(RemoteAddr)) != 0) {
		LOGE("------------udtc_connect----downloadPicture-----");
		goto error;
	}
	if ((tcp_sock_info = (tcp_sock_info_t*) malloc(sizeof(tcp_sock_info_t)))
			== NULL)
		goto error;
	tcp_sock_info->sock_fd = tcp_sock;
	tcp_sock_info->next = NULL;
	addTcpSockInfo(pConnect, tcp_sock_info);

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(tcp_sock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(tcp_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	//发送请求
	net_msg.msg_head.msg_type = TCP_TYPE_START_DOWNLOAD_PIC;
	net_msg.msg_head.msg_size = sizeof(msg_download_pic_t);
	memcpy(net_msg.msg_data, &info, sizeof(msg_download_pic_t));
	if (udtc_send(tcp_sock, (char*) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_download_pic_t), 0)
			!= sizeof(msg_head_t) + sizeof(msg_download_pic_t))
		goto error;
	//接收应答
	if (udtc_recv(tcp_sock, (char*) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	iRet = udtc_recv(tcp_sock, (char *) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0);
	if (iRet != (int) net_msg.msg_head.msg_size)
		goto error;

	memcpy(&msg_download_pic, net_msg.msg_data, sizeof(msg_download_pic_t));

	iPicSize = msg_download_pic.pic_len;
	if (iPicSize > 500 * 1024 || iPicSize <= 0)
		goto error;

	//打开文件
	if ((fp = fopen(pic_name, "wb+")) == NULL)
		goto error;

	//接收数据
	while (iPicSize > 0) {
		if (udtc_recv(tcp_sock, (char *) &net_pack, sizeof(pack_head_t), 0)
				!= sizeof(pack_head_t))
			goto error;
		if ((pack_size = net_pack.pack_head.pack_size) <= 0)
			goto error;
		if (udtc_recv(tcp_sock, (char*) &net_pack + sizeof(pack_head_t),
				pack_size, 0) != pack_size)
			goto error;
		iPicSize -= pack_size;
		fwrite((char*) &net_pack + sizeof(pack_head_t), 1, pack_size, fp);
		if (iPicSize == 0) {
			fclose(fp);
			udtc_close(tcp_sock);
			LOGI("------------udtc_close---------");
			delTcpSockInfo(pConnect, tcp_sock_info);
			return 0;
		}
	}

	error: if (tcp_sock > 0) {
		udtc_close(tcp_sock);
		LOGI("------------udtc_close---------");
		return -1;
	}

	tcp_sock = -1;
	if (fp != NULL)
		fclose(fp);
	fp = NULL;
	if (tcp_sock_info != NULL)
		delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;
	return -1;
}

int st_net_upgradeSystem(unsigned long user_id, char *filename) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;

	struct stat buf;
	int fh = 0;
	int iTcpSock = 0;
	int iRet;
//	int i;
//	int j;
//	char absolute_filename[1024];
	net_pack_t Pack;
	net_msg_t net_msg;
	msg_upgrade_info_t msg_upgrade_info;
	msg_upgrade_status_t msg_upgrade_status;
	struct sockaddr_in RemoteAddr;

	msg_session_id_t msg_session_id;
	tcp_sock_info_t *tcp_sock_info = NULL;
	int iii = 0;

	if (!pConnect)
		return -1;

	if (pConnect == NULL || filename == NULL)
		return -1;
	/*
	 i = 0;
	 while (*(filename+i) != '\0')
	 i++;
	 j = i;
	 while (*(filename+j) != '//')
	 j--;
	 strncpy(absolute_filename,filename+j+1,i-j);
	 */

	if ((iTcpSock = udtc_socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return (-1);
	memset(&RemoteAddr, 0, sizeof(RemoteAddr));
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
	//RemoteAddr.sin_port        = htons(pConnect->port + 1);
	//add by fengzx 2011-07-05
	int nPortOffset = 0;
	if (pConnect->bPortOffset) {
		nPortOffset = PORT_OFFSET_TCP;
	}
	RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
	//end add
	if (udtc_connect(iTcpSock, (struct sockaddr*) &RemoteAddr,
			sizeof(RemoteAddr)) != 0) {
		LOGE("---udtc_connect--upgradeSystem---");
		goto error;
	}

	msg_session_id.session_id = pConnect->session_id;
	SendNetMsg(iTcpSock, MSG_SESSION_ID, 0, (char *) &msg_session_id,
			sizeof(msg_session_id_t));
	if (udtc_recv(iTcpSock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if ((fh = open(filename, O_RDONLY, 0777)) == -1)
		goto error;

	if ((tcp_sock_info = (tcp_sock_info_t*) malloc(sizeof(tcp_sock_info_t)))
			== NULL)
		goto error;
	tcp_sock_info->sock_fd = iTcpSock;
	tcp_sock_info->next = NULL;
	addTcpSockInfo(pConnect, tcp_sock_info);

	fstat(fh, &buf);
	msg_upgrade_info.filesize = buf.st_size;
	net_msg.msg_head.msg_type = TCP_TYPE_UPGRADE_SYSTEM;
	net_msg.msg_head.msg_size = sizeof(msg_upgrade_info_t);
	memcpy(net_msg.msg_data, &msg_upgrade_info, sizeof(msg_upgrade_info_t));
	//发送请求
	iRet = udtc_send(iTcpSock, (char *) &net_msg,
			sizeof(msg_head_t) + sizeof(msg_upgrade_info_t), 0);
	if (iRet != sizeof(msg_head_t) + sizeof(msg_upgrade_info_t))
		goto error;
	if (udtc_recv(iTcpSock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		goto error;
	if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
		goto error;
	if (udtc_recv(iTcpSock, (char *) &net_msg + sizeof(msg_head_t),
			net_msg.msg_head.msg_size, 0) != (int) net_msg.msg_head.msg_size)
		goto error;

	//发送数据

	while ((Pack.pack_head.pack_size = read(fh, Pack.pack_data,
			MAX_NET_PACK_SIZE)) > 0) {
		Pack.pack_head.pack_no = iii++;
		//发送sizeof(NET_PACK)大小的数据
		udtc_send(iTcpSock, (char *) &Pack,
				sizeof(pack_head_t) + Pack.pack_head.pack_size, 0);
	}

//	LOGI("...............close fh11111 SUSS");
	udtc_close(fh), fh = 0; //close -->udtc_close
//	LOGI("...............close fh22222 SUSS");
	while (1) {

		struct timeval tval1;
		fd_set rset1;
//		LOGI("=========================start sock SUSS");
		FD_ZERO(&rset1);
		FD_SET((unsigned int) iTcpSock, &rset1);
		tval1.tv_sec = 5;
		tval1.tv_usec = 0;
		if (select(iTcpSock + 1, &rset1, 0, 0, &tval1) <= 0)
			goto error;
		iRet = udtc_recv(iTcpSock, (char *) &net_msg,
				sizeof(msg_head_t) + sizeof(msg_upgrade_status_t), 0);
		if (iRet != sizeof(msg_head_t) + sizeof(msg_upgrade_status_t))
			goto error;
		if (net_msg.msg_head.ack_flag == ACK_SUCCESS
				&& net_msg.msg_head.msg_type == TCP_TYPE_UPGRADE_RESULT) {
			memcpy((char *) &msg_upgrade_status, net_msg.msg_data,
					sizeof(msg_upgrade_status_t));
			if (pConnect->update_status_callback != NULL)
				pConnect->update_status_callback(msg_upgrade_status.percent,
						pConnect->update_status_callback_context);
			if (msg_upgrade_status.percent == 100)
				break;
		} else
			goto error;
	}
	udtc_close(iTcpSock);
//	LOGI("========================close sock SUSS");
	delTcpSockInfo(pConnect, tcp_sock_info);
	return 0;

	error:
	LOGE("========================ERROR....");
	if (fh > 0) {
		udtc_close(fh);
		LOGI("------------udtc_close---------");
	}
	fh = 0;
	if (iTcpSock > 0) {
		udtc_close(iTcpSock);
		LOGI("------------udtc_close---------");
	}
	iTcpSock = 0;
	if (tcp_sock_info != NULL)
		delTcpSockInfo(pConnect, tcp_sock_info);
	tcp_sock_info = NULL;
	return (-1);
}
int st_net_registerServerPlayrecordDecodeCallback(unsigned long remote_play_id,
		PLAYRECORD_DECODE_CALLBACK callback, void *context) {
	LOGI("-----st_net_registerServerPlayrecordDecodeCallback ");

	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	LOGI("remote_play->frame_num222:__%d", remote_play->frame_num);
	if (!remote_play)
		return -1;

	remote_play->playrecord_decode_callback = callback;
	remote_play->playrecord_decode_callback_context = context;
	return 0;
}
int st_net_registerServerPlayrecordStatusCallback(unsigned long remote_play_id,
		PLAYRECORD_STATUS_CALLBACK callback, void *context) {
	remote_play_t *remote_play = (remote_play_t *) remote_play_id;
	if (!remote_play)
		return -1;

	remote_play->playrecord_status_callback = callback;
	remote_play->playrecord_status_callback_context = context;
	return 0;
}
int zte_keepaliveliset_is_exist(keepalive_t keepalive) {
	device_info_t *loop;

	pthread_mutex_lock(&hDeviceMutex);
	loop = g_device_info;
	while (loop) {
		if (memcmp(&loop->keepalive, &keepalive, sizeof(keepalive_t)) == 0) {
			pthread_mutex_unlock(&hDeviceMutex);
			return 0;
		}
		loop = loop->next;
	}
	pthread_mutex_unlock(&hDeviceMutex);
	return -1;
}
int zte_keepaliveliset_refresh(keepalive_t keepalive) {
	device_info_t *loop;

	pthread_mutex_lock(&hDeviceMutex);
	loop = g_device_info;
	while (loop) {
		if (memcmp(&loop->keepalive, &keepalive, sizeof(keepalive_t)) == 0) {
			loop->heart_beat_num++;
			loop->refresh_time = time(NULL);
			pthread_mutex_unlock(&hDeviceMutex);
			return 0;
		}
		loop = loop->next;
	}
	pthread_mutex_unlock(&hDeviceMutex);
	return -1;
}
int zte_keepaliveliset_insert(keepalive_t keepalive) {
	LOGI("-----zte_keepaliveliset_insert ");

	device_info_t *loop;
	device_info_t *tmp = NULL;

	pthread_mutex_lock(&hDeviceMutex);
	loop = g_device_info;
	if (loop == NULL) {
		g_device_info = (device_info_t*) malloc(sizeof(device_info_t));
		memset(g_device_info, 0, sizeof(device_info_t));
		memcpy(&g_device_info->keepalive, &keepalive, sizeof(keepalive_t));
		g_device_info->next = NULL;
	} else {
		tmp = (device_info_t*) malloc(sizeof(device_info_t));
		memset(tmp, 0, sizeof(device_info_t));
		memcpy(&tmp->keepalive, &keepalive, sizeof(keepalive_t));
		tmp->next = NULL;
		while (loop->next) {
			loop = loop->next;
		}
		loop->next = tmp;
	}
	pthread_mutex_unlock(&hDeviceMutex);
	return 0;
}
static int zte_deal_netpack(keepalive_t keepalive) {
	LOGI("-----zte_deal_netpack ");

	if (g_register_callback == NULL)
		return -1;
	if (zte_keepaliveliset_is_exist(keepalive) == 0)
		zte_keepaliveliset_refresh(keepalive);
	else {
		zte_keepaliveliset_insert(keepalive);
		zte_keepaliveliset_refresh(keepalive);
		if (g_register_callback != NULL)
			g_register_callback(TRUE, keepalive.ip, keepalive.port,
					keepalive.user_name, keepalive.user_password,
					keepalive.puid);
	}
	return 0;
}
static void *ZTE_OnlineListenThread(void *param) {
	LOGI("-----ZTE_OnlineListenThread ");

	msg_time_t time_info;
	struct sockaddr_in RemoteAddr;
	keepalive_t keepalive;
	int sock = (int) param;
	int preview_flag = 1;

	pthread_detach(pthread_self()); //线程分离.

	while (preview_flag) {
		int Remotelen = sizeof(RemoteAddr);
		if (recvfrom(sock, (char *) &keepalive, sizeof(keepalive), 0,
				(struct sockaddr *) &RemoteAddr, (unsigned int*) &Remotelen)
				== sizeof(keepalive_t))
			zte_deal_netpack(keepalive);
		else {
			udtc_close(sock);
			LOGI("------------udtc_close---------");
			return NULL;
		}
		st_net_getLocaltime(&time_info);
		if (sendto(sock, (char *) &time_info, sizeof(msg_time_t), 0,
				(struct sockaddr *) &RemoteAddr, Remotelen)
				!= sizeof(msg_time_t)) {
			udtc_close(sock);
			LOGI("------------udtc_close---------");
			return NULL;
		}
	}
	return NULL;
}
int zte_start_onlinelisten() {
	LOGI("-----zte_start_onlinelisten ");
	int sock;

	int len = 720 * 576 * 3 / 2;
	int reuse = 1;
	struct sockaddr_in LocalAddr;
	int port_no = 9000;
	pthread_t preview_thread_id;

	if ((sock = udtc_socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return -1;

	if (udtc_setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *) &len,
			sizeof(len)) == -1) {
		udtc_close(sock);
		LOGI("------------udtc_close---------");
		return -1;
	}

	if (udtc_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse,
			sizeof(reuse)) == -1) {
		udtc_close(sock);
		LOGI("------------udtc_close---------");
		return -1;
	}

	LocalAddr.sin_family = PF_INET;
	LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	LocalAddr.sin_port = htons(port_no);

	if (udtc_bind(sock, (struct sockaddr *) &LocalAddr, sizeof(LocalAddr))
			== -1) {
		udtc_close(sock);
		LOGI("------------udtc_close---------");
		return -1;
	}

	pthread_create(&preview_thread_id, NULL, ZTE_OnlineListenThread,
			(void*) sock);
	LOGE("created ZTE_OnlineListenThread!");
	return 0;
}
device_info_t *zte_keepaliveliset_getdel() {
	device_info_t *loop;
	pthread_mutex_lock(&hDeviceMutex);
	loop = g_device_info;
	while (loop) {
		if (time(NULL) - loop->refresh_time > HEART_BEART_DEAD_TIME) {
			pthread_mutex_unlock(&hDeviceMutex);
			return loop;
		}
		loop = loop->next;
	}
	pthread_mutex_unlock(&hDeviceMutex);
	return NULL;
}
int zte_keepaliveliset_delete(device_info_t *device_info) {
	device_info_t *loop;
	device_info_t *tmp = NULL;
	device_info_t *del_device = NULL;

	pthread_mutex_lock(&hDeviceMutex);
	loop = g_device_info;
	if (loop == NULL) {
		pthread_mutex_unlock(&hDeviceMutex);
		return -1;
	} else {
		while (loop) {
			if (loop == device_info) {
				if (loop == g_device_info) {
					del_device = g_device_info;
					g_device_info = g_device_info->next;
					free(del_device);
				} else {
					del_device = loop;
					loop = loop->next;
					tmp->next = loop;
					free(del_device);
				}
				pthread_mutex_unlock(&hDeviceMutex);
				return 0;
			}
			tmp = loop;
			loop = loop->next;
		}
	}
	pthread_mutex_unlock(&hDeviceMutex);
	return -1;
}
static void *ZTE_OnlineManageThread(void *param) {

	device_info_t *device_info;
	keepalive_t keepalive;
	int preview_flag = 1;

	pthread_detach(pthread_self()); //线程分离.

	while (preview_flag) {
		if (g_register_callback == NULL) {
			usleep(600 * 1000);
			continue;
		}
		while ((device_info = zte_keepaliveliset_getdel()) != NULL) {
			memcpy(&keepalive, &device_info->keepalive, sizeof(keepalive_t));
			zte_keepaliveliset_delete(device_info);
			if (g_register_callback != NULL)
				g_register_callback(FALSE, keepalive.ip, keepalive.port,
						keepalive.user_name, keepalive.user_password,
						keepalive.puid);
		}
		sleep(3);
	}
	return NULL;
}
int zte_start_onlinemanage() {
	pthread_t Thread_id;
	pthread_mutex_init(&hDeviceMutex, NULL);
	pthread_create(&Thread_id, NULL, ZTE_OnlineManageThread, NULL);
	return 0;
}
int zte_get_onlinedev(dev_online_t *dev_online) {
	int num = 0;
	device_info_t *device_info;
	device_info = g_device_info;
	while (device_info) {
		strcpy(dev_online[num].ip, device_info->keepalive.ip);
		dev_online[num].port = device_info->keepalive.port;
		strcpy(dev_online[num].user_name, device_info->keepalive.user_name);
		strcpy(dev_online[num].user_password,
				device_info->keepalive.user_password);
		device_info = device_info->next;
		num++;
	}
	return num;
}
static void *ZTE_remotePlayThread(void *param) {
	pthread_detach(pthread_self()); //线程分离.
#if 0
			remote_play_t *remote_play = (remote_play_t *)param;
			unsigned long user_id = remote_play->user_id;
			msg_play_record_t msg_play_record = remote_play->msg_play_record;
			tp_server_info_t *pServerInfo = (tp_server_info_t *)user_id;
			server_connect_t *pConnect = pServerInfo->server_connect;
			if (!pConnect)
			return NULL;

			int tmp_size;
			int len;
			int tcp_sock;
			int frame_num;
			int iRet;
			net_pack_t net_pack;
			net_msg_t net_msg;
			int status = 0;
			frame_head_t *pFrameHead = NULL;
			char file_name[MSG_RECORD_FILENAME_LEN];
			unsigned char frame_data[300*1024];
			frame_t frame;
			frame.frame_data = frame_data;
			unsigned int frame_no = 0;
			int pos = 0;
			int pre_frame_full;
			int frame_full;

			strcpy(file_name, msg_play_record.file_name);
			if(pConnect == NULL || file_name == NULL)
			return NULL;

			net_msg.msg_head.msg_type = TCP_TYPE_PLAY_RECORD;
			strcpy(net_msg.msg_data,file_name);

			struct sockaddr_in RemoteAddr;
			tcp_sock = udtc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			len = DATA_BUF_LEN;
			if(udtc_setsockopt(tcp_sock, SOL_SOCKET, SO_RCVBUF, (const char *)&len, sizeof(len)) == -1)
			{
				udtc_close(tcp_sock);
				return NULL;
			}

			RemoteAddr.sin_family = AF_INET;
			RemoteAddr.sin_addr.s_addr = inet_addr(pConnect->ip_addr);
			//RemoteAddr.sin_port        = htons(pConnect->port + 1);
			//add by fengzx 2011-07-05
			int nPortOffset = 0;
			if (pConnect->bPortOffset)
			{
				nPortOffset = PORT_OFFSET_TCP;
			}
			RemoteAddr.sin_port = htons(pConnect->port + nPortOffset);
			//end add
			iRet = udtc_connect(tcp_sock, (struct sockaddr *) &RemoteAddr, sizeof(RemoteAddr));
			if(iRet < 0 )
			{
				udtc_close(tcp_sock);
				return NULL;
			}
			tcp_sock_info_t *tcp_sock_info;
			tcp_sock_info = (tcp_sock_info_t*)malloc(sizeof(tcp_sock_info_t));
			tcp_sock_info->sock_fd = tcp_sock;
			tcp_sock_info->next = NULL;

			addTcpSockInfo(pConnect,tcp_sock_info);
			//发送请求
			iRet = udtc_send(tcp_sock,(char *)&net_msg,sizeof(net_msg_t),0);
			if(iRet != sizeof(net_msg_t))
			{
				delTcpSockInfo(pConnect,tcp_sock_info);
				return NULL;
			}
			//接收应答
			iRet = udtc_recv(tcp_sock,(char *)&net_msg,sizeof(net_msg_t),0);
			if(iRet != sizeof(net_msg_t))
			{
				delTcpSockInfo(pConnect,tcp_sock_info);
				return NULL;
			}
			if (net_msg.msg_head.ack_flag != ACK_SUCCESS)
			{
				delTcpSockInfo(pConnect,tcp_sock_info);
				return NULL;
			}

			memcpy(&frame_num,net_msg.msg_data,sizeof(int));

			status = 0;
			while(remote_play->run_flag)
			{
//		if(frame_num <= 0) //暂时去掉
//			break;
				if (status == 1)
				{
					udtc_send(tcp_sock,(char *)&net_msg,sizeof(net_msg_t),0);
//			frame_num = frame_num - 100;
					status = 0;
				}
				struct timeval tval;
				fd_set rset;
				FD_ZERO(&rset);
				FD_SET(tcp_sock,&rset);
				tval.tv_sec = 0;
				tval.tv_usec = 800*1000;
				if(select(tcp_sock+1,&rset,0,0,&tval) <= 0)
				{
					status = remote_play->zte_play_flag;
					continue;
				}
				iRet = udtc_recv(tcp_sock,(char *)&net_pack,sizeof(net_pack_t),0);
				if(iRet != sizeof(net_pack_t))
				{
					udtc_close(tcp_sock);
					delTcpSockInfo(pConnect,tcp_sock_info);
					return NULL;
				}
//		//////////////////////////////////////////////////////////////////////////
				pFrameHead = (frame_head_t*)net_pack.pack_data;

				if (frame_no != pFrameHead->frame_no)
				{
					if (pFrameHead->slice_no != 0)
					continue;
					if (pFrameHead->frame_no - frame_no > 1 && frame_no != 0)
					printf("lose frame %d---%d",frame_no+1,pFrameHead->frame_no-1);
					if (pre_frame_full == 0)
					printf("pre frame is not full,current frame no %d",pFrameHead->frame_no);
					pre_frame_full = 0;
					pos = 0;
					frame_no = pFrameHead->frame_no;
					tmp_size = pFrameHead->frame_size;

					memcpy(frame.frame_data,net_pack.pack_data+sizeof(frame_head_t),pFrameHead->slice_size);
					tmp_size -= pFrameHead->slice_size;
					pos += pFrameHead->slice_size;
				}
				else
				{
					memcpy(frame.frame_data + pos,net_pack.pack_data + sizeof(frame_head_t),pFrameHead->slice_size);
					tmp_size -= pFrameHead->slice_size;
					pos += pFrameHead->slice_size;
				}
				if (tmp_size == 0)
				{
					frame.frame_head = *pFrameHead;
					frame_full = 1;
					pre_frame_full = 1;
				}

				if (frame_full == 1)
				{
					if (pConnect->playrecord_decode_callback != NULL)
					{
						void *context = pConnect->playrecord_decode_callback_context;
						pConnect->playrecord_decode_callback(&frame,frame.frame_head.frame_size+sizeof(frame_head_t), context);
					}
					frame_full = 0;
				}
			}
			udtc_close(tcp_sock);
			delTcpSockInfo(pConnect,tcp_sock_info);
#endif
	return NULL;
}
unsigned long st_net_openRemotePlay(unsigned long user_id,
		msg_play_record_t msg_play_record) {
	remote_play_t *remote_play = NULL;
	remote_play = (remote_play_t *) malloc(sizeof(remote_play_t));
	memset(remote_play, 0, sizeof(remote_play_t));
	if (remote_play == NULL)
		return 0;

	remote_play->user_id = user_id;
	remote_play->msg_play_record = msg_play_record;
	remote_play->run_flag = 1;
	pthread_create(&remote_play->hRomtePlayThread_id, NULL,
			ZTE_remotePlayThread, (void*) &remote_play);
	return (unsigned long) remote_play;
}
int st_net_closeRemotePlay(unsigned long play_id) {
	remote_play_t *remote_play = (remote_play_t *) play_id;
	if (remote_play == NULL)
		return -1;

	remote_play->run_flag = 0;
	usleep(500 * 1000);

	if (NULL != remote_play) {
		free(remote_play);
		remote_play = NULL;
	}
	return 0;
}

int st_net_startPlayStream(unsigned long play_id) {
	remote_play_t *remote_play = (remote_play_t *) play_id;
	if (remote_play != NULL) {
		remote_play->zte_play_flag = 1;
		return 0;
	}
	return -1;
}

int st_net_stopPlayStream(unsigned long play_id) {
	remote_play_t *remote_play = (remote_play_t *) play_id;
	if (remote_play != NULL) {
		remote_play->zte_play_flag = 0;
		return 0;
	}
	return -1;
}

int st_net_get3GSignalVlaue(unsigned long user_id) {
	tp_server_info_t *pServerInfo = (tp_server_info_t *) user_id;
	server_connect_t *pConnect = pServerInfo->server_connect;
	msg_3G_module_info_t signal_3G_info;

	net_msg_t cache_net_msg;
	net_msg_t net_msg;

	if (!pConnect)
		return -1;

	net_msg.msg_head.msg_type = MSG_3G_SIGNAL;
	net_msg.msg_head.msg_subtype = 0;
	net_msg.msg_head.msg_size = 0;
	net_msg.msg_head.ack_flag = ACK_SUCCESS;
	pConnect->interval_valid[INDEX_MSG_3G_SIGNAL] = TRUE;

	if (udtc_send(pConnect->msg_sock, (char *) &net_msg, sizeof(msg_head_t), 0)
			!= sizeof(msg_head_t))
		pServerInfo->iMsgStatus = 0;

	if (st_netcodec_semTrywait(&pConnect->sem_id[INDEX_MSG_3G_SIGNAL], 6 * 1000)
			== 0) {
		if (GetMsg(pConnect, INDEX_MSG_3G_SIGNAL, &cache_net_msg) < 0)
			return -1;
		if (cache_net_msg.msg_head.ack_flag != ACK_SUCCESS)
			signal_3G_info.signal = 0;
		else
			memcpy(&signal_3G_info, &cache_net_msg.msg_data,
					cache_net_msg.msg_head.msg_size);
	} else
		pServerInfo->iMsgStatus = 0;
	return signal_3G_info.signal;
}

int st_net_getChannelNum(unsigned long user_id) {
	LOGI("st_net_getChannelNum");
	int m_hDevType, m_ChannNo = 0;
	m_hDevType = st_net_getDeviceType(user_id);
	if (m_hDevType <= 0) {
		return -1;
	}

	switch (m_hDevType) {
	case DEV_TYPE_NT200HA:
	case DEV_TYPE_NT200HB:
	case DEV_TYPE_NT200HC:
	case DEV_TYPE_NT200HD:

	case DEV_TYPE_NT6050:
	case DEV_TYPE_NT8189:
	case DEV_TYPE_NT4094:
	case DEV_TYPE_NT6050H:
	case DEV_TYPE_NT8189H:
	case DEV_TYPE_NT4094H:

	case DEV_TYPE_NT9072H:
	case DEV_TYPE_NT1102H:
	case DEV_TYPE_NT1102H_D:

		//case DEV_TYPE_NT6050H_1:
		//case DEV_TYPE_NT6050H_2:
	{
		m_hDevType = DEV_TYPE_NT200HD;
		m_ChannNo = 1;

	}
		break;
	case DEV_TYPE_NT202HD_1:
	case DEV_TYPE_NT202HD:
		//case DEV_TYPE_NT202HA:
	{
		m_hDevType = DEV_TYPE_NT202HD;
		m_ChannNo = 2;

	}
		break;
	case DEV_TYPE_NT204HA:
	case DEV_TYPE_NT204HD_1:
	case DEV_TYPE_NT204HD: {
		m_hDevType = DEV_TYPE_NT204HD;
		m_ChannNo = 4;

	}
		break;
	case DEV_TYPE_HI3515: {
		m_hDevType = DEV_TYPE_HI3515;
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		m_ChannNo = sys_config.channel_num; //可变通道

	}
		break;
	case DEV_TYPE_NT3511H: {
		m_hDevType = DEV_TYPE_NT3511H;
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		m_ChannNo = sys_config.channel_num; //可变通道

	}
		break;
	default:
		break;
	}
	return m_ChannNo;

}

int st_net_getIsSurportSlave(unsigned long user_id) {
	int m_hDevType, ds_enable_flag = 0;
	m_hDevType = st_net_getDeviceType(user_id);
	if (m_hDevType <= 0) {
		return -1;
	}

	switch (m_hDevType) {
	case DEV_TYPE_HI3515: {
		m_hDevType = DEV_TYPE_HI3515;
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		ds_enable_flag = sys_config.ds_enable_flag; //可变通道

	}
		break;
	case DEV_TYPE_NT3511H: {
		m_hDevType = DEV_TYPE_NT3511H;
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		ds_enable_flag = sys_config.ds_enable_flag; //可变通道
	}
		break;
	default:
		break;
	}
	return ds_enable_flag;

}

int st_net_getVideoParam(unsigned long user_id, int channel, void* codeinfo) {
	int m_hDevType;

	m_hDevType = st_net_getDeviceType(user_id);
	if (m_hDevType <= 0) {
		return -1;
	}
	video_encode_t video_encode[16];
	video_config_t video_config[16];
	void *pVideoPara;
	int type;
	int data_size;

	memset(&video_config, 0, sizeof(video_config_t) * 16);
	pVideoPara = &video_config;
	type = PARAM_VIDEO_CONFIG;
	data_size = sizeof(video_config_t);

	switch (m_hDevType) {
	case DEV_TYPE_NT200HA:
	case DEV_TYPE_NT200HB:
	case DEV_TYPE_NT200HC:
	case DEV_TYPE_NT200HD:
	case DEV_TYPE_NT6050:
	case DEV_TYPE_NT8189:
	case DEV_TYPE_NT4094:
	case DEV_TYPE_NT6050H:
	case DEV_TYPE_NT8189H:
	case DEV_TYPE_NT4094H:
	case DEV_TYPE_NT9072H:
	case DEV_TYPE_NT1102H:
	case DEV_TYPE_NT1102H_D: {
		if (st_net_getParam(user_id, type, pVideoPara, data_size) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT202HD_1:
	case DEV_TYPE_NT202HD:
		//case DEV_TYPE_NT202HA:
	{
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 2) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT204HA:
	case DEV_TYPE_NT204HD_1:
	case DEV_TYPE_NT204HD: {
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 4) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_HI3515: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		int m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT3511H: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		int m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	default:
		return -1;
		break;
	}

	memcpy(codeinfo, &video_config[channel], data_size);
	return 1;
}

int st_net_getVideoEncodeParam(unsigned long user_id, int channel,
		void* codeinfo, int channel_type) {
	int m_hDevType;
	m_hDevType = st_net_getDeviceType(user_id);
	if (m_hDevType <= 0) {
		return -1;
	}
	video_encode_t video_encode[16];
//	video_config_t video_config[16];
	void *pVideoPara;
	int type;
	int data_size;

	memset(&video_encode, 0, sizeof(video_encode_t) * 16);
	pVideoPara = &video_encode;
	data_size = sizeof(video_encode_t);

	if (channel_type == 0) {
		type = PARAM_VIDEO_ENCODE;
	} else if (channel_type == 1) {
		type = PARAM_SLAVE_ENCODE;
	}

	switch (m_hDevType) {
	case DEV_TYPE_NT200HA:
	case DEV_TYPE_NT200HB:
	case DEV_TYPE_NT200HC:
	case DEV_TYPE_NT200HD:
	case DEV_TYPE_NT6050:
	case DEV_TYPE_NT8189:
	case DEV_TYPE_NT4094:
	case DEV_TYPE_NT6050H:
	case DEV_TYPE_NT8189H:
	case DEV_TYPE_NT4094H:
	case DEV_TYPE_NT9072H:
	case DEV_TYPE_NT1102H:
	case DEV_TYPE_NT1102H_D: {
		if (st_net_getParam(user_id, type, pVideoPara, data_size) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT202HD_1:
	case DEV_TYPE_NT202HD:
		//case DEV_TYPE_NT202HA:
	{
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 2) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT204HA:
	case DEV_TYPE_NT204HD_1:
	case DEV_TYPE_NT204HD: {
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 4) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_HI3515: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		int m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT3511H: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		int m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	default:
		return -1;
		break;
	}

	memcpy(codeinfo, &video_encode[channel], data_size);

	return 1;
}

//设置编码、视频参数
int set_videoPara(unsigned long user_id, int channel, void* codeinfo,
		int isVideoConfig) {
	int m_hDevType;
	int m_ChannNo;
	m_hDevType = st_net_getDeviceType(user_id);
	if (m_hDevType <= 0) {
		return -1;
	}
	video_encode_t video_encode[16];
	video_config_t video_config[16];
	void *pVideoPara;
	int type;
	int data_size;
	if (isVideoConfig == 1) {
		memset(&video_config, 0, sizeof(video_config_t) * 16);
		pVideoPara = &video_config;
		type = PARAM_VIDEO_CONFIG;
		data_size = sizeof(video_config_t);
	} else {
		memset(&video_encode, 0, sizeof(video_encode_t) * 16);
		pVideoPara = &video_encode;
		type = PARAM_VIDEO_ENCODE;
		data_size = sizeof(video_encode_t);
	}

	switch (m_hDevType) {
	case DEV_TYPE_NT200HA:
	case DEV_TYPE_NT200HB:
	case DEV_TYPE_NT200HC:
	case DEV_TYPE_NT200HD:
	case DEV_TYPE_NT6050:
	case DEV_TYPE_NT8189:
	case DEV_TYPE_NT4094:
	case DEV_TYPE_NT6050H:
	case DEV_TYPE_NT8189H:
	case DEV_TYPE_NT4094H:
	case DEV_TYPE_NT9072H:
	case DEV_TYPE_NT1102H:
	case DEV_TYPE_NT1102H_D: {
		m_ChannNo = 1;
		if (st_net_getParam(user_id, type, pVideoPara, data_size) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT202HD_1:
	case DEV_TYPE_NT202HD:
		//case DEV_TYPE_NT202HA:
	{
		m_ChannNo = 2;
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 2) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT204HA:
	case DEV_TYPE_NT204HD_1:
	case DEV_TYPE_NT204HD: {
		m_ChannNo = 4;
		if (st_net_getParam(user_id, type, pVideoPara, data_size * 4) < 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_HI3515: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	case DEV_TYPE_NT3511H: {
		sys_config_t sys_config;
		memset(&sys_config, 0, sizeof(sys_config_t));
		if (st_net_getParam(user_id, PARAM_3511_SYSINFO, &sys_config,
				sizeof(sys_config_t)) < 0) {
			return 0;
		}
		m_ChannNo = sys_config.channel_num; //可变通道
		if (st_net_getParam(user_id, type, pVideoPara, data_size * m_ChannNo)
				< 0) {
			return -1;
		}
	}
		break;
	default:
		return -1;
		break;
	}
	if (isVideoConfig == 1) {
		video_config_t *pinfo = (video_config_t*) codeinfo;
		video_config[channel].brightness = pinfo->brightness;
		video_config[channel].contrast = pinfo->contrast;
		video_config[channel].hue = pinfo->hue;
		video_config[channel].saturation = pinfo->saturation;
		st_net_setParam(user_id, type, &video_config, data_size * m_ChannNo);
	} else {
		video_encode_t *pinfo = (video_encode_t*) codeinfo;

		video_encode[channel].video_format = pinfo->video_format;
		video_encode[channel].resolution = pinfo->resolution;
		video_encode[channel].bitrate_type = pinfo->bitrate_type;
		video_encode[channel].level = pinfo->level;
		video_encode[channel].frame_rate = pinfo->frame_rate;
		video_encode[channel].Iframe_interval = pinfo->Iframe_interval;
		video_encode[channel].prefer_frame = pinfo->prefer_frame;
		video_encode[channel].Qp = pinfo->Qp;
		video_encode[channel].encode_type = pinfo->encode_type;

		st_net_setParam(user_id, type, &video_encode, data_size * m_ChannNo);
	}
	return 1;
}

// 2014-5-21  LinZh107
int tp_net_get_DevData(int channel, ck_dev_data_t *pDevInfo,
		msg_time_t *alarmTime) {
	pthread_mutex_lock(&alarmInfoMutex);

	//2014-4-18 LinZh107 加上alarminfo组号
	//LOGI("g_AlarmInfoGroup[channel].ip = %.16s", g_AlarmInfoGroup[channel].ip);
	if (25 != g_AlarmInfoGroup[channel].alarm_type) {
		//LOGE("g_AlarmInfoGroup[channel].alarm_type!=25");
		pthread_mutex_unlock(&alarmInfoMutex);
		return g_AlarmInfoGroup[channel].alarm_type;
	}

	memcpy(pDevInfo, g_AlarmInfoGroup[channel].alarm_info,
			sizeof(ck_dev_data_t));
	memcpy(alarmTime, &g_AlarmInfoGroup[channel].tm, sizeof(msg_time_t));
	pthread_mutex_unlock(&alarmInfoMutex);
	return 25; // 指示是我们所需的此类报警信息
}

//add by LinZh107 for net_io_control 150601
int tp_net_get_io_state(int user_id, void* param) {
	return st_net_getParam(user_id, PARAM_HANDIO_CONTROL, param,
			sizeof(dev_io_control_t));
}

int tp_net_set_io_state(int user_id, void* param) {
	return st_net_setParam(user_id, PARAM_HANDIO_CONTROL, param,
			sizeof(dev_io_control_t));
}
int startRecv() {
	isfull = 0;
	LOGI("调用了startRecv");
	return 0;
}
int stopRecv() {
	isfull = 1;
	LOGI("调用了stopRecv");
	return 1;
}
