/*
 * fsm.c - {Link, IP} Control Protocol Finite State Machine.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define RCSID	"$Id: fsm.c,v 1.23 2004/11/13 02:28:15 paulus Exp $"

/*
 * TODO:
 * Randomize fsm id on link/init.
 * Deal with variable outgoing MTU.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "pppd.h"
#include "fsm.hpp"

static const char rcsid[] = RCSID;

/*
static void fsm_timeout __P((void *));
static void fsm_rconfreq __P((fsm *, int, u_char *, int));
static void fsm_rconfack __P((fsm *, int, u_char *, int));
static void fsm_rconfnakrej __P((fsm *, int, int, u_char *, int));
static void fsm_rtermreq __P((fsm *, int, u_char *, int));
static void fsm_rtermack __P((fsm *));
static void fsm_rcoderej __P((fsm *, u_char *, int));
static void fsm_sconfreq __P((fsm *, int));
*/
//#define PROTO_NAME(f)	((f)->callbacks->proto_name)
int peer_mru[NUM_PPP];

rng0::FSM::FSM()
{
	this->state = INITIAL;
	this->flags = 0;
	this->id = 0;				/* XXX Start with random id? */
	this->timeouttime = DEFTIMEOUT;
	this->maxconfreqtransmits = DEFMAXCONFREQS;
	this->maxtermtransmits = DEFMAXTERMREQS;
	this->maxnakloops = DEFMAXNAKLOOPS;
	this->term_reason_len = 0;
}

/*
 * fsm_lowerup - The lower layer is up.
 */
void rng0::FSM::lowerup()
{
	switch( this->state ){
		case INITIAL:
			this->state = CLOSED;
			break;

		case STARTING:
			if( this->flags & OPT_SILENT )
				this->state = STOPPED;
			else {
				/* Send an initial configure-request */
				this->sconfreq(0);
				this->state = REQSENT;
			}
			break;

		default:
			FSMDEBUG(("%s: Up event in state %d!", this->proto_name, this->state));
	}
}

/*
 * fsm_lowerdown - The lower layer is down.
 *
 * Cancel all timeouts and inform upper layers.
 */

void rng0::FSM::lowerdown()
{
	switch( this->state ){
		case CLOSED:
			this->state = INITIAL;
			break;

		case STOPPED:
			this->state = STARTING;
			if( this->callbacks->starting )
				(*this->callbacks->starting)(this);
			break;

		case CLOSING:
			this->state = INITIAL;
			UNTIMEOUT(this->timeout, this);	/* Cancel timeout */
			break;

		case STOPPING:
		case REQSENT:
		case ACKRCVD:
		case ACKSENT:
			this->state = STARTING;
			UNTIMEOUT(this->timeout, this);	/* Cancel timeout */
			break;

		case OPENED:
			if( this->callbacks->down )
				(*this->callbacks->down)(this);
			this->state = STARTING;
			break;

		default:
			FSMDEBUG(("%s: Down event in state %d!", this->proto_name(), this->state));
	}
}

/*
 * fsm_open - Link is allowed to come up.
 */
void rng0::FSM::open()
{
	switch( this->state ){
		case INITIAL:
			this->state = STARTING;
			if( this->callbacks->starting )
				(*this->callbacks->starting)(this);
			break;

		case CLOSED:
			if( this->flags & OPT_SILENT )
				this->state = STOPPED;
			else {
				/* Send an initial configure-request */
				this->sconfreq(0);
				this->state = REQSENT;
			}
			break;

		case CLOSING:
			this->state = STOPPING;
			/* fall through */
		case STOPPED:
		case OPENED:
			if( this->flags & OPT_RESTART ){
				this->lowerdown();
				this->lowerup();
			}
			break;
		default:
			break;
	}
}

/*
 * terminate_layer - Start process of shutting down the FSM
 *
 * Cancel any timeout running, notify upper layers we're done, and
 * send a terminate-request message as configured.
 */
