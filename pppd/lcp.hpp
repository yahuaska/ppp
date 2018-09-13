//
// Created by ringo on 13.09.18.
//

#ifndef PPP_LCP_HPP
#define PPP_LCP_HPP

#include "pppd.h"
#include "fsm.hpp"
namespace rng0 {

	struct epdisc {
		unsigned char	cls;
		unsigned char	length;
		unsigned char	value[MAX_ENDP_LEN];
	};


#define DEFMRU	1500		/* Try for this */
#define MINMRU	128		/* No MRUs below this */
#define MAXMRU	16384		/* Normally limit MRU to this */

	/*
 * Options.
 */
#define CI_VENDOR	0	/* Vendor Specific */
#define CI_MRU		1	/* Maximum Receive Unit */
#define CI_ASYNCMAP	2	/* Async Control Character Map */
#define CI_AUTHTYPE	3	/* Authentication Type */
#define CI_QUALITY	4	/* Quality Protocol */
#define CI_MAGICNUMBER	5	/* Magic Number */
#define CI_PCOMPRESSION	7	/* Protocol Field Compression */
#define CI_ACCOMPRESSION 8	/* Address/Control Field Compression */
#define CI_FCSALTERN	9	/* FCS-Alternatives */
#define CI_SDP		10	/* Self-Describing-Pad */
#define CI_NUMBERED	11	/* Numbered-Mode */
#define CI_CALLBACK	13	/* callback */
#define CI_MRRU		17	/* max reconstructed receive unit; multilink */
#define CI_SSNHF	18	/* short sequence numbers for multilink */
#define CI_EPDISC	19	/* endpoint discriminator */
#define CI_MPPLUS	22	/* Multi-Link-Plus-Procedure */
#define CI_LDISC	23	/* Link-Discriminator */
#define CI_LCPAUTH	24	/* LCP Authentication */
#define CI_COBS		25	/* Consistent Overhead Byte Stuffing */
#define CI_PREFELIS	26	/* Prefix Elision */
#define CI_MPHDRFMT	27	/* MP Header Format */
#define CI_I18N		28	/* Internationalization */
#define CI_SDL		29	/* Simple Data Link */

/* Default number of times we receive our magic number from the peer
   before deciding the link is looped-back. */
#define DEFLOOPBACKFAIL	10

/* Value used as data for CI_CALLBACK option */
#define CBCP_OPT	6	/* Use callback control protocol */

/*
 * LCP-specific packet types (code numbers).
 */
#define PROTREJ		8	/* Protocol Reject */
#define ECHOREQ		9	/* Echo Request */
#define ECHOREP		10	/* Echo Reply */
#define DISCREQ		11	/* Discard Request */
#define IDENTIF		12	/* Identification */
#define TIMEREM		13	/* Time Remaining */

	/*
	 * The state of options is described by an lcp_options structure.
	 */
	typedef struct lcp_options {
		bool passive;		/* Don't die if we don't get a response */
		bool silent;		/* Wait for the other end to start first */
		bool restart;		/* Restart vs. exit after close */
		bool neg_mru;		/* Negotiate the MRU? */
		bool neg_asyncmap;		/* Negotiate the async map? */
		bool neg_upap;		/* Ask for UPAP authentication? */
		bool neg_chap;		/* Ask for CHAP authentication? */
		bool neg_eap;		/* Ask for EAP authentication? */
		bool neg_magicnumber;	/* Ask for magic number? */
		bool neg_pcompression;	/* HDLC Protocol Field Compression? */
		bool neg_accompression;	/* HDLC Address/Control Field Compression? */
		bool neg_lqr;		/* Negotiate use of Link Quality Reports */
		bool neg_cbcp;		/* Negotiate use of CBCP */
		bool neg_mrru;		/* negotiate multilink MRRU */
		bool neg_ssnhf;		/* negotiate short sequence numbers */
		bool neg_endpoint;		/* negotiate endpoint discriminator */
		int  mru;			/* Value of MRU */
		int	 mrru;			/* Value of MRRU, and multilink enable */
		u_char chap_mdtype;		/* which MD types (hashing algorithm) */
		u_int32_t asyncmap;		/* Value of async map */
		u_int32_t magicnumber;
		int  numloops;		/* Number of loops during magic number neg. */
		u_int32_t lqr_period;	/* Reporting period for LQR 1/100ths second */
		struct epdisc endpoint;	/* endpoint discriminator */
	} lcp_options;

	class LCP : public FSM {
	public:
		explicit LCP(int);
		void open() override;
		void close(char *) override;
		void lowerup() override;
		void lowerdown() override;
		void input(u_char *, int) override;

		void up();
		static void delayed_up(void *);
		void down();
		void starting();
		void finishing();
		void finished();

		void rprotrej(u_char *inp, int len);
		void protrej();
		void sprotrej(u_char *p, int len);

		int ackci(u_char *p, int len);
		void addci(u_char *ucp, int *lenp);
		int cilen();
		int nakci(u_char *p, int len, int treat_as_reject);
		int rejci(u_char *p, int len);
		void resetci();
		int reqci(u_char *inp, size_t *lenp, int reject_if_disagree);

//		void protreject(u_char *, int);
		void received_echo_reply(int, u_char*, int);
		int extcode(int code, int id, u_char *inp, int len);

		void LcpLinkFailure();
		void LcpEchoCheck();
		void LcpSendEchoRequest();
		void echo_lowerup();
		void echo_lowerdown();
		static void LcpEchoTimeout(LCP *);

		lcp_options *get_wantoptions() { return &this->lcp_wantoptions; }
		lcp_options *get_allowoptions() { return &this->lcp_allowoptions; }
		lcp_options *get_gotoptions() { return &this->lcp_gotoptions; }

	protected:
		lcp_options lcp_wantoptions;	/* Options that we want to request */
		lcp_options lcp_gotoptions;	/* Options that peer ack'd */
		lcp_options lcp_allowoptions;	/* Options we allow peer to request */
		lcp_options lcp_hisoptions;	/* Options that we ack'd */
	};
}
#endif //PPP_LCP_HPP
