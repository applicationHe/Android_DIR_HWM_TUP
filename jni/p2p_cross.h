/*
 * create by LinZh107
 * need 3  pjproject's librarys
 *	libpj-arm-unknown-linux-androideabi.a
 *	libpjlib-util-arm-unknown-linux-androideabi.a
 *	libpjnath-arm-unknown-linux-androideabi.a
 * 2015-2-12 11:43:37
 * Copyright sztopvs@2015
 */

#ifndef __ICEDEMO__
#define __ICEDEMO__


/* ****************************************************** */
/* Add By LinZh107 at 150307 , helping to get the sock_fd. */
#include <pj/activesock.h>
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath/stun_sock.h>
#include <pjnath.h>
#include <ioqueue_common_abs.h>

/* ***** copy from ioqueue_common_abs.h ***** */
struct pj_ioqueue_key_t
{
	/*
	*	replace PJ_DECL_LIST_MEMBER
	*/
    PJ_DECL_LIST_MEMBER(struct pj_ioqueue_key_t);
    pj_ioqueue_t         		*ioqueue;               \
    pj_grp_lock_t				*grp_lock;				\
    pj_lock_t            		*lock;                  \
    pj_bool_t					inside_callback;		\
    pj_bool_t					destroy_requested;		\
    pj_bool_t					allow_concurrent;		\
    pj_sock_t					fd;						\
    int							fd_type;                \
    void						*user_data;				\
    pj_ioqueue_callback	    	cb;                     \
    int							connecting;             \
    struct read_operation		read_list;              \
    struct write_operation		write_list;             \
    struct accept_operation		accept_list;
	/*
	* remove redeclaration member
	*/
    pj_ioqueue_operation_e  op;
};
/* ****************** */

/* ***** copy from ActiveSock.c ***** */
enum read_type
{
	TYPE_NONE,
	TYPE_RECV,
	TYPE_RECV_FROM
};
struct send_data
{
	pj_uint8_t		*data;
	pj_ssize_t		 len;
	pj_ssize_t		 sent;
	unsigned		 flags;
};
struct pj_activesock_t
{
	pj_ioqueue_key_t	*key;
	pj_bool_t		 stream_oriented;
	pj_bool_t		 whole_data;
	pj_ioqueue_t	*ioqueue;
	void		*user_data;
	unsigned		 async_count;
	unsigned	 	 shutdown;
	unsigned		 max_loop;
	pj_activesock_cb	 cb;
#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
	PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT!=0
	int			 bg_setting;
	pj_sock_t		 sock;
	CFReadStreamRef	 readStream;
#endif

	unsigned		 err_counter;
	pj_status_t		 last_err;
	struct send_data	 send_data;
	struct read_op	*read_op;
	pj_uint32_t		 read_flags;
	enum read_type	 read_type;
	struct accept_op	*accept_op;
};
/* **************** */

/* ***** copy from stun_sock.c ***** */
struct pj_stun_sock
{
    char		*obj_name;	/* Log identification	    */
    pj_pool_t		*pool;		/* Pool			    */
    void		*user_data;	/* Application user data    */
    pj_bool_t		 is_destroying; /* Destroy already called   */
    int			 af;		/* Address family	    */
    pj_stun_config	 stun_cfg;	/* STUN config (ioqueue etc)*/
    pj_stun_sock_cb	 cb;		/* Application callbacks    */

    int			 ka_interval;	/* Keep alive interval	    */
    pj_timer_entry	 ka_timer;	/* Keep alive timer.	    */

    pj_sockaddr		 srv_addr;	/* Resolved server addr	    */
    pj_sockaddr		 mapped_addr;	/* Our public address	    */

    pj_dns_srv_async_query *q;		/* Pending DNS query	    */
    pj_sock_t		 sock_fd;	/* Socket descriptor	    */
    pj_activesock_t	*active_sock;	/* Active socket object	    */
    pj_ioqueue_op_key_t	 send_key;	/* Default send key for app */
    pj_ioqueue_op_key_t	 int_send_key;	/* Send key for internal    */