void rng0::FSM::terminate_layer(State nextstate)
{
	if( this->state != OPENED )
		;
		//UNTIMEOUT(timeout, this);	/* Cancel timeout */
	else if( this->callbacks->down )
		(*this->callbacks->down)(this);	/* Inform upper layers we're down */

	/* Init restart counter and send Terminate-Request */
	this->retransmits = this->maxtermtransmits;
	this->sdata(TERMREQ, this->reqid = ++this->id,
	          (u_char *) this->term_reason, this->term_reason_len);

	if (this->retransmits == 0) {
		/*
		 * User asked for no terminate requests at all; just close it.
		 * We've already fired off one Terminate-Request just to be nice
		 * to the peer, but we're not going to wait for a reply.
		 */
		this->state = nextstate == CLOSING ? CLOSED : STOPPED;
		if( this->callbacks->finished )
			(*this->callbacks->finished)(this);
		return;
	}

	//TIMEOUT(timeout, this, this->timeouttime);
	--this->retransmits;

	this->state = nextstate;
}

/*
 * fsm_close - Start closing connection.
 *
 * Cancel timeouts and either initiate close or possibly go directly to
 * the CLOSED state.
 */
void rng0::FSM::close(char *reason)
{
	this->term_reason = reason;
	this->term_reason_len = (reason == nullptr? 0: strlen(reason));
	switch( this->state ){
		case STARTING:
			this->state = INITIAL;
			break;
		case STOPPED:
			this->state = CLOSED;
			break;
		case STOPPING:
			this->state = CLOSING;
			break;

		case REQSENT:
		case ACKRCVD:
		case ACKSENT:
		case OPENED:
		default:
			this->terminate_layer(CLOSING);
			break;
	}
}

/*
 * fsm_timeout - Timeout expired.
 */
void rng0::FSM::timeout(void *arg)
{
	FSM *f = (FSM *) arg;

	switch (f->state) {
		case CLOSING:
		case STOPPING:
			if( f->retransmits <= 0 ){
				/*
				 * We've waited for an ack long enough.  Peer probably heard us.
				 */
				f->state = (f->state == CLOSING)? CLOSED: STOPPED;
				if( f->callbacks->finished )
					(*f->callbacks->finished)(f);
			} else {
				/* Send Terminate-Request */
				f->sdata(TERMREQ, f->reqid = ++f->id,
				          (u_char *) f->term_reason, f->term_reason_len);
				//::TIMEOUT(fsm_timeout, f, f->timeouttime);
				--f->retransmits;
			}
			break;

		case REQSENT:
		case ACKRCVD:
		case ACKSENT:
			if (f->retransmits <= 0) {
				warn((char *)"%s: timeout sending Config-Requests\n", f->proto_name());
				f->state = STOPPED;
				if( (f->flags & OPT_PASSIVE) == 0 && f->callbacks->finished )
					(*f->callbacks->finished)(f);

			} else {
				/* Retransmit the configure-request */
				if (f->callbacks->retransmit)
					(*f->callbacks->retransmit)(f);
				f->sconfreq(1);		/* Re-send Configure-Request */
				if( f->state == ACKRCVD )
					f->state = REQSENT;
			}
			break;

		default:
			FSMDEBUG(("%s: Timeout event in state %d!", this->protoname(), f->state));
	}
}

/*
 * fsm_input - Input packet.
 */
