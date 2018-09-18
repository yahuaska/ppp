//
// Created by ringo on 13.09.18.
//

#ifndef PPP_MAIN_HPP
#define PPP_MAIN_HPP
/*
 * main.c - Point-to-Point Protocol main module
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
 *
 * Copyright (c) 1999-2004 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 3. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define RCSID    "$Id: main.c,v 1.156 2008/06/23 11:47:18 paulus Exp $"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <netdb.h>
#include <utmp.h>
#include <pwd.h>
#include <setjmp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pppd.hpp"
#include "magic.h"
#include "fsm.hpp"
#include "lcp.hpp"
//#include "ipcp.h"

#ifdef INET6
//#include "ipv6cp.h"
#endif

//#include "upap.h"
//#include "chap-new.h"
//#include "eap.h"
//#include "ccp.h"
//#include "ecp.h"
#include "pathnames.h"

#ifdef USE_TDB
#include "tdb.h"
#endif

#ifdef CBCP_SUPPORT
#include "cbcp.h"
#endif

#ifdef IPX_CHANGE
#include "ipxcp.h"
#endif /* IPX_CHANGE */
#ifdef AT_CHANGE
#include "atcp.h"
#endif

static const char rcsid[] = RCSID;

/* interface vars */
char ifname[MAXIFNAMELEN];    /* Interface name */
int ifunit;            /* Interface unit number */

struct channel *the_channel;

char *progname;            /* Name of this program */
char hostname[MAXNAMELEN];    /* Our hostname */
static char pidfilename[MAXPATHLEN];    /* name of pid file */
static char linkpidfile[MAXPATHLEN];    /* name of linkname pid file */
char ppp_devnam[MAXPATHLEN];    /* name of PPP tty (maybe ttypx) */
uid_t uid;            /* Our real user-id */
struct notifier *pidchange = nullptr;
struct notifier *phasechange = nullptr;
struct notifier *exitnotify = nullptr;
struct notifier *sigreceived = nullptr;
struct notifier *fork_notifier = nullptr;

int hungup;            /* terminal has been hung up */
int privileged;            /* we're running as real uid root */
int need_holdoff;        /* need holdoff period before restarting */
int detached;            /* have detached from terminal */
volatile int status;        /* exit status for pppd */
int unsuccess;            /* # unsuccessful connection attempts */
int do_callback;        /* != 0 if we should do callback next */
int doing_callback;        /* != 0 if we are doing callback */
int ppp_session_number;        /* Session number, for channels with such a
				   concept (eg PPPoE) */
int childwait_done;        /* have timed out waiting for children */

#ifdef USE_TDB
TDB_CONTEXT *pppdb;		/* database for storing status etc. */
#endif

char db_key[32];

int (*holdoff_hook)__P((void)) = nullptr;

int (*new_phase_hook)__P((int)) = nullptr;

void (*snoop_recv_hook)__P((unsigned char * p, int
		                           len)) = nullptr;

void (*snoop_send_hook)__P((unsigned char * p, int
		                           len)) = nullptr;

static int conn_running;    /* we have a [dis]connector running */
static int fd_loop;        /* fd for getting demand-dial packets */

int fd_devnull;            /* fd for /dev/null */
int devfd = -1;            /* fd of underlying device */
int fd_ppp = -1;        /* fd for talking PPP */
int phase;            /* where the link is at */
int kill_link;
int asked_to_quit;
int open_ccp_flag;
int listen_time;
int got_sigusr2;
int got_sigterm;
int got_sighup;

static sigset_t signals_handled;
static int waiting;
static sigjmp_buf sigjmp;

char **script_env;        /* Env. variable values for scripts */
int s_env_nalloc;        /* # words avail at script_env */

u_char outpacket_buf[PPP_MRU + PPP_HDRLEN]; /* buffer for outgoing packet */
u_char inpacket_buf[PPP_MRU + PPP_HDRLEN]; /* buffer for incoming packet */

static int n_children;        /* # child processes still running */
static int got_sigchld;        /* set if we have received a SIGCHLD */

int privopen;            /* don't lock, open device as root */

char *no_ppp_msg = "Sorry - this system lacks PPP kernel support\n";

GIDSET_TYPE groups[NGROUPS_MAX];/* groups the user is in */
int ngroups;            /* How many groups valid in groups */

static struct timeval start_time;    /* Time when link was started. */

static struct pppd_stats old_link_stats;
struct pppd_stats link_stats;
unsigned link_connect_time;
int link_stats_valid;

int error_count;

bool bundle_eof;
bool bundle_terminating;

/*
 * We maintain a list of child process pids and
 * functions to call when they exit.
 */
struct subprocess {
	pid_t pid;
	char *prog;

	void (*done)__P((void * ));

	void *arg;
	int killable;
	struct subprocess *next;
};

static struct subprocess *children;

/* Prototypes for procedures local to this file. */

static void setup_signals __P((void));

static void create_pidfile __P((int
		                               pid));

static void create_linkpidfile __P((int
		                                   pid));

static void cleanup __P((void));

static void get_input __P((void));

static void calltimeout __P((void));

static struct timeval *timeleft __P((struct timeval *));

static void kill_my_pg __P((int));

static void hup __P((int));

static void term __P((int));

static void chld __P((int));

static void toggle_debug __P((int));

static void open_ccp __P((int));

static void bad_signal __P((int));

static void holdoff_end __P((void * ));

static void forget_child __P((int
		                             pid, int
		                             status));

static int reap_kids __P((void));

static void childwait_end __P((void * ));

#ifdef USE_TDB
static void update_db_entry __P((void));
static void add_db_key __P((const char *));
static void delete_db_key __P((const char *));
static void cleanup_db __P((void));
#endif

static void handle_events __P((void));

void print_link_stats __P((void));

extern char *getlogin __P((void));

int main __P((int, char * []));

#ifdef ultrix
#undef	O_NONBLOCK
#define	O_NONBLOCK	O_NDELAY
#endif

#ifdef ULTRIX
#define setlogmask(x)
#endif
#endif //PPP_MAIN_HPP