    pj_uint16_t		 tsx_id[6];	/* .. to match STUN msg	    */
    pj_stun_session	*stun_sess;	/* STUN session		    */
    pj_grp_lock_t	*grp_lock;	/* Session group lock	    */
};
/* **************** */

/* *********** copy from ice_strans.c ********* */
typedef struct pj_ice_strans_comp
{
    pj_ice_strans	*ice_st;	/**< ICE stream transport.	*/
    unsigned		 comp_id;	/**< Component ID.		*/

    pj_stun_sock	*stun_sock;	/**< STUN transport.		*/
    pj_turn_sock	*turn_sock;	/**< TURN relay transport.	*/
    pj_bool_t		 turn_log_off;	/**< TURN loggin off?		*/
    unsigned		 turn_err_cnt;	/**< TURN disconnected count.	*/

    unsigned		 cand_cnt;	/**< # of candidates/aliaes.	*/
    pj_ice_sess_cand	 cand_list[PJ_ICE_ST_MAX_CAND];	/**< Cand array	*/

    unsigned		 default_cand;	/**< Default candidate.		*/

} pj_ice_strans_comp;

struct pj_ice_strans
{
    char		    	*obj_name;	/**< Log ID.			*/
    pj_pool_t		    *pool;		/**< Pool used by this object.	*/
    void		    	*user_data;	/**< Application data.		*/
    pj_ice_strans_cfg	cfg;		/**< Configuration.		*/
    pj_ice_strans_cb	cb;			/**< Application callback.	*/
    pj_grp_lock_t	    *grp_lock;  /**< Group lock.		*/

    pj_ice_strans_state	state;		/**< Session state.		*/
    pj_ice_sess		    *ice;		/**< ICE session.		*/
    pj_time_val		    start_time;	/**< Time when ICE was started	*/

    unsigned		    comp_cnt;	/**< Number of components.	*/
    pj_ice_strans_comp	**comp;		/**< Components array.		*/

    pj_timer_entry	    ka_timer;	/**< STUN keep-alive timer.	*/

    pj_bool_t		    destroy_req;/**< Destroy has been called?	*/
    pj_bool_t		    cb_called;	/**< Init error callback called?*/
};
/* ************************************** */
/* ****************************************************** */
/* End add By LinZh107 at 150307 , helping get the sock_fd. */


typedef struct app_t
{
	/* Command line options are stored here */
	struct options
	{
		unsigned comp_cnt;
		pj_str_t ns;
		int max_host;
		pj_bool_t regular;
		pj_str_t stun_srv;
		pj_str_t turn_srv;
		pj_bool_t turn_tcp;
		pj_str_t turn_username;
		pj_str_t turn_password;
		pj_bool_t turn_fingerprint;
		const char *log_file;
	} opt;

	/* Our global variables */
	pj_caching_pool 	cp;
	pj_pool_t 			*pool;
	pj_thread_t 		*thread;
	pj_bool_t			thread_quit_flag;
	pj_ice_strans_cfg	ice_cfg;
	pj_ice_strans		*icest;
	FILE 				*log_fhnd;

	/* Variables to store parsed remote ICE info */
	struct rem_info
	{
		char ufrag[80];
		char pwd[80];
		unsigned comp_cnt;
		pj_sockaddr def_addr[PJ_ICE_MAX_COMP];
		unsigned cand_cnt;
		pj_ice_sess_cand cand[PJ_ICE_ST_MAX_CAND];
	} rem;
} g_app_t;


#ifndef __P2P_THROUGH_PARAM__
#define __P2P_THROUGH_PARAM__
typedef struct __p2p_thorough_param
{
	char puid[32];
	int udt_sock;
	int exit_flag;
}p2p_thorough_param_t;
#endif

/**************** export api *******************/
int tp_pjice_main(void *p2p_param);
void tp_pjice_free(void);


#endif //#ifndef __ICEDEMO__