void rng0::FSM::input(u_char *inpacket, int l)
{
	u_char *inp;
	u_char code, id;
	int len;

	/*
	 * Parse header (code, id and length).
	 * If packet too short, drop it.
	 */
	inp = inpacket;
	if (l < HEADERLEN) {
		FSMDEBUG(("fsm_input(%x): Rcvd short header.", this->protocol));
		return;
	}
	GETCHAR(code, inp);
	GETCHAR(id, inp);
	GETSHORT(len, inp);
	if (len < HEADERLEN) {
		FSMDEBUG(("fsm_input(%x): Rcvd illegal length.", this->protocol));
		return;
	}
	if (len > l) {
		FSMDEBUG(("fsm_input(%x): Rcvd short packet.", this->protocol));
		return;
	}
	len -= HEADERLEN;		/* subtract header length */

	if( this->state == INITIAL || this->state == STARTING ){
		FSMDEBUG(("fsm_input(%x): Rcvd packet in state %d.",
				this->protocol, this->state));
		return;
	}

	/*
	 * Action depends on code.
	 */
	switch (code) {
		case CONFREQ:
			this->rconfreq(id, inp, len);
			break;

		case CONFACK:
			this->rconfack(id, inp, len);
			break;

		case CONFNAK:
		case CONFREJ:
			this->rconfnakrej(code, id, inp, len);
			break;

		case TERMREQ:
			this->rtermreq(id, inp, len);
			break;

		case TERMACK:
			this->rtermack();
			break;

		case CODEREJ:
			this->rcoderej(inp, len);
			break;

		default:
			if( !this->callbacks->extcode
			    || !(*this->callbacks->extcode)(this, code, id, inp, len) )
				this->sdata(CODEREJ, ++this->id, inpacket, (size_t)len + HEADERLEN);
			break;
	}
}

/*
 * fsm_rconfreq - Receive Configure-Request.
 */
void rng0::FSM::rconfreq(u_char id, u_char *inp, int len)
{

	int code, reject_if_disagree;

	switch( this->state ){
		case CLOSED:
			/* Go away, we're closed */
			this->sdata(TERMACK, id, nullptr, 0);
			return;
		case CLOSING:
		case STOPPING:
			return;

		case OPENED:
			/* Go down and restart negotiation */
			if( this->callbacks->down )
				(*this->callbacks->down)(this);	/* Inform upper layers */
			this->sconfreq(0);		/* Send initial Configure-Request */
			this->state = REQSENT;
			break;

		case STOPPED:
			/* Negotiation started by our peer */
			this->sconfreq(0);		/* Send initial Configure-Request */
			this->state = REQSENT;
			break;
		default:
			break;
	}

	/*
	 * Pass the requested configuration options
	 * to protocol-specific code for checking.
	 */
	if (this->callbacks->reqci){		/* Check CI */
		reject_if_disagree = (this->nakloops >= this->maxnakloops);
		code = (*this->callbacks->reqci)(this, inp, &len, reject_if_disagree);
	} else if (len)
		code = CONFREJ;			/* Reject all CI */
	else
		code = CONFACK;

	/* send the Ack, Nak or Rej to the peer */
	this->sdata((u_char)code, id, inp, (size_t)len);

	if (code == CONFACK) {
		if (this->state == ACKRCVD) {
			UNTIMEOUT(timeout, this);	/* Cancel timeout */
			this->state = OPENED;
			if (this->callbacks->up)
				(*this->callbacks->up)(this);	/* Inform upper layers */
		} else
			this->state = ACKSENT;
		this->nakloops = 0;

	} else {
		/* we sent CONFNAK or CONFREJ */
		if (this->state != ACKRCVD)
			this->state = REQSENT;
		if( code == CONFNAK )
			++this->nakloops;
	}
}

/*
 * fsm_rconfack - Receive Configure-Ack.
 */
void rng0::FSM::rconfack(u_char id, u_char *inp, int len)
{
	if (id != this->reqid || this->seen_ack)		/* Expected id? */
		return;					/* Nope, toss... */
	if(this->callbacks->ackci ? len == 0 : (*this->callbacks->ackci)(this, inp, len)){
		/* Ack is bad - ignore it */
		error((char *)"Received bad configure-ack: %P", inp, len);
		return;
	}
	this->seen_ack = 1;
	this->rnakloops = 0;

	switch (this->state){
		case CLOSED:
		case STOPPED:
			this->sdata(TERMACK, id, nullptr, 0);
			break;

		case REQSENT:
			this->state = ACKRCVD;
			this->retransmits = this->maxconfreqtransmits;
			break;

		case ACKRCVD:
			/* Huh? an extra valid Ack? oh well... */
//			UNTIMEOUT(timeout, this);	/* Cancel timeout */
			this->sconfreq(0);
			this->state = REQSENT;
			break;

		case ACKSENT:
//			UNTIMEOUT(timeout, this);	/* Cancel timeout */
			this->state = OPENED;
			this->retransmits = this->maxconfreqtransmits;
			if (this->callbacks->up)
				(*this->callbacks->up)(this);	/* Inform upper layers */
			break;

		case OPENED:
			/* Go down and restart negotiation */
			if (this->callbacks->down)
				(*this->callbacks->down)(this);	/* Inform upper layers */
			this->sconfreq(0);		/* Send initial Configure-Request */
			this->state = REQSENT;
			break;
		default:
			break;
	}
}

