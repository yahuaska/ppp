//
// Created by ringo on 12.09.18.
//

#ifndef PPP_FSM_HPP
#define PPP_FSM_HPP

#include <sys/types.h>
/**
 * @brief Finite State Machine. Derived from ppp's FSM
 */
namespace rng0 {
	/**/
	class FSM;
	typedef struct fsm_callbacks {
		void (*resetci)		/* Reset our Configuration Information */
				__P((FSM *));
		int  (*cilen)		/* Length of our Configuration Information */
				__P((FSM *));
		void (*addci) 		/* Add our Configuration Information */
				__P((FSM *, u_char *, int *));
		int  (*ackci)		/* ACK our Configuration Information */
				__P((FSM *, u_char *, int));
		int  (*nakci)		/* NAK our Configuration Information */
				__P((FSM *, u_char *, int, int));
		int  (*rejci)		/* Reject our Configuration Information */
				__P((FSM *, u_char *, int));
		int  (*reqci)		/* Request peer's Configuration Information */
				__P((FSM *, u_char *, int *, int));
		void (*up)			/* Called when fsm reaches OPENED state */
				__P((FSM *));
		void (*down)		/* Called when fsm leaves OPENED state */
				__P((FSM *));
		void (*starting)		/* Called when we want the lower layer */
				__P((FSM *));
		void (*finished)		/* Called when we don't want the lower layer */
				__P((FSM *));
		void (*protreject)		/* Called when Protocol-Reject received */
				__P((int));
		void (*retransmit)		/* Retransmission is necessary */
				__P((FSM *));
		int  (*extcode)		/* Called when unknown code received */
				__P((FSM *, int, int, u_char *, int));
		char *proto_name;		/* String name for protocol (for messages) */
	} fsm_callbacks;

	constexpr size_t HEADERLEN = 4;
	/*
    *  CP (LCP, IPCP, etc.) codes.
    */
	constexpr int CONFREQ		= 1;	/* Configuration Request */
	constexpr int CONFACK		= 2;	/* Configuration Ack */
	constexpr int CONFNAK		= 3;	/* Configuration Nak */
	constexpr int CONFREJ		= 4;	/* Configuration Reject */
	constexpr int TERMREQ		= 5;	/* Termination Request */
	constexpr int TERMACK		= 6;	/* Termination Ack */
	constexpr int CODEREJ		= 7;	/* Code Reject */

	/*
	 * Timeouts.
	 */
	constexpr int DEFTIMEOUT	    = 3;	/* Timeout time in seconds */
	constexpr int DEFMAXTERMREQS	= 2;    /* Maximum Terminate-Request transmissions */
	constexpr int DEFMAXCONFREQS	= 10;   /* Maximum Configure-Request transmissions */
	constexpr int DEFMAXNAKLOOPS	= 5;    /* Maximum number of nak loops */

	/*
	 * Flags - indicate options controlling FSM operation
	 */
	constexpr int OPT_PASSIVE	= 1;	/* Don't die if we don't get a response */
	constexpr int OPT_RESTART	= 2;	/* Treat 2nd OPEN as DOWN, UP */
	constexpr int OPT_SILENT	= 4;	/* Wait for peer to speak first */

	/*
	 * Link states.
	 */
	enum State {
		INITIAL		= 0,	/* Down, hasn't been opened */
		STARTING	= 1,	/* Down, been opened */
		CLOSED		= 2,	/* Up, hasn't been opened */
		STOPPED		= 3,	/* Open, waiting for down event */
		CLOSING		= 4,	/* Terminating the connection, not open */
		STOPPING	= 5,	/* Terminating, but open */
		REQSENT		= 6,	/* We've sent a Config Request */
		ACKRCVD		= 7,	/* We've received a Config Ack */
		ACKSENT		= 8,	/* We've sent a Config Ack */
		OPENED		= 9	/* Connection available */
	};

	class FSM {
	public:
		FSM();
		virtual void lowerup();
		virtual void lowerdown();
		virtual void open();
		void terminate_layer(State);
		virtual void close(char *reason);
		// static void fsm_timeout __P((void *));
		static void timeout(void *);
		virtual void input(u_char *, int);

		// fsm_rconfreq __P((fsm *, int, u_char *, int));
		void rconfreq(u_char id, u_char *inp, int len);
		// static void fsm_rconfack __P((fsm *, int, u_char *, int));
		void rconfack(u_char id, u_char *inp, int len);
		// static void fsm_rconfnakrej __P((fsm *, int, int, u_char *, int));
		void rconfnakrej(int code,  int id, u_char *inp, int len);
		// static void fsm_rtermreq __P((fsm *, int, u_char *, int));
		void rtermreq(u_char id, u_char *inp, int len);
		// static void fsm_rtermack __P((fsm *));
		void rtermack();
		// static void fsm_rcoderej __P((fsm *, u_char *, int));
		void rcoderej(u_char *inp, int len);
		// static void fsm_sconfreq __P((fsm *, int));
		void sconfreq(int);
		// fsm_sdata(code, id, data, datalen)
		void sdata(u_char code, u_char id, u_char *data, size_t datalen);

		inline char *proto_name() { return this->callbacks->proto_name; }

		static inline char *proto_name(FSM *fsm) { return fsm->callbacks->proto_name; }
//		virtual void init();
		int flags;            /* Contains option bits */

	protected:
		void protreject();
		int unit;            /* Interface unit number */
		int protocol;        /* Data Link Layer Protocol field value */
		int state;            /* State */
		u_char id;            /* Current id */
		u_char reqid;        /* Current request id */
		u_char seen_ack;        /* Have received valid Ack/Nak/Rej to Req */
	public:
		int timeouttime;        /* Timeout time in milliseconds */
		int maxconfreqtransmits;    /* Maximum Configure-Request transmissions */
		int retransmits;        /* Number of retransmissions left */
		int maxtermtransmits;    /* Maximum Terminate-Request transmissions */
		int nakloops;        /* Number of nak loops since last ack */
		int rnakloops;        /* Number of naks received */
		int maxnakloops;        /* Maximum number of nak loops tolerated */
		struct fsm_callbacks *callbacks;    /* Callback routines */
		char *term_reason;        /* Reason for closing protocol */
		size_t term_reason_len;    /* Length of term_reason */
	};
}
#endif //PPP_FSM_HPP
