#ifndef _PROTO_H
#define _PROTO_H

#include <sys/types.h>

#define MSG_PU_REG				0x0001
#define MSG_PU_KEEPLIVE			0x0002
#define MSG_PU_REG_ACK			0x0003
#define MSG_PU_KEEPLIVE_ACK		0x0004

// add by LinZh107
#define MSG_ASSIST_HOLE			0x0005
#define MSG_ASSIST_HOLE_ACK		0x0006
#define MSG_ASSIST_HOLE_CANCEL	0x0007

#define MSG_HOLE_CU2PU			0x0008
#define MSG_HOLE_PU2CU			0x0009
#define MSG_HOLE_CU2PU_ACK		0x000a
#define MSG_HOLE_PU2CU_ACK		0x000b

#define MSG_HOLE_TEST      		0x000c
#define MSG_HOLE_TEST_ACK 		0x000d

#define LOGIN 1
#define LOGOUT 2
#define SONNECTACK 3
#define GETALLUSER  4
#define USERACTIVEQUERY 6
#define USERACTIVEQUERYACK 7
#define USERLOGACK  8
#define REQUESTHOLE  9
#define P2PHOLESUCCESS 10
#define GETSPECIEDUSER 11
#define P2PHOLEACK   12


#define MAX_USERNAME 15
#define MAX_TRY_NUMBER 5
#define MAX_ADDR_NUMBER 5

#define SERVER_PORT 6900
#define COMMANDMAXC 256
#define REMOTE_SDP_LEN 512

typedef struct _cu_login_info_t
{
    char cu_userName[10];
    char cu_password[10];
    
    char pu_userName[10];
    char pu_password[10];

    char cu_id[32];
    char pu_id[32];

	unsigned int cu_local_ip;
    unsigned short cu_local_port;
    unsigned int cu_public_ip;
    unsigned short cu_public_port;

    unsigned int pu_local_ip;
    unsigned short pu_local_port;
    unsigned int pu_public_ip;
    unsigned short pu_public_port;


    int cu_sessionCount;
    unsigned long  cu_dwLastActiveTime;
    int cu_bClientLoginFlag;

    int pu_sessionCount;
    unsigned long pu_dwLastActiveTime;
    int pu_bClientLoginFlag;
    int bPunchFlag;
    
    int bcu_to_pu_status;
    int bpu_to_cu_status;

}cu_login_info_t;

typedef struct stLoginMessage
{
	char userName[10];
	char password[10];
 
    char id[32];
    int sessionCount;
    unsigned long dwLastActiveTime;
	int bClientLoginFlag;
}stLoginMessage;

typedef struct ADDR_INFO
{
	unsigned int  dwIp;
    unsigned short  nPort;
}ADDR_INFO;


typedef struct stLogoutMessage
{
	char id[32];
}stLogoutMessage;

typedef struct stP2PTranslate
{
	char id[32];
}stP2PTranslate;

typedef struct stMessage
{
	int iMessageType;
	union _message
	{
		stLogoutMessage logoutmember;
		cu_login_info_t translatemessage;
	}message;
}stMessage;

typedef struct stUserListNode
{
   char cu_userName[10];
   char cu_password[10];
   char pu_userName[10];
   char pu_password[10];


   char cu_id[32];
   char pu_id[32];
   unsigned int cu_local_ip;
   unsigned short cu_local_port;
   unsigned int cu_public_ip;
   unsigned short cu_public_port;
   
   unsigned int pu_local_ip;
   unsigned short pu_local_port;
   unsigned int pu_public_ip;
   unsigned short pu_public_port;

   int cu_sessionCount;
   unsigned long cu_dwLastActiveTime;
   int cu_bClientLoginFlag;
 
   int pu_sessionCount;
   unsigned long  pu_dwLastActiveTime;
   int pu_bClientLoginFlag;
   int bPunchFlag;
   int bcu_to_pu_status;
   int bpu_to_cu_status;

}stUserListNode;

#if 0
struct stServerToClient
{
	int iMessageType;
	union _message_t
	{
		stUserListNode user;
	}message_t;

};
#endif

#define P2PMESSAGE 100               // 发送消息
#define P2PMESSAGEACK 101            // 收到消息的应答
#define P2PSOMEONEWANTTOCALLYOU 102  // 服务器向客户端发送的消息
                                     // 希望此客户端发送一个UDP打洞包
#define P2PTRASH        103          // 客户端发送的打洞包，接收端应该忽略此消息

typedef struct stP2PMessage
{
	int iStringLen;         // or IP address
	unsigned short Port;
    int iMessageType;
    cu_login_info_t stUser; 
}stP2PMessage_t;

typedef struct _server_info_t
{
    unsigned int ServerIp;
    char sz_userName[MAX_USERNAME];
    char szPuId[32];
    char reserve[32];
}server_info_t;

////////////////////////////////////////////////////////////////////////////////////////////
typedef struct __p2p_msg_head_t
{
	int type;
	int ack;
	int size;
	char data[REMOTE_SDP_LEN]; //#define REMOTE_SDP_LEN 512
}p2p_msg_head_t;

typedef struct __p2p_msg_t
{
	p2p_msg_head_t head;
	char msg_data[256];
}p2p_msg_t;

#if 0
typedef struct __p2p_pu_info_t
{
	char pu_id[32];
	int last_tick;

	char public_ip[16];
	char local_ip[16];
	
	int public_port;
	int local_port;
}p2p_pu_info_t;
#endif

typedef struct __p2p_cu_info_t
{
	char pu_id[32];

	char public_ip[16];
	char local_ip[16];
	
	int public_port;
	int local_port;

}p2p_cu_info_t;

typedef struct __pu_reg_info_t
{
	char pu_id[32];
	char public_ip[16];
	int public_port;

	char user_name[32];
	char user_pwd[32];
	int last_tick;
}pu_reg_info_t;

typedef struct __assist_hole_t
{
	char pu_id[32];
	char pu_public_ip[16];
	int pu_public_port;

	int cu_public_port;	
	char cu_public_ip[16];
}assist_hole_t;

#endif