/*
 * fsm_rconfnakrej - Receive Configure-Nak or Configure-Reject.
 */
void rng0::FSM::rconfnakrej(int code,  int id, u_char *inp, int len)
{
	int ret;
	int treat_as_reject;

	if (id != this->reqid || this->seen_ack)	/* Expected id? */
		return;				/* Nope, toss... */

	if (code == CONFNAK) {
		++this->rnakloops;
		treat_as_reject = (this->rnakloops >= this->maxnakloops);
		if (this->callbacks->nakci == nullptr
		    || !(ret = this->callbacks->nakci(this, inp, len, treat_as_reject))) {
			error((char *)"Received bad configure-nak: %P", inp, len);
			return;
		}
	} else {
		this->rnakloops = 0;
		if (this->callbacks->rejci == nullptr
		    || !(ret = this->callbacks->rejci(this, inp, len))) {
			error((char *)"Received bad configure-rej: %P", inp, len);
			return;
		}
	}

	this->seen_ack = 1;

	switch (this->state) {
		case CLOSED:
		case STOPPED:
			this->sdata(TERMACK, (u_char)id, nullptr, 0);
			break;

		case REQSENT:
		case ACKSENT:
			/* They didn't agree to what we wanted - try another request */
			// UNTIMEOUT(fsm_timeout, f);	/* Cancel timeout */
			if (ret < 0)
				this->state = STOPPED;		/* kludge for stopping CCP */
			else
				this->sconfreq(0);		/* Send Configure-Request */
			break;

		case ACKRCVD:
			/* Got a Nak/reject when we had already had an Ack?? oh well... */
			//UNTIMEOUT(timeout, this);	/* Cancel timeout */
			this->sconfreq(0);
			this->state = REQSENT;
			break;

		case OPENED:
			/* Go down and restart negotiation */
			if (this->callbacks->down)
				(*this->callbacks->down)(this);	/* Inform upper layers */
			this->sconfreq(0);		/* Send initial Configure-Request */
			this->state = REQSENT;
			break;
		default:
			break;
	}
}

/*
 * fsm_rtermreq - Receive Terminate-Req.
 */
void rng0::FSM::rtermreq(u_char id, u_char *p, int len)
{
	switch (this->state) {
		default:
		case ACKRCVD:
		case ACKSENT:
			this->state = REQSENT;		/* Start over but keep trying */
			break;

		case OPENED:
			if (len > 0) {
				info((char *)"%s terminated by peer (%0.*v)", this->proto_name(), len, p);
			} else
				info((char *)"%s terminated by peer", this->proto_name());
			this->retransmits = 0;
			this->state = STOPPING;
			if (this->callbacks->down)
				(*this->callbacks->down)(this);	/* Inform upper layers */
			//TIMEOUT(fsm_timeout, f, this->timeouttime);
			break;
	}

	this->sdata(TERMACK, id, nullptr, 0);
}

/*
 * fsm_rtermack - Receive Terminate-Ack.
 */
void rng0::FSM::rtermack()
{
	switch (this->state) {
		case CLOSING:
			//UNTIMEOUT(fsm_timeout, f);
			this->state = CLOSED;
			if( this->callbacks->finished )
				(*this->callbacks->finished)(this);
			break;
		case STOPPING:
			///UNTIMEOUT(fsm_timeout, f);
			this->state = STOPPED;
			if( this->callbacks->finished )
				(*this->callbacks->finished)(this);
			break;

		case ACKRCVD:
			this->state = REQSENT;
			break;

		case OPENED:
			if (this->callbacks->down)
				(*this->callbacks->down)(this);	/* Inform upper layers */
			this->sconfreq(0);
			this->state = REQSENT;
			break;
		default:
			break;
	}
}

