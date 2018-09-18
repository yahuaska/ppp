#include "main.hpp"

/*
 * PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 * The last entry must be nullptr.
 */
static rng0::FSM *protos[16];

int main(int argc, char *argv[])
{
	auto lcp =new rng0::LCP(0);
	protos[0] = lcp;
	int i, t;
	char *p;
	struct passwd *pw;
	char numbuf[16];

	link_stats_valid = 0;
	new_phase(PHASE_INITIALIZE);

	script_env = nullptr;

	/* Initialize syslog facilities */
	reopen_log();

	if (gethostname(hostname, MAXNAMELEN) < 0) {
		option_error("Couldn't get hostname: %m");
		exit(1);
	}
	hostname[MAXNAMELEN - 1] = 0;

	/* make sure we don't create world or group writable files. */
	umask(umask(0777) | 022);

	uid = getuid();
	privileged = uid == 0;
	slprintf(numbuf, sizeof(numbuf), "%d", uid);
	script_setenv("ORIG_UID", numbuf, 0);

	ngroups = getgroups(NGROUPS_MAX, groups);

	/*
	 * Initialize magic number generator now so that protocols may
	 * use magic numbers in initialization.
	 */
	magic_init();
	/*
	 * Initialize the default channel.
	 */
	tty_init();

	progname = *argv;

	/*
	 * Parse, in order, the system options file, the user's options file,
	 * and the command line arguments.
	 */
	if (!options_from_file(_PATH_SYSOPTIONS, !privileged, 0, 1)
	    || !options_from_user()
	    || !parse_args(argc - 1, argv + 1))
		exit(EXIT_OPTION_ERROR);
	devnam_fixed = 1;        /* can no longer change device name */

	/*
	 * Work out the device name, if it hasn't already been specified,
	 * and parse the tty's options file.
	 */
	if (the_channel->process_extra_options)
		(*the_channel->process_extra_options)();

	if (debug)
		setlogmask(LOG_UPTO(LOG_DEBUG));

	/*
	 * Check that we are running as root.
	 */
	if (geteuid() != 0) {
		option_error("must be root to run %s, since it is not setuid-root", argv[0]);
		exit(EXIT_NOT_ROOT);
	}

	if (!ppp_available()) {
		option_error("%s", no_ppp_msg);
		exit(EXIT_NO_KERNEL_SUPPORT);
	}

	/*
	 * Check that the options given are valid and consistent.
	 */
	check_options();
	if (!sys_check_options())
		exit(EXIT_OPTION_ERROR);
	auth_check_options();
#ifdef HAVE_MULTILINK
	mp_check_options();
#endif
	rng0::FSM *prot = nullptr;
	for (i = 0; (prot = protos[i]) != nullptr; ++i) {
		prot->check_options();
	}
	if (the_channel->check_options)
		(*the_channel->check_options)();


	if (dump_options || dryrun) {
		init_pr_log(nullptr, LOG_INFO);
		print_options(pr_log, nullptr);
		end_pr_log();
	}

	if (dryrun)
		die(0);

	/* Make sure fds 0, 1, 2 are open to somewhere. */
	fd_devnull = open(_PATH_DEVNULL, O_RDWR);
	if (fd_devnull < 0)
		fatal("Couldn't open %s: %m", _PATH_DEVNULL);
	while (fd_devnull <= 2) {
		i = dup(fd_devnull);
		if (i < 0)
			fatal("Critical shortage of file descriptors: dup failed: %m");
		fd_devnull = i;
	}

	/*
	 * Initialize system-dependent stuff.
	 */
	sys_init();

#ifdef USE_TDB
	pppdb = tdb_open(_PATH_PPPDB, 0, 0, O_RDWR|O_CREAT, 0644);
	if (pppdb != nullptr) {
	slprintf(db_key, sizeof(db_key), "pppd%d", getpid());
	update_db_entry();
	} else {
	warn("Warning: couldn't open ppp database %s", _PATH_PPPDB);
	if (multilink) {
		warn("Warning: disabling multilink");
		multilink = 0;
	}
	}
#endif

	/*
	 * Detach ourselves from the terminal, if required,
	 * and identify who is running us.
	 */
	if (!nodetach && !updetach)
		detach();
	p = getlogin();
	if (p == nullptr) {
		pw = getpwuid(uid);
		if (pw != nullptr && pw->pw_name != nullptr)
			p = pw->pw_name;
		else
			p = "(unknown)";
	}
	syslog(LOG_NOTICE, "pppd %s started by %s, uid %d", VERSION, p, uid);
	script_setenv("PPPLOGNAME", p, 0);

	if (devnam[0])
		script_setenv("DEVICE", devnam, 1);
	slprintf(numbuf, sizeof(numbuf), "%d", getpid());
	script_setenv("PPPD_PID", numbuf, 1);

	setup_signals();

	create_linkpidfile(getpid());

	waiting = 0;

	/*
	 * If we're doing dial-on-demand, set up the interface now.
	 */
	if (demand) {
		/*
		 * Open the loopback channel and set it up to be the ppp interface.
		 */
		fd_loop = open_ppp_loopback();
		set_ifunit(1);
		/*
		 * Configure the interface and mark it up, etc.
		 */
		demand_conf();
	}

	do_callback = 0;
	for (;;) {

		bundle_eof = 0;
		bundle_terminating = 0;
		listen_time = 0;
		need_holdoff = 1;
		devfd = -1;
		status = EXIT_OK;
		++unsuccess;
		doing_callback = do_callback;
		do_callback = 0;

		if (demand && !doing_callback) {
			/*
			 * Don't do anything until we see some activity.
			 */
			new_phase(PHASE_DORMANT);
			demand_unblock();
			add_fd(fd_loop);
			for (;;) {
				handle_events();
				if (asked_to_quit)
					break;
				if (get_loop_output())
					break;
			}
			remove_fd(fd_loop);
			if (asked_to_quit)
				break;

			/*
			 * Now we want to bring up the link.
			 */
			demand_block();
			info("Starting link");
		}

		gettimeofday(&start_time, nullptr);
		script_unsetenv("CONNECT_TIME");
		script_unsetenv("BYTES_SENT");
		script_unsetenv("BYTES_RCVD");

		lcp->open();
		start_link(0);
		while (phase != PHASE_DEAD) {
			handle_events();
			get_input();
			if (kill_link)
				lcp->close("User request");
			if (asked_to_quit) {
				bundle_terminating = 1;
				if (phase == PHASE_MASTER)
						mp_bundle_terminated();
			}
			if (open_ccp_flag) {
				if (phase == PHASE_NETWORK || phase == PHASE_RUNNING) {
					ccp_fsm[0].flags = rng0::OPT_RESTART; /* clears OPT_SILENT */
					(*ccp_protent.open)(0);
				}
			}
		}
		/* restore FSMs to original state */
		lcp->close("");

		if (!persist || asked_to_quit || (maxfail > 0 && unsuccess >= maxfail))
			break;

		if (demand)
			demand_discard();
		t = need_holdoff ? holdoff : 0;
		if (holdoff_hook)
			t = (*holdoff_hook)();
		if (t > 0) {
			new_phase(PHASE_HOLDOFF);
			TIMEOUT(holdoff_end, nullptr, t);
			do {
				handle_events();
				if (kill_link)
					new_phase(PHASE_DORMANT); /* allow signal to end holdoff */
			} while (phase == PHASE_HOLDOFF);
			if (!persist)
				break;
		}
	}

	/* Wait for scripts to finish */
	reap_kids();
	if (n_children > 0) {
		if (child_wait > 0)
			TIMEOUT(childwait_end, nullptr, child_wait);
		if (debug) {
			struct subprocess *chp;
			dbglog("Waiting for %d child processes...", n_children);
			for (chp = children; chp != nullptr; chp = chp->next)
				dbglog("  script %s, pid %d", chp->prog, chp->pid);
		}
		while (n_children > 0 && !childwait_done) {
			handle_events();
			if (kill_link && !childwait_done)
				childwait_end(nullptr);
		}
	}

	die(status);
	return 0;
}

/*
 * new_phase - signal the start of a new phase of pppd's operation.
 */
void new_phase(int p)
{
	phase = p;
	if (new_phase_hook)
		(*new_phase_hook)(p);
	notify(phasechange, p);
}