/*
 * fsm_rcoderej - Receive an Code-Reject.
 */
void rng0::FSM::rcoderej(u_char *inp, int len)
{
	u_char code, id;

	if (len < HEADERLEN) {
		FSMDEBUG(("fsm_rcoderej: Rcvd short Code-Reject packet!"));
		return;
	}
	GETCHAR(code, inp);
	GETCHAR(id, inp);
	warn((char *)"%s: Rcvd Code-Reject for code %d, id %d", this->proto_name(), code, id);

	if( this->state == ACKRCVD )
		this->state = REQSENT;
}

/*
 * fsm_protreject - Peer doesn't speak this protocol.
 *
 * Treat this as a catastrophic error (RXJ-).
 */
void rng0::FSM::protreject()
{
	switch( this->state ){
		case CLOSING:
//			UNTIMEOUT(fsm_timeout, f);	/* Cancel timeout */
			/* fall through */
		case CLOSED:
			this->state = CLOSED;
			if( this->callbacks->finished )
				(*this->callbacks->finished)(this);
			break;

		case STOPPING:
		case REQSENT:
		case ACKRCVD:
		case ACKSENT:
//			UNTIMEOUT(fsm_timeout, f);	/* Cancel timeout */
			/* fall through */
		case STOPPED:
			this->state = STOPPED;
			if( this->callbacks->finished )
				(*this->callbacks->finished)(this);
			break;

		case OPENED:
			this->terminate_layer(STOPPING);
			break;

		default:
			FSMDEBUG(("%s: Protocol-reject event in state %d!",
					this->protoname(), this->state));
	}
}

/*
 * fsm_sconfreq - Send a Configure-Request.
 */
void rng0::FSM::sconfreq(int retransmit)
{
	u_char *outp;
	int cilen;

	if( this->state != REQSENT && this->state != ACKRCVD && this->state != ACKSENT ){
		/* Not currently negotiating - reset options */
		if( this->callbacks->resetci )
			(*this->callbacks->resetci)(this);
		this->nakloops = 0;
		this->rnakloops = 0;
	}

	if( !retransmit ){
		/* New request - reset retransmission counter, use new ID */
		this->retransmits = this->maxconfreqtransmits;
		this->reqid = ++this->id;
	}

	this->seen_ack = 0;

	/*
	 * Make up the request packet
	 */
	outp = outpacket_buf + PPP_HDRLEN + HEADERLEN;
	if( this->callbacks->cilen && this->callbacks->addci ){
		cilen = (*this->callbacks->cilen)(this);
		if( cilen > peer_mru[this->unit] - HEADERLEN )
			cilen = peer_mru[this->unit] - (int)HEADERLEN;
		if (this->callbacks->addci)
			(*this->callbacks->addci)(this, outp, &cilen);
	} else
		cilen = 0;

	/* send the request to our peer */
	this->sdata(CONFREQ, this->reqid, outp, (size_t)cilen);

	/* start the retransmit timer */
	--this->retransmits;
	//TIMEOUT(fsm_timeout, f, this->timeouttime);
}

/*
 * fsm_sdata - Send some data.
 *
 * Used for all packets sent to our peer by this module.
 */
void rng0::FSM::sdata(u_char code, u_char id, u_char *data, size_t datalen)
{
	u_char *outp;
	size_t outlen;

	/* Adjust length to be smaller than MTU */
	outp = outpacket_buf;
	if (datalen > peer_mru[this->unit] - HEADERLEN)
		datalen = peer_mru[this->unit] - HEADERLEN;
	if (datalen && data != outp + PPP_HDRLEN + HEADERLEN)
		BCOPY(data, outp + PPP_HDRLEN + HEADERLEN, datalen);
	outlen = datalen + HEADERLEN;
	MAKEHEADER(outp, this->protocol);
	PUTCHAR(code, outp);
	PUTCHAR(id, outp);
	PUTSHORT(outlen, outp);
	output(this->unit, outpacket_buf, (int)outlen + PPP_HDRLEN);
}






